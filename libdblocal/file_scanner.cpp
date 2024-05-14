#include "file_scanner.h"
#include "config.h"
#include "libutil/walker.h"
#include "libutil/trace.h"
#include "libutil/file.h"
#include "libutil/http.h"
#include "libutil/observable.h"
#include "libutil/counted_pointer.h"
#include "libdb/query.h"
#include "libdb/recordset.h"
#include "libmediadb/schema.h"
#include "libmediadb/db.h"
#include "libimport/playlist.h"
#include "libimport/tags.h"
#include "libimport/file_notifier.h"
#include <map>
#include <algorithm>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <mutex>
#include <condition_variable>

#if HAVE_AVFORMAT
extern "C" {
#include <libavformat/avformat.h>
}
#endif

LOG_DECL(DBLOCAL);

namespace db {
namespace local {


        /* FileScanner::Impl */


class FileScanner::Impl: public util::DirectoryWalker::Observer,
			 public util::Observable<FileScanner::Observer>
{
    std::mutex m_mutex;
    std::condition_variable m_finished;
    unsigned int m_count;
    unsigned int m_tunes;
    unsigned int m_hicount;
    uint64_t m_size;
    uint64_t m_hisize;
    uint64_t m_duration;
    std::string m_loroot;
    std::string m_hiroot;
    db::Database *m_db;
    mediadb::Database *m_idallocator;
    util::TaskQueue *m_queue;
    import::FileNotifierTask *m_notifier;

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

    bool m_scanning;
    unsigned int m_error;

    db::RecordsetPtr GetRecordForPath(const std::string& path, uint32_t *id);

    friend class FileScanner;

public:
    Impl(const std::string& loroot, const std::string& hiroot,
	 db::Database *thedb, mediadb::Database *idallocator,
	 util::TaskQueue *queue, import::FileNotifierTask *notifier)
	: m_count(0), m_tunes(0), m_hicount(0),
	  m_size(0), m_hisize(0), m_duration(0),
	  m_db(thedb),
	  m_idallocator(idallocator),
	  m_queue(queue),
	  m_notifier(notifier),
	  m_scanning(false),
          m_error(0)
    {
	m_loroot = util::Canonicalise(loroot);
	if (!hiroot.empty())
	    m_hiroot = util::Canonicalise(hiroot);
    }
    
    unsigned int OnFile(dircookie parent_cookie, unsigned int index, 
			const std::string& path, const std::string&,
			const struct stat*);
    unsigned int OnEnterDirectory(dircookie parent_cookie, unsigned int index, 
				  const std::string& path, const std::string&,
				  const struct stat *, dircookie *cookie_out);
    unsigned int OnLeaveDirectory(dircookie cookie,
				  const std::string& path, const std::string&,
				  const struct stat*);
    void OnFinished(unsigned int);

    unsigned int Count() const { return m_tunes; }
    unsigned int HiCount() const { return m_hicount; }

    uint64_t Duration() const { return m_duration; }
    uint64_t Size() const { return m_size; }
    uint64_t HiSize() const { return m_hisize; }

    unsigned int StartScan();
    unsigned int WaitForCompletion();
};

db::RecordsetPtr FileScanner::Impl::GetRecordForPath(const std::string& path,
						     uint32_t *id)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    db::QueryPtr qp = m_db->CreateQuery();
    qp->Where(qp->Restrict(mediadb::PATH, db::EQ, path));
    db::RecordsetPtr rs = qp->Execute();
    if (rs && !rs->IsEOF())
    {
	*id = rs->GetInteger(mediadb::ID);
	m_map[path] = *id;
    }
    else
    {
	// Not found
	*id = (path == m_loroot) ? 0x100 : m_idallocator->AllocateID();
	rs = m_db->CreateRecordset();
	rs->AddRecord();
	rs->SetInteger(mediadb::ID, *id);
	rs->SetString(mediadb::PATH, path);
	rs->SetInteger(mediadb::TYPE, mediadb::PENDING);
	rs->Commit();
	m_map[path] = *id;

	LOG(DBLOCAL) << "id " << *id << " is " << path << "\n";
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

    unsigned int rc = Fire(&FileScanner::Observer::OnFile, path);
    if (rc)
	return rc;

    bool seen_this_time = false;

    {
	std::lock_guard<std::mutex> lock(m_mutex);

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
	    // Defaults -- to be overriden later where needed
	    rs->SetInteger(mediadb::TYPE, mediadb::FILE);
	    rs->SetString(mediadb::TITLE, util::StripExtension(leaf.c_str()));
	    rs->SetString(mediadb::PATH, path);
	    rs->SetInteger(mediadb::SIZEBYTES, (unsigned int)pst->st_size);
	    rs->SetInteger(mediadb::MTIME, (unsigned int)pst->st_mtime);
	    rs->SetInteger(mediadb::ID, id);
	    rs->SetInteger(mediadb::IDPARENT, (unsigned int)parent_cookie);
    
	    if (extension == "mp3" || extension == "mp2" || extension == "ogg"
		|| extension == "flac")
	    {
		import::TagReader tags;
		rc = tags.Init(path);
		if (rc == 0)
		    rc = tags.Read(rs.get());
		if (rc == 0)
		    rs->SetInteger(mediadb::TYPE,
				   (extension == "flac") ? mediadb::TUNEHIGH
				                         : mediadb::TUNE);

		if (extension != "flac" && !flacname.empty())
		{
		    std::lock_guard<std::mutex> lock(m_mutex);
		    rs->SetInteger(mediadb::IDHIGH, m_map[flacname]);
//			TRACE << "flac(" << id << ")="
//			      << m_map[flacname] << "\n";
		}
	    }
	    else if (extension == "mp4" || extension == "mpg"
                     || extension == "avi" || extension == "mkv"
                     || extension == "vob")
	    {
		rs->SetInteger(mediadb::TYPE, mediadb::VIDEO);
		if (extension == "mp4")
		    rs->SetInteger(mediadb::CONTAINER, mediadb::MP4);
		else if (extension == "avi")
		    rs->SetInteger(mediadb::CONTAINER, mediadb::AVI);
		else if (extension == "mkv")
		    rs->SetInteger(mediadb::CONTAINER, mediadb::MATROSKA);
		else if (extension == "vob")
		    rs->SetInteger(mediadb::CONTAINER, mediadb::VOB);
		else
		    rs->SetInteger(mediadb::CONTAINER, mediadb::MPEGPS);

#if HAVE_AVFORMAT
                // https://stackoverflow.com/questions/6451814/how-to-use-libavcodec-ffmpeg-to-find-duration-of-video-file
                AVFormatContext* pFormatCtx = avformat_alloc_context();
                avformat_open_input(&pFormatCtx, path.c_str(), NULL, NULL);
                avformat_find_stream_info(pFormatCtx,NULL);
                // avformat duration is in microseconds, we want milliseconds
                rs->SetInteger(mediadb::DURATIONMS,
                               (uint32_t)(pFormatCtx->duration/1000));
                avformat_close_input(&pFormatCtx);
                avformat_free_context(pFormatCtx);
#else
                rs->SetInteger(mediadb::DURATIONMS, 5*60*1000);
#endif
	    }
	    else if (extension == "jpg" || extension == "jpeg")
	    {
		rs->SetInteger(mediadb::TYPE, mediadb::IMAGE);
		rs->SetInteger(mediadb::CONTAINER, mediadb::JPEG);
	    }
	    else // Is it a playlist?
	    {
		import::Playlist p;
		std::list<std::string> entries;

		if (p.Init(path) == 0
		    && p.Load(&entries) == 0)
		{
		    bool is_radio = false;

		    unsigned int n=0;
		    for (std::list<std::string>::const_iterator i = entries.begin();
			 i != entries.end();
			 ++i)
		    {
			if (i == entries.begin() && util::http::IsHttpURL(*i))
			{
			    is_radio = true;
			    break;
			}

			std::string abspath = util::Canonicalise(*i);
		    
			// Don't link to dirs or playlists
			import::Playlist childplaylist;
			if (childplaylist.Init(abspath) == 0)
			    continue;

			struct stat childst;
			if (::stat(abspath.c_str(), &childst) < 0)
			    continue;
			if (S_ISDIR(childst.st_mode))
			    continue;
#ifdef S_ISLNK
			if (S_ISLNK(childst.st_mode))
			    continue;
#endif
			OnFile(id, n++, abspath,
			       util::GetLeafName(abspath.c_str()), &childst);
		    }
	    
		    if (is_radio)
		    {
			rs->SetInteger(mediadb::TYPE, mediadb::RADIO);
			// We don't know for sure that it's MP3, but seeing
			// as we can't play any other types, we might as well
			// assume...
			rs->SetInteger(mediadb::AUDIOCODEC, mediadb::MP3);
			rs->SetInteger(mediadb::SAMPLERATE, 44100);
			rs->SetInteger(mediadb::BITSPERSEC, 128000);
			// Not UINT_MAX as real Receivers can't deal with it
			rs->SetInteger(mediadb::SIZEBYTES, INT_MAX);
			rs->SetString(mediadb::ARTIST, "Internet Radio");
		    }
		    else
		    {
			std::lock_guard<std::mutex> lock(m_mutex);
			std::vector<unsigned int>& vec = m_children[id];
			vec.erase(std::remove(vec.begin(), vec.end(), 0U), 
				  vec.end());
			rs->SetString(mediadb::CHILDREN, 
				      mediadb::VectorToChildren(vec));
			rs->SetInteger(mediadb::TYPE, mediadb::PLAYLIST);
		    }
		}
	    }

	    rs->Commit();
	}
    }

    if (parent_cookie)
    {
	std::lock_guard<std::mutex> lock(m_mutex);
	size_t oldsz = m_children[(unsigned int)parent_cookie].size();
	if (index >= oldsz)
	    m_children[(unsigned int)parent_cookie].resize(index+1);
	m_children[(unsigned int)parent_cookie][index] = id;
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
	db::RecordsetPtr rs = GetRecordForPath(path, &id);
	rs->SetInteger(mediadb::IDPARENT, (unsigned int)parent_cookie);
	rs->Commit();

	std::lock_guard<std::mutex> lock(m_mutex);
	size_t oldsz = m_children[(unsigned int)parent_cookie].size();
	if (index >= oldsz)
	    m_children[(unsigned int)parent_cookie].resize(index+1);
	m_children[(unsigned int)parent_cookie][index] = id;
    }

    *cookie_out = id;

    return 0; 
}

unsigned int FileScanner::Impl::OnLeaveDirectory(dircookie cookie,
						 const std::string& path,
						 const std::string& leaf,
						 const struct stat*)
{ 
//    TRACE << "Leaving '" << path << "' (" << cookie << ")\n";
    unsigned int id;
    db::RecordsetPtr rs = GetRecordForPath(path, &id);
    if (id != cookie)
    {
	TRACE << "Oh dear, path=" << path << " id=" << id << " cookie=" << cookie << "\n";

//	FILE *f = fopen("db.borked.xml", "w");
//	mediadb::WriteXML(m_db, mediadb::SCHEMA_VERSION, f);
//	fclose(f);
//	assert(id == cookie);
    }
    assert(id == cookie);
    rs->SetInteger(mediadb::TYPE, mediadb::DIR);
    rs->SetString(mediadb::TITLE, leaf);
    {
	std::lock_guard<std::mutex> lock(m_mutex);
	std::vector<unsigned int>& vec = m_children[(unsigned int)cookie];
	vec.erase(std::remove(vec.begin(), vec.end(), 0u), vec.end());
	rs->SetString(mediadb::CHILDREN, mediadb::VectorToChildren(vec));
    }
    rs->Commit();

    if (m_notifier)
	m_notifier->Watch(path.c_str());

    return 0;
}				 

unsigned int FileScanner::Impl::StartScan()
{
    assert(!m_scanning);

    /* Deal with changed root. Both the ID and the PATH should be unique in
     * the database, so, as we force the root path to have id BROWSE_ROOT, on
     * a change of root we need to delete the previous record for BROWSE_ROOT
     * and, if the new root path already has a record, change its id to
     * BROWSE_ROOT.
     *
     * Everything else, including deleting records no longer accessible through
     * the new root, will be taken care of in the scan.
     */
    db::QueryPtr qp = m_db->CreateQuery();
    qp->Where(qp->Restrict(mediadb::ID, db::EQ, mediadb::BROWSE_ROOT));
    db::RecordsetPtr rs = qp->Execute();
    if (rs && !rs->IsEOF())
    {
	if (rs->GetString(mediadb::PATH) != m_loroot)
	{
	    TRACE << "Root changed\n";
	    rs->Delete();

	    qp = m_db->CreateQuery();
	    qp->Where(qp->Restrict(mediadb::PATH, db::EQ, m_loroot));
	    rs = qp->Execute();
	    if (rs && !rs->IsEOF())
	    {
		rs->SetInteger(mediadb::ID, mediadb::BROWSE_ROOT);
		rs->Commit();
	    }
	}
    }

    m_map.clear();
    m_children.clear();
    m_scanning = true;
    m_error = 0;

    return util::DirectoryWalker::Walk(m_loroot, this, m_queue);
}

void FileScanner::Impl::OnFinished(unsigned int error)
{
    if (!error)
    {
	db::QueryPtr qp = m_db->CreateQuery();
	qp->OrderBy(mediadb::PATH);
	db::RecordsetPtr rs = qp->Execute();
	map_t::const_iterator i = m_map.begin();
	
	while (!rs->IsEOF() && i != m_map.end())
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
//	    TRACE << dbpath << " gone away, deleting\n";
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
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_finished.notify_all();
    m_scanning = false;
    m_error = error;

    Fire(&FileScanner::Observer::OnFinished, error);
}

unsigned int FileScanner::Impl::WaitForCompletion()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    while (m_scanning)
    {
	m_finished.wait_for(lock, std::chrono::seconds(60));
    }
    return m_error;
}


        /* FileScanner */


FileScanner::FileScanner(const std::string& loroot, const std::string& hiroot,
			 db::Database *thedb, mediadb::Database *idallocator,
			 util::TaskQueue *queue,
			 import::FileNotifierTask *fn)
    : m_impl(new Impl(loroot, hiroot, thedb, idallocator, queue, fn))
{
}

FileScanner::~FileScanner()
{
    delete m_impl;
}

unsigned int FileScanner::StartScan()
{
    return m_impl->StartScan();
}

unsigned int FileScanner::WaitForCompletion()
{
    return m_impl->WaitForCompletion();
}

unsigned int FileScanner::Scan()
{
    int rc = StartScan();
    if (rc)
	return rc;
    return WaitForCompletion();
}

void FileScanner::AddObserver(Observer *obs)
{
    return m_impl->AddObserver(obs);
}

} // namespace db::local
} // namespace db

#ifdef TEST

/** Note that tests are allowed to include anything -- modularity rules don't
 * apply to tests.
 */
# include <errno.h>
# include "db.h"
# include "libmediadb/xml.h"
# include "libdbsteam/db.h"
# include "libutil/worker_thread_pool.h"
# include "libutil/http_client.h"

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

    util::WorkerThreadPool wtp(util::WorkerThreadPool::NORMAL, 2);

    util::http::Client client;
    db::local::Database ldb(&sdb, &client);

    {
	db::local::FileScanner ifs(fullname, "", &sdb, &ldb, &wtp);

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
	db::local::FileScanner ifs(fullname, "", &sdb, &ldb, &wtp);

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
    if (::symlink(file.c_str(), link.c_str()) < 0)
    {
	fprintf(stderr, "Can't create symlink: %d\n", errno);
	return 1;
    }

    {
	db::local::FileScanner ifs(fullname, "", &sdb, &ldb, &wtp);
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
	db::local::FileScanner ifs(fullname, "", &sdb, &ldb, &wtp);
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
	db::local::FileScanner ifs(fullname, "", &sdb, &ldb, &wtp);
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
    if (system(rmrf.c_str()) < 0)
    {
	fprintf(stderr, "Can't tidy up: %d\n", errno);
	return 1;
    }
    return 0;
}

#endif
