#include "sync.h"
#include "config.h"
#include "db.h"
#include "schema.h"
#include "libdb/query.h"
#include "libdb/recordset.h"
#include "libutil/diff.h"
#include "libutil/file.h"
#include "libutil/trace.h"
#include "libutil/stream.h"
#include "libutil/errors.h"
#include "libutil/counted_pointer.h"
#include <time.h>
#include <string.h>
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

namespace mediadb {

Synchroniser::Synchroniser(Database *src, Database *dest, bool dry_run)
    : m_src(src), 
      m_dest(dest), 
      m_dry_run(dry_run),
      m_any_deleted(false)
{
    memset(&m_statistics, 0, sizeof(m_statistics));
}

static unsigned int GetRecord(db::Database *db, unsigned int id,
			      db::RecordsetPtr *rs)
{
    db::QueryPtr qp = db->CreateQuery();
    qp->Where(qp->Restrict(mediadb::ID, db::EQ, id));
    *rs = qp->Execute();
    if (!*rs || (*rs)->IsEOF())
    {
	TRACE << "Expected record " << id << " not found\n";
	return ENOENT;
    }
    return 0;
}

struct NameAndType
{
    std::string name;
    bool is_composite;

    bool operator==(const NameAndType& other) const
    {
	return name == other.name && is_composite == other.is_composite;
    }
};

/** Given a vector of IDs, produce the corresponding vector of titles.
 */
static unsigned int GetNames(db::Database *db,
			     std::vector<unsigned int>* ids,
			     std::vector<NameAndType> *names)
{
    names->clear();
    names->reserve(ids->size());
    for (std::vector<unsigned int>::iterator i = ids->begin();
	 i != ids->end();
	 ++i)
    {
	db::QueryPtr qp = db->CreateQuery();
	qp->Where(qp->Restrict(mediadb::ID, db::EQ, *i));
	db::RecordsetPtr rs = qp->Execute();
	if (!rs || rs->IsEOF())
	{
	    TRACE << "Expected record " << *i << " not found\n";
	    *i = 0; // Mark as unwanted
	}
	else
	{
	    unsigned int type = rs->GetInteger(mediadb::TYPE);

	    if (type == mediadb::FILE)
		*i = 0; // Unwanted
	    else
	    {
		NameAndType nat;
		nat.name = rs->GetString(mediadb::TITLE);
		nat.is_composite = (type == mediadb::PLAYLIST
				    || type == mediadb::DIR);
		names->push_back(nat);
	    }
	}
    }
    ids->erase(std::remove(ids->begin(), ids->end(), 0), ids->end());

    assert(ids->size() == names->size());

    return 0;
}

static bool Different(db::RecordsetPtr src, db::RecordsetPtr dest,
		      unsigned int field)
{
    if (src->GetString(field) == dest->GetString(field))
	return false;
//    TRACE << src->GetString(mediadb::TITLE) << ": '"
//	  << src->GetString(field) << "' vs '"
//	  << dest->GetString(field) << "'\n";
    return true;
}

/** Match up child items by name, as far as possible.
 *
 * This is possible because we define that all items are reachable
 * from the root item.
 */
unsigned int Synchroniser::CheckPlaylistNames(unsigned int srcid,
					      unsigned int destid,
					      bool *amend_parent)
{
    if (m_srcids_seen.find(srcid) != m_srcids_seen.end())
    {
	if (m_srctodest.find(srcid) != m_srctodest.end())
	{
	    if (m_srctodest[srcid] != destid)
	    {
//		TRACE << "Unification: srcid " << srcid << " is both destid "
//		      << destid << " and destid " << m_srctodest[srcid]
//		      << "\n";
		m_destids_maybe_delete.insert(destid);
                if (amend_parent)
                {
                    *amend_parent = true;
                }
	    }
	    return 0;
	}
    }
    m_srcids_seen.insert(srcid);

    std::vector<unsigned int> src_ids;
    db::RecordsetPtr srcrs;
    unsigned int rc = GetRecord(m_src, srcid, &srcrs);
    if (rc)
	return rc;
    mediadb::ChildrenToVector(srcrs->GetString(mediadb::CHILDREN), &src_ids);

    std::vector<unsigned int> dest_ids;
    db::RecordsetPtr destrs;
    rc = GetRecord(m_dest, destid, &destrs);
    if (rc)
	return rc;
    mediadb::ChildrenToVector(destrs->GetString(mediadb::CHILDREN), &dest_ids);

    unsigned int desttype = destrs->GetInteger(mediadb::TYPE);
    bool destcomposite = (desttype == mediadb::DIR
			 || desttype == mediadb::PLAYLIST);

    if (!destcomposite)
    {
	uint32_t destsize = destrs->GetInteger(mediadb::SIZEBYTES);
	uint32_t srcsize = srcrs->GetInteger(mediadb::SIZEBYTES);

	// Upgrade to FLAC if possible
	unsigned int tunehigh = srcrs->GetInteger(mediadb::IDHIGH);
	if (tunehigh)
	{
	    db::RecordsetPtr rs2;
	    rc = GetRecord(m_src, tunehigh, &rs2);
	    if (!rc)
	    {
		m_srctodest[srcid] = destid;
		srcid = tunehigh;
		srcsize = rs2->GetInteger(mediadb::SIZEBYTES);
	    }
	}

	if (srcsize != destsize)
	{
	    /** As two composite things are never "different" in this
	     * sense, we can assume they're files and that srcid is just a
	     * newer version.
	     */
	    m_srcids_add_tune.insert(srcid);
//	    TRACE << "Replacing destid " << destid << " with srcid " << srcid
//		  << " in-place\n";
	}
	else if (Different(srcrs, destrs, mediadb::ARTIST)
		 || Different(srcrs, destrs, mediadb::ALBUM)
		 || Different(srcrs, destrs, mediadb::GENRE)
		 || Different(srcrs, destrs, mediadb::TRACKNUMBER)
		 || Different(srcrs, destrs, mediadb::YEAR))
	{
	    m_srcids_amend_metadata.insert(srcid);
	    m_statistics.files_modified++;
	}
    }

    m_desttosrc[destid] = srcid;
    m_srctodest[srcid] = destid;

    std::vector<NameAndType> src_names;
    rc = GetNames(m_src, &src_ids, &src_names);
    if (rc)
	return rc;
    std::vector<NameAndType> dest_names;
    rc = GetNames(m_dest, &dest_ids, &dest_names);
    if (rc)
	return rc;

    util::DiffResult res;

    unsigned int dist = util::Diff(dest_names.begin(), dest_names.end(),
				   src_names.begin(), src_names.end(), &res);
    (void)dist;

    unsigned int src_index = 0, dest_index = 0;
    bool amend = false;

    while (src_index < src_ids.size()
	   || dest_index < dest_ids.size())
    {
//	TRACE << "si " << src_index << "/" << src_ids.size()
//	      << " di " << dest_index << "/" << dest_ids.size()
//	      << " add " << res.additions << "\n";
	
	if (src_index < src_ids.size()
	    && res.additions.find(src_index+1) != res.additions.end())
	{
	    m_srcids_maybe_add.insert(src_ids[src_index]);
	    amend = true;
	    ++src_index;
	}
	else if (dest_index < dest_ids.size()
		 && res.deletions.find(dest_index+1) != res.deletions.end())
	{
//	    TRACE << "Maybe deleting destid " << dest_ids[dest_index] << "\n";
	    m_destids_maybe_delete.insert(dest_ids[dest_index]);
	    amend = true;
	    ++dest_index;
	}
	else
	{
//	    TRACE << "srcid " << src_ids[src_index] << " = destid "
//		  << dest_ids[dest_index] << " " << src_names[src_index]
//		  << "\n";

	    rc = CheckPlaylistNames(src_ids[src_index], dest_ids[dest_index],
				    &amend);
	    if (rc)
		return rc;

	    ++src_index;
	    ++dest_index;
	}
    }

    if (amend)
	m_srcids_amend_playlist.insert(srcid);

    return 0;
}

/** Determine whether a particular record needs to be deleted from the
 * destination.
 *
 * Note that (as it may be dealing with IDs that don't exist in the source)
 * this is one of the few functions that operates on a *destination* ID.
 *
 * This can only be done after the initial scan, as it might have been
 * removed from one playlist (making it a deletion candidate) but
 * still be present in one or more others (meaning it can't be
 * deleted).
 *
 * If it has been reprieved due to being rediscovered in another
 * playlist, it will have an entry in m_desttosrc. Otherwise, we might
 * as well delete it straightaway (after recursively MaybeDelete'ing
 * its children, and if m_dry_run isn't set), as it plays no further
 * part in the sync process.
 */
unsigned int Synchroniser::MaybeDelete(unsigned int destid, set_t *already)
{
//    TRACE << "MaybeDelete(" << destid << ")\n";
    if (already->find(destid) != already->end())
    {
//	TRACE << " already deleting\n";
	return 0;
    }
    already->insert(destid);

    /* If it's still accessible via another route, don't delete */
    if (m_desttosrc.find(destid) != m_desttosrc.end())
    {
//	TRACE << " still accessible\n";
	return 0;
    }

    std::vector<unsigned int> child_ids;
    db::RecordsetPtr rs;
    unsigned int rc = GetRecord(m_dest, destid, &rs);
    if (rc)
    {
//	TRACE << " getrecord error " << rc << "\n";
	if (rc == ENOENT)
	    return 0; // Already not there, perhaps due to previous crash
	return rc;
    }
    mediadb::ChildrenToVector(rs->GetString(mediadb::CHILDREN), &child_ids);

    for (std::vector<unsigned int>::const_iterator i = child_ids.begin();
	 i != child_ids.end();
	 ++i)
    {
	rc = MaybeDelete(*i, already);
	if (rc)
	    return rc;
    }

    if (!m_any_deleted)
    {
	Fire(&SyncObserver::OnStage, SyncObserver::DELETING);
	m_any_deleted = true;
    }

//    TRACE << "Deleting destid " << destid << " " 
//	  << rs->GetString(mediadb::TITLE) << "\n";

    switch (rs->GetInteger(mediadb::TYPE))
    {
    case PLAYLIST:
    case DIR:
	m_statistics.playlists_deleted++;
	break;
    default:
	m_statistics.files_deleted++;
	break;
    }

    if (!m_dry_run)
	rc = rs->Delete();

    Fire(&SyncObserver::OnDelete, destid);

    return rc;
}

/** Determine whether a particular record needs to be added to the destination.
 *
 * This can only be done after the initial scan, as we might discover
 * at any time during the scan that the file is one we actually
 * already know about. If so, it'll be in m_srctodest by this stage.
 *
 * If not, we definitely need to transfer it, so it's added to
 * m_srcids_add_tune or m_srcids_add_playlist as appropriate. A new ID
 * is allocated for it in the destination database.
 *
 * We can't actually transfer them as we find them -- we must wait
 * until we have a complete list. If it's a playlist, we can't
 * transfer it until we find out what IDs *all* its children have; if
 * it's a file, we want to transfer the playlists first anyway.
 */
unsigned int Synchroniser::MaybeAdd(unsigned int srcid, set_t *already)
{
    if (already->find(srcid) != already->end())
	return 0;
    already->insert(srcid);

    /* If it's already there, we can do it with linking and needn't transfer */
    if (m_srctodest.find(srcid) != m_srctodest.end())
	return 0;

    db::RecordsetPtr rs;
    unsigned int rc = GetRecord(m_src, srcid, &rs);
    if (rc)
	return rc;

    if (rs->GetInteger(mediadb::TYPE) == mediadb::FILE)
    {
	// Don't bother, then
	m_srctodest[srcid] = 0;
//	TRACE << "Ignoring srcid " << srcid << " "
//	      << rs->GetString(mediadb::TITLE) << "\n";
	return 0;
    }

    std::vector<unsigned int> child_ids;
    mediadb::ChildrenToVector(rs->GetString(mediadb::CHILDREN), &child_ids);

    for (std::vector<unsigned int>::const_iterator i = child_ids.begin();
	 i != child_ids.end();
	 ++i)
    {
	rc = MaybeAdd(*i, already);
	if (rc)
	    return rc;
    }

    unsigned int destid = m_dest->AllocateID();

    m_srctodest[srcid] = destid;
    m_desttosrc[destid] = srcid;

    // Upgrade to FLAC if possible
    unsigned int tunehigh = rs->GetInteger(mediadb::IDHIGH);
    if (tunehigh)
    {
	db::RecordsetPtr rs2;
	rc = GetRecord(m_src, tunehigh, &rs2);
	if (rc == 0)
	{
	    srcid = tunehigh;
	    m_srctodest[srcid] = destid;
	    rs = rs2;
	}
    }

//    TRACE << "Adding srcid " << srcid << " as destid " << destid
//	  << " '" << rs->GetString(mediadb::TITLE) << "'\n";

    switch (rs->GetInteger(mediadb::TYPE))
    {
    case mediadb::PLAYLIST:
    case mediadb::DIR:
	m_statistics.playlists_added++;
	m_srcids_add_playlist.push_back(srcid);
	break;

    case mediadb::TUNE:
    case mediadb::TUNEHIGH:
	m_srcids_add_tune.insert(srcid);

	m_statistics.files_added++;
	m_statistics.bytes_transferred += rs->GetInteger(mediadb::SIZEBYTES);
	break;

    default:
	TRACE << "Can't sync item of type " << rs->GetInteger(mediadb::TYPE)
	      << "\n";
	break;
    }

    return 0;
}

/** Copy a single playlist (or directory) from the source to the destination.
 *
 * Or not, if m_dry_run is set. Clips out any file for which there is
 * no corresponding destination file; typically, these will be
 * non-media files which aren't desired to be copied.
 */
unsigned int Synchroniser::CopyPlaylist(unsigned int srcid,
					unsigned int destid)
{
    db::RecordsetPtr srs;
    unsigned int rc = GetRecord(m_src, srcid, &srs);
    if (rc)
	return rc;

    std::vector<unsigned int> src_ids;
    mediadb::ChildrenToVector(srs->GetString(mediadb::CHILDREN), &src_ids);

    std::vector<unsigned int> dest_ids;
    dest_ids.reserve(src_ids.size());
    for (unsigned int i=0; i<src_ids.size(); ++i)
    {
	unsigned int cdestid = m_srctodest[src_ids[i]];
	if (cdestid)
	    dest_ids.push_back(cdestid);
	else
	{
//	    TRACE << "No destid for srcid " << src_ids[i] << "\n";
	}
    }
    if (dest_ids.size() != src_ids.size())
    {
//	TRACE << src_ids.size() << "-entry playlist became "
//	      << dest_ids.size() << "-entry playlist '"
//	      << srs->GetString(mediadb::TITLE) << "\n";
    }

    if (m_dry_run)
	return 0;

    db::QueryPtr qp = m_dest->CreateQuery();
    qp->Where(qp->Restrict(mediadb::ID, db::EQ, destid));
    db::RecordsetPtr drs = qp->Execute();
    if (!drs || drs->IsEOF())
    {
	drs = m_dest->CreateRecordset();
	drs->AddRecord();
	drs->SetInteger(mediadb::ID, destid);
    }

    drs->SetString(mediadb::TITLE, srs->GetString(mediadb::TITLE));
    drs->SetInteger(mediadb::TYPE, srs->GetInteger(mediadb::TYPE));
    drs->SetString(mediadb::CHILDREN, mediadb::VectorToChildren(dest_ids));
    return drs->Commit();
}

/** Copy the metadata of a single track from the source to the destination.
 *
 * Or not, if m_dry_run is set.
 */
unsigned int Synchroniser::CopyMetadata(unsigned int srcid, 
					unsigned int destid)
{
    db::RecordsetPtr rs;
    unsigned int rc = GetRecord(m_src, srcid, &rs);
    if (rc)
	return rc;

    if (m_dry_run)
	return 0;

    db::RecordsetPtr destrs;
    db::QueryPtr qp = m_dest->CreateQuery();
    qp->Where(qp->Restrict(mediadb::ID, db::EQ, destid));
    destrs = qp->Execute();
    if (!destrs || destrs->IsEOF())
    {
	destrs = m_dest->CreateRecordset();
	destrs->AddRecord();
	destrs->SetInteger(mediadb::ID, destid);
    }

    for (unsigned int i=1; i<mediadb::FIELD_COUNT; ++i)
	destrs->SetString(i, rs->GetString(i));

    if (destrs->GetInteger(mediadb::TYPE) == mediadb::TUNEHIGH)
	destrs->SetInteger(mediadb::TYPE, mediadb::TUNE);
	    
    if (destrs->GetString(mediadb::TITLE).empty())
    {
	destrs->SetString(mediadb::TITLE,
			  util::StripExtension(
			      util::GetLeafName(
				  rs->GetString(mediadb::PATH).c_str())
			      .c_str()));
	TRACE << "Strangely blank title for '"
	      << rs->GetString(mediadb::PATH) << "', using '"
	      << destrs->GetString(mediadb::TITLE) << "'\n";
    }
    
//    TRACE << "Set dest size to " << destrs->GetInteger(mediadb::SIZEBYTES)
//	  << " src size is " << rs->GetInteger(mediadb::SIZEBYTES) << "\n";

    return destrs->Commit();
}

unsigned int Synchroniser::AmendPlaylist(unsigned int srcid,
					 size_t num, size_t denom)
{
//    TRACE << "Amending playlist srcid " << srcid << "\n";
    m_statistics.playlists_modified++;

    Fire(&SyncObserver::OnAmendPlaylist, srcid, num, denom);
    return CopyPlaylist(srcid, m_srctodest[srcid]);
}

unsigned int Synchroniser::AmendTune(unsigned int srcid,
				     size_t num, size_t denom)
{
    Fire(&SyncObserver::OnAmendMetadata, srcid, num, denom);
    return CopyMetadata(srcid, m_srctodest[srcid]);
}

unsigned int Synchroniser::AddPlaylist(unsigned int srcid, 
				       size_t num, size_t denom)
{
    Fire(&SyncObserver::OnAddPlaylist, srcid, num, denom);
    return CopyPlaylist(srcid, m_srctodest[srcid]);
}

/** Copy a single file (not playlist) from the source to the destination.
 *
 * Or not, if m_dry_run is set.
 */
unsigned int Synchroniser::AddTune(unsigned int srcid,
				   size_t num, size_t denom)
{
    Fire(&SyncObserver::OnAddFile, srcid, num, denom);

    unsigned int destid = m_srctodest[srcid];
    if (!destid)
    {
	TRACE << "Can't find destid for srcid " << srcid << "\n";
	return EINVAL;
    }

    unsigned int rc = CopyMetadata(srcid, destid);
    if (rc)
	return rc;

    if (m_dry_run)
	return 0;

    /** @todo SinkFromFile optimisation to allow sendfile()
     */

    std::unique_ptr<util::Stream> ssps = m_src->OpenRead(srcid);
    if (!ssps.get())
    {
	TRACE << "Can't open for read\n";
	return EIO;
    }

    std::unique_ptr<util::Stream> sspd = m_dest->OpenWrite(destid);
    if (!sspd.get())
    {
	TRACE << "Can't open for write\n";
	return EPERM;
    }

    unsigned int len = (unsigned int)ssps->GetLength();
    rc = sspd->SetLength(len);
    if (rc)
    {
	TRACE << "SetLength failed " << rc << "\n";
	return rc;
    }

#if HAVE_GETTIMEOFDAY
    struct timeval tv;
    ::gettimeofday(&tv, NULL);
    uint64_t usec = (((uint64_t)tv.tv_sec) * 1000000) + tv.tv_usec;
#endif
	    
    rc = util::CopyStream(ssps.get(), sspd.get());
    if (rc)
    {
	TRACE << "CopyStream failed " << rc << "\n";
	return rc;
    }

#if HAVE_GETTIMEOFDAY
    ::gettimeofday(&tv, NULL);
    usec = (((uint64_t)tv.tv_sec) * 1000000) + tv.tv_usec - usec;

    if (usec)
    {
//	uint64_t bps = (len * 1000000ull) / usec;
//	TRACE << (usec/1000000) << "s " << bps << "bps "
//	      << (bps/1024) << "Kps\n";
    }
//    else
//	TRACE << "Infinity bps\n";
#endif

    return 0;
}

/** Central synchronisation algorithm.
 *
 * The overall scheme is borne out by the methods called, but for reference:
 *
 *  - Starting at the root directory/playlist, walk both trees in
 *  parallel noting when names match, and recording potential
 *  additions or deletions every time directory/playlist entries don't
 *  match;
 *
 *  - Make sure the deletions aren't "reprieved" by also being
 *  accessible via another route, like perhaps a symlink, and then do
 *  the deletions;
 *
 *  - Make sure the additions aren't existing destination files with
 *  new links, and determine final lists of files and playlists to be
 *  added, also at this stage allocating IDs for them in the
 *  destination database;
 *
 *  - Amend existing destination playlists to name our new entries
 *  (doing this first helps reduce the risk of orphans if the sync
 *  fails or is aborted);
 *
 *  - Add our new playlists;
 *
 *  - Add our new files (again, adding playlists before files reduces the
 *  risk of orphans).
 *
 */
unsigned int Synchroniser::Synchronise()
{
    Fire(&SyncObserver::OnStage, SyncObserver::BEGIN);

    m_srcids_seen.clear();
    m_any_deleted = false;

    /* Playlist 0x100 is 0x100 no matter what its name is */
    unsigned rc = CheckPlaylistNames(0x100, 0x100);
    if (rc)
	return rc;

    set_t done;
    for (set_t::const_iterator i = m_destids_maybe_delete.begin();
	 i != m_destids_maybe_delete.end();
	 ++i)
    {
	rc = MaybeDelete(*i, &done);
	if (rc)
	    return rc;
    }

    done.clear();
    for (set_t::const_iterator i = m_srcids_maybe_add.begin();
	 i != m_srcids_maybe_add.end();
	 ++i)
    {
	rc = MaybeAdd(*i, &done);
	if (rc)
	    return rc;
    }

    size_t n = m_srcids_amend_playlist.size();
    size_t j=0;
    for (set_t::const_iterator i = m_srcids_amend_playlist.begin();
	 i != m_srcids_amend_playlist.end();
	 ++i, ++j)
    {
	rc = AmendPlaylist(*i, j, n);
	if (rc)
	    return rc;
    }

    n = m_srcids_add_playlist.size();
    j=0;
    for (list_t::const_iterator i = m_srcids_add_playlist.begin();
	 i != m_srcids_add_playlist.end();
	 ++i, ++j)
    {
	rc = AddPlaylist(*i, j, n);
	if (rc)
	    return rc;
    }

    n = m_srcids_add_tune.size();
    j=0;
    for (set_t::const_iterator i = m_srcids_add_tune.begin();
	 i != m_srcids_add_tune.end();
	 ++i, ++j)
    {
	rc = AddTune(*i, j, n);
	if (rc)
	    return rc;
    }

    n = m_srcids_amend_metadata.size();
    j=0;
    for (set_t::const_iterator i = m_srcids_amend_metadata.begin();
	 i != m_srcids_amend_metadata.end();
	 ++i, ++j)
    {
	rc = AmendTune(*i, j, n);
	if (rc)
	    return rc;
    }

    Fire(&SyncObserver::OnStage, SyncObserver::END);
    return rc;
}

} // namespace mediadb
