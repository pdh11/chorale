#include "file_scanner.h"
#include "libutil/walker.h"
#include "libutil/trace.h"
#include "libutil/file.h"
#include "libdb/db.h"
#include "libmediadb/schema.h"
#include "libmediadb/allocate_id.h"
#include "playlist.h"
#include "tags.h"
#include "file_notifier.h"
#include <map>
#include <algorithm>
#include <boost/thread/recursive_mutex.hpp>

namespace import {


        /* FileScanner::Impl */


class FileScanner::Impl: public util::DirectoryWalker::Observer
{
    boost::recursive_mutex m_mutex;
    unsigned int m_count;
    unsigned int m_tunes;
    unsigned int m_hicount;
    uint64_t m_size;
    uint64_t m_hisize;
    uint64_t m_duration;
    std::string m_loroot;
    std::string m_hiroot;
    db::Database *m_db;
    util::TaskQueue *m_queue;
    FileNotifier *m_notifier;
    mediadb::AllocateID m_allocateid;

    typedef std::map<std::string, unsigned int> map_t;
    /** Sadly, we need to keep the path->ID map ourselves, as the DB
     * may be bottling-up transactions and doing them in one go. Still, this
     * is also faster, I guess.
     */
    map_t m_map;

    typedef std::map<unsigned int, std::vector<unsigned int> > children_t;
    /** Child vectors of all (outstanding) playlists or directories 
     */
    children_t m_children;

    friend class FileScanner;

    db::RecordsetPtr GetRecordForPath(const std::string& path, uint32_t *id);

public:
    Impl(const std::string& loroot, const std::string& hiroot,
	 db::Database *thedb, util::TaskQueue *queue, FileNotifier *notifier)
	: m_count(0), m_tunes(0), m_hicount(0),
	  m_size(0), m_hisize(0), m_duration(0),
	  m_loroot(loroot), m_hiroot(hiroot),
	  m_db(thedb), m_queue(queue), m_notifier(notifier),
	  m_allocateid(thedb) {}
    
    unsigned int OnFile(dircookie parent_cookie, unsigned int index, 
			const std::string& path, const std::string&,
			const struct stat*);
    unsigned int OnEnterDirectory(dircookie parent_cookie, unsigned int index, 
				  const std::string& path, const std::string&,
				  const struct stat *, dircookie *cookie_out);
    unsigned int OnLeaveDirectory(dircookie cookie,
				  const std::string& path, const std::string&);
    void OnFinished(unsigned int) {}

    unsigned int Count() const { return m_tunes; }
    unsigned int HiCount() const { return m_hicount; }

    uint64_t Duration() const { return m_duration; }
    uint64_t Size() const { return m_size; }
    uint64_t HiSize() const { return m_hisize; }
};

db::RecordsetPtr FileScanner::Impl::GetRecordForPath(const std::string& path,
						     uint32_t *id)
{
    boost::recursive_mutex::scoped_lock lock(m_mutex);

    db::QueryPtr qp = m_db->CreateQuery();
    qp->Restrict(mediadb::PATH, db::EQ, path);
    db::RecordsetPtr rs = qp->Execute();
    if (rs && !rs->IsEOF())
    {
	*id = rs->GetInteger(mediadb::ID);
	m_map[path] = *id;
    }
    else
    {
	// Not found
	*id = (path == m_loroot) ? 0x100 : m_allocateid.Allocate();
	rs = m_db->CreateRecordset();
	rs->AddRecord();
	rs->SetInteger(mediadb::ID, *id);
	rs->SetString(mediadb::PATH, path);
	rs->Commit();
	m_map[path] = *id;
    }
    return rs;
}

unsigned int FileScanner::Impl::OnFile(dircookie parent_cookie,
				       unsigned int index,
				       const std::string& path,
				       const std::string& leaf,
				       const struct stat *pst)
{
    uint32_t id;

//    TRACE << "OnFile(" << path << ")\n";

    bool seen_this_time = false;

    {
	boost::recursive_mutex::scoped_lock lock(m_mutex);

	map_t::iterator i = m_map.find(path);
	if (i != m_map.end())
	{
	    seen_this_time = true;
	    id = m_map[path];
	}
    }

    if (!seen_this_time)
    {
	db::RecordsetPtr rs = GetRecordForPath(path, &id);

	std::string extension = util::GetExtension(path.c_str());
	std::string flacname;
	if (extension == "mp3" || extension == "ogg")
	{
	    // Look for FLAC version
	    flacname = util::StripExtension(path.c_str()) + ".flac";
	    struct stat st;
	    if (stat(flacname.c_str(), &st) < 0)
	    {
		if (!m_hiroot.empty()
		    && !strncmp(path.c_str(), m_loroot.c_str(), 
				m_loroot.length()))
		{
		    flacname = m_hiroot
			+ std::string(flacname, m_loroot.length());
//			    TRACE << "mp3 " << path << " trying flac "
//				  << flacname << " \n";
		    if (stat(flacname.c_str(), &st) < 0)
			flacname.clear();
		}
		else
		    flacname.clear();
	    }

	    if (!flacname.empty())
	    {
		OnFile(0, 0, flacname,
		       util::GetLeafName(flacname.c_str()), &st);
	    }
	}

	// Has it changed since we last saw it?
	if (rs->GetInteger(mediadb::MTIME) != (unsigned)pst->st_mtime
	    || rs->GetInteger(mediadb::SIZEBYTES) != (unsigned)pst->st_size)
	{
	    // Default -- to be overriden later if poss
	    rs->SetInteger(mediadb::TYPE, mediadb::FILE);
	    rs->SetString(mediadb::TITLE, util::StripExtension(leaf.c_str()));
    
	    if (extension == "mp3" || extension == "ogg"
		|| extension == "flac")
	    {
		import::TagsPtr tags = import::Tags::Create(path);
	    
		unsigned int rc = tags->Read(rs);
		if (rc == 0)
		    rs->SetInteger(mediadb::TYPE,
				   (extension == "flac") ? mediadb::TUNEHIGH
				                         : mediadb::TUNE);

		if (extension != "flac" && !flacname.empty())
		{
		    boost::recursive_mutex::scoped_lock lock(m_mutex);
		    rs->SetInteger(mediadb::IDHIGH, m_map[flacname]);
//			TRACE << "flac(" << id << ")="
//			      << m_map[flacname] << "\n";
		}
	    }
	    else if (extension == "asx" || extension == "wpl")
	    {
		import::PlaylistPtr pp = import::Playlist::Create(path);
	    
		unsigned int rc = pp->Load();
		if (rc == 0)
		{
		    rs->SetInteger(mediadb::TYPE, mediadb::PLAYLIST);
		    size_t entries = pp->GetLength();
		    for (size_t i=0; i<entries; ++i)
		    {
			std::string relpath = pp->GetEntry(i);
			std::string abspath = util::MakeAbsolutePath(path,
								     relpath);
			abspath = util::Canonicalise(abspath);
		    
			// Don't link to dirs or playlists
			if (util::GetExtension(abspath.c_str()) == "asx")
			    continue;

			struct stat childst;
			if (::stat(abspath.c_str(), &childst) < 0)
			    continue;
			if (S_ISDIR(childst.st_mode)
			    || S_ISLNK(childst.st_mode))
			    continue;

			OnFile(id, i, abspath,
			       util::GetLeafName(abspath.c_str()), &childst);
		    }
	    
		    {
			boost::recursive_mutex::scoped_lock lock(m_mutex);
			std::vector<unsigned int>& vec = m_children[id];
			vec.erase(std::remove(vec.begin(), vec.end(), 0U), 
				  vec.end());
			rs->SetString(mediadb::CHILDREN, 
				      mediadb::VectorToChildren(vec));
		    }
		    rs->SetInteger(mediadb::TYPE, mediadb::PLAYLIST);
		}
	    }

	    rs->SetString(mediadb::PATH, path);
	    rs->SetInteger(mediadb::SIZEBYTES, pst->st_size);
	    rs->SetInteger(mediadb::ID, id);
	    rs->SetInteger(mediadb::IDPARENT, parent_cookie);
	    rs->Commit();
	}

	/* Stats */
	{
	    boost::recursive_mutex::scoped_lock lock(m_mutex);
	    
	    if (rs->GetInteger(mediadb::TYPE) == mediadb::TUNE)
	    {
		++m_tunes;
		if (rs->GetInteger(mediadb::IDHIGH))
		    ++m_hicount;
		else
		    m_hisize += pst->st_size;
	    
		m_duration += rs->GetInteger(mediadb::DURATIONMS);
		m_size += pst->st_size;
	    }
	    else if (rs->GetInteger(mediadb::TYPE) == mediadb::TUNEHIGH)
		m_hisize += pst->st_size;
	}
    }

    if (parent_cookie)
    {
	boost::recursive_mutex::scoped_lock lock(m_mutex);
	size_t oldsz = m_children[parent_cookie].size();
	if (index >= oldsz)
	    m_children[parent_cookie].resize(index+1);
	m_children[parent_cookie][index] = id;
    }

    return 0;
}

unsigned int FileScanner::Impl::OnEnterDirectory(dircookie parent_cookie,
						 unsigned int index, 
						 const std::string& path,
						 const std::string&,
						 const struct stat*,
						 dircookie *cookie_out)
{
//    TRACE << "Entering '" << path << "' ("
//	  << parent_cookie << ":" << index << ")\n";

    unsigned int id;
    if (path == m_loroot)
    {
	id = 0x100;
    }
    else
    {
	boost::recursive_mutex::scoped_lock lock(m_mutex);
	db::RecordsetPtr rs = GetRecordForPath(path, &id);
	rs->SetInteger(mediadb::IDPARENT, parent_cookie);
	rs->Commit();

	size_t oldsz = m_children[parent_cookie].size();
	if (index >= oldsz)
	    m_children[parent_cookie].resize(index+1);
	m_children[parent_cookie][index] = id;
    }

    *cookie_out = id;

    return 0; 
}

unsigned int FileScanner::Impl::OnLeaveDirectory(dircookie cookie,
						 const std::string& path,
						 const std::string& leaf)
{ 
//    TRACE << "Leaving '" << path << "' (" << cookie << ")\n";
    unsigned int id;
    db::RecordsetPtr rs = GetRecordForPath(path, &id);
    if (id != cookie)
    {
	TRACE << "Oh dear, path=" << path << " id=" << id << " cookie=" << cookie << "\n";
    }
    assert(id == cookie);
    rs->SetInteger(mediadb::TYPE, mediadb::DIR);
    rs->SetString(mediadb::TITLE, leaf);
    {
	boost::recursive_mutex::scoped_lock lock(m_mutex);
	std::vector<unsigned int>& vec = m_children[cookie];
	vec.erase(std::remove(vec.begin(), vec.end(), 0u), vec.end());
	rs->SetString(mediadb::CHILDREN, mediadb::VectorToChildren(vec));
    }
    rs->Commit();

    if (m_notifier)
	m_notifier->Watch(path.c_str());

    return 0;
}				 


        /* FileScanner */


FileScanner::FileScanner(const std::string& loroot, const std::string& hiroot,
			 db::Database *thedb, util::TaskQueue *queue,
			 FileNotifier *fn)
    : m_impl(new Impl(loroot, hiroot, thedb, queue, fn))
{
}

FileScanner::~FileScanner()
{
    delete m_impl;
}

unsigned int FileScanner::Scan()
{
    util::DirectoryWalker w(m_impl->m_loroot, m_impl, m_impl->m_queue);
    w.Start();
    w.WaitForCompletion();
//    TRACE << "Scanned " << m_impl->Count() << " files\n"; 
//    TRACE << m_impl->HiCount() << " MP3s have corresponding FLAC\n";
//    uint64_t s = m_impl->Duration()/1000;
//    uint64_t m = (s/60) % 60;
//    uint64_t h = (s/3600) % 24;
//    uint64_t d = (s/(3600*24));
//    TRACE << "Total duration ";
//    if (d)
//	TRACE << d << "d ";
//    TRACE << h << "h " << m << "m\n";
//    printf("Size (w/FLAC) %'llu (MP3) %'llu\n", 
//	   (long long unsigned)m_impl->HiSize(), 
//	   (long long unsigned)m_impl->Size());

    TRACE << "Cleaning up deletions\n";

    db::QueryPtr qp = m_impl->m_db->CreateQuery();
    qp->OrderBy(mediadb::PATH);
    db::RecordsetPtr rs = qp->Execute();
    Impl::map_t::const_iterator i = m_impl->m_map.begin();

    while (!rs->IsEOF() && i != m_impl->m_map.end())
    {
	std::string dbpath = rs->GetString(mediadb::PATH);
	std::string mappath = i->first;
	int discriminant = strcmp(dbpath.c_str(), mappath.c_str());
	if (discriminant > 0)
	{
	    // dbpath > mappath -- shouldn't happen
	    assert(discriminant <= 0);
	    ++i;
	}
	else if (discriminant < 0)
	{
	    // mappath > dbpath
	    TRACE << dbpath << " gone away, deleting\n";
	    rs->Delete();
	}
	else
	{
	    assert(discriminant == 0);
	    rs->MoveNext();
	    ++i;
	}
    }

    // Got to end of map (maybe db too); delete remaining db records
    while (!rs->IsEOF())
    {
	rs->Delete();
    }

    return 0;
}

}; // namespace import

#ifdef TEST

/** Note that tests are allowed to include anything -- modularity rules don't
 * apply to tests.
 */
# include "libdbsteam/db.h"
# include "libutil/worker_thread_pool.h"

int main()
{
    db::steam::Database sdb(mediadb::FIELD_COUNT);
    sdb.SetFieldInfo(mediadb::ID, 
		     db::steam::FIELD_INT|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::PATH,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);

    char root[] = "file_scanner.test.XXXXXX";

    if (!mkdtemp(root))
    {
	fprintf(stderr, "Can't create temporary dir\n");
	return 1;
    }

    std::string fullname = util::Canonicalise(root);

    util::WorkerThreadPool wtp(2);

    {
	import::FileScanner ifs(fullname, "", &sdb, wtp.GetTaskQueue());

	ifs.Scan();
    }
    
    /* Assert that the DB contains only the root */

    db::QueryPtr qp = sdb.CreateQuery();
    qp->OrderBy(mediadb::PATH);
    db::RecordsetPtr rs = qp->Execute();

    assert(!rs->IsEOF());
    assert(rs->GetString(mediadb::PATH) == fullname);
    assert(rs->GetInteger(mediadb::TYPE) == mediadb::DIR);
    assert(rs->GetInteger(mediadb::ID) == 0x100);
    rs->MoveNext();
    assert(rs->IsEOF());
    
    /* Make a file in the directory and rescan */

    std::string file = fullname + "/foo.txt";
    FILE *f = fopen(file.c_str(), "wb");
    fprintf(f, "frink\n");
    fclose(f);

    {
	import::FileScanner ifs(fullname, "", &sdb, wtp.GetTaskQueue());

	ifs.Scan();
    }

    /* Assert that the DB contains only the root and the new file */

    qp = sdb.CreateQuery();
    qp->OrderBy(mediadb::PATH);
    rs = qp->Execute();

    assert(!rs->IsEOF());
    assert(rs->GetString(mediadb::PATH) == fullname);
    assert(rs->GetInteger(mediadb::TYPE) == mediadb::DIR);
    assert(rs->GetInteger(mediadb::ID) == 0x100);
    rs->MoveNext();
    assert(!rs->IsEOF());
    assert(rs->GetString(mediadb::PATH) == file);
    assert(rs->GetInteger(mediadb::TYPE) == mediadb::FILE);
    unsigned int fileid = rs->GetInteger(mediadb::ID);
    assert(fileid != 0x100);
    assert(fileid != 0);
    rs->MoveNext();
    assert(rs->IsEOF());

    /* Make a link in the directory and rescan */

    std::string link = fullname + "/zachary.txt";
    symlink(file.c_str(), link.c_str());

    {
	import::FileScanner ifs(fullname, "", &sdb, wtp.GetTaskQueue());
	ifs.Scan();
    }

    /* Assert still only two records, but root has two children now */

    qp = sdb.CreateQuery();
    qp->OrderBy(mediadb::PATH);
    rs = qp->Execute();

    assert(!rs->IsEOF());
    assert(rs->GetString(mediadb::PATH) == fullname);
    assert(rs->GetInteger(mediadb::TYPE) == mediadb::DIR);
    assert(rs->GetInteger(mediadb::ID) == 0x100);
    std::vector<unsigned int> cv;
    mediadb::ChildrenToVector(rs->GetString(mediadb::CHILDREN), &cv);
    assert(cv.size() == 2);
    assert(cv[0] == fileid);
    assert(cv[1] == fileid);
    rs->MoveNext();
    assert(!rs->IsEOF());
    assert(rs->GetString(mediadb::PATH) == file);
    assert(rs->GetInteger(mediadb::TYPE) == mediadb::FILE);
    assert(rs->GetInteger(mediadb::ID) == fileid);
    rs->MoveNext();
    assert(rs->IsEOF());

    /* Make a subdir and rescan */

    std::string subdir = fullname + "/beyonc\xC3\xA9";
    mkdir(subdir.c_str(), 0755);

    {
	import::FileScanner ifs(fullname, "", &sdb, wtp.GetTaskQueue());
	ifs.Scan();
    }

    /* Assert subdir found */

    qp = sdb.CreateQuery();
    qp->OrderBy(mediadb::PATH);
    rs = qp->Execute();

    assert(!rs->IsEOF());
    assert(rs->GetString(mediadb::PATH) == fullname);
    assert(rs->GetInteger(mediadb::TYPE) == mediadb::DIR);
    assert(rs->GetInteger(mediadb::ID) == 0x100);
    cv.clear();
    mediadb::ChildrenToVector(rs->GetString(mediadb::CHILDREN), &cv);
    assert(cv.size() == 3);
    assert(cv[1] == fileid); // Don't know dir's id yet
    assert(cv[2] == fileid);
    rs->MoveNext();
    assert(!rs->IsEOF());
    assert(rs->GetString(mediadb::PATH) == subdir);
    assert(rs->GetInteger(mediadb::TYPE) == mediadb::DIR);
    unsigned int dirid = rs->GetInteger(mediadb::ID);
    assert(dirid != 0x100);
    assert(dirid != 0);
    assert(dirid != fileid);
    rs->MoveNext();
    assert(!rs->IsEOF());
    assert(rs->GetString(mediadb::PATH) == file);
    assert(rs->GetInteger(mediadb::TYPE) == mediadb::FILE);
    assert(rs->GetInteger(mediadb::ID) == fileid);
    rs->MoveNext();
    assert(rs->IsEOF());

    /* Remove the subdir again and rescan */

    TRACE << "deleting " << subdir << "\n";
    rmdir(subdir.c_str());
    {
	import::FileScanner ifs(fullname, "", &sdb, wtp.GetTaskQueue());
	ifs.Scan();
    }

    /* Assert subdir not found */

    qp = sdb.CreateQuery();
    qp->OrderBy(mediadb::PATH);
    rs = qp->Execute();

    assert(!rs->IsEOF());
    assert(rs->GetString(mediadb::PATH) == fullname);
    assert(rs->GetInteger(mediadb::TYPE) == mediadb::DIR);
    assert(rs->GetInteger(mediadb::ID) == 0x100);
    cv.clear();
    mediadb::ChildrenToVector(rs->GetString(mediadb::CHILDREN), &cv);
    assert(cv.size() == 2);
    assert(cv[0] == fileid);
    assert(cv[1] == fileid);
    rs->MoveNext();
    assert(!rs->IsEOF());
    assert(rs->GetString(mediadb::PATH) == file);
    assert(rs->GetInteger(mediadb::TYPE) == mediadb::FILE);
    assert(rs->GetInteger(mediadb::ID) == fileid);
    rs->MoveNext();
    assert(rs->IsEOF());

    /* Tidy up */

    std::string rmrf = "rm -r " + fullname;
    system(rmrf.c_str());

    return 0;
}

#endif
