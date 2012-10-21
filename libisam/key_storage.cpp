#include "key_storage.h"
#include "format.h"
#include "file.h"
#include "page_allocator.h"
#include "page_locking.h"
#include "direct_page_handler.h"
#include "libutil/trace.h"
#include "libutil/errors.h"
#include "libutil/counted_pointer.h"
#include <sys/time.h>
#include <string.h>
#include <stdio.h>

namespace isam {

KeyStorage::KeyStorage(File *file, PageAllocator *allocator, 
		       PageLocking *locking)
    : m_file(file),
      m_allocator(allocator),
      m_locking(locking),
      m_ok(false)
{
}

unsigned int KeyStorage::InitialiseFile()
{
    bool new_file = false;

    {
	ReadLock lock(m_locking, 0);

	const SuperBlock *superblock = (const SuperBlock*)lock.Lock();
	if (!superblock)
	    return EINVAL;

	if (!superblock->format && !superblock->version)
	{
	    new_file = true;
	}
	else
	{
	    if (superblock->format != FORMAT
		|| superblock->version != VERSION)
		return EINVAL;
	    m_ok = true;
	}
    }

    if (new_file)
    {
	WriteLock lock(m_locking, 0);
	SuperBlock *superblock = (SuperBlock*)lock.Lock();
	if (!superblock)
	    return EINVAL;

	superblock->format = FORMAT;
	superblock->version = VERSION;
	superblock->root_data_page = 0;
	superblock->bogus_shutdown = (unsigned int)-1;
	m_ok = true;
    }

    return 0;
}

static const DirectPageHandler s_dph;

const KeyStorage::PageBinding KeyStorage::sm_page_bindings[] = {
    { DIRECT_KEY, &s_dph },
//    { FANOUT_KEY, &s_fph }
};

const PageHandler *KeyStorage::Handler(unsigned int type)
{
    for (unsigned int i=0; 
	 i<sizeof(sm_page_bindings)/sizeof(sm_page_bindings[0]); 
	 ++i)
    {
	if (type == sm_page_bindings[i].type)
	    return sm_page_bindings[i].handler;
    }
    return NULL;
}

unsigned int KeyStorage::ReplacePageNumberInBlock(DataBlock *block, 
						  unsigned int old_pageno,
						  unsigned int new_pageno)
{
    if (block->type == SUPERBLOCK)
    {
	SuperBlock *superblock = (SuperBlock*)block;
	if (superblock->root_data_page == old_pageno)
	    superblock->root_data_page = new_pageno;
	return 0;
    }
    const PageHandler *handler = Handler(block->type);
    if (!handler)
    {
	TRACE << "Don't understand block type " << block->type << "\n";
	return EINVAL;
    }
    handler->ReplacePageNumberInBlock(block, old_pageno, new_pageno);
    return 0;
}

template <class LockChain>
unsigned int KeyStorage::FindPage(LockChain *chain,
				  Key *key /* INOUT */,
				  std::string *direct_value,
				  uint32_t *value_page)
{
    uint32_t page = 0;
    unsigned int rc = chain->Add(page, 1);
    if (rc)
	return rc;

    PageResult pr;

    *value_page = (uint32_t)-1;

    do {
	assert(page == chain->GetChildPage());

	unsigned int siblings;
	const DataBlock *block = (const DataBlock*)m_file->Page(page);

	if (block->type == SUPERBLOCK)
	{
	    const SuperBlock *superblock = (const SuperBlock*)block;
	    if (superblock->root_data_page)
		page = superblock->root_data_page;
	    else
		return ENOENT;

	    siblings = 1;
	}
	else
	{
	    const PageHandler *handler = Handler(block->type);
	    if (!handler)
	    {
		TRACE << "Don't understand block type " << block->type << "\n";
		return EINVAL;
	    }

	    rc = handler->Find(block, key, &pr);
	    if (rc)
		return rc; // Error

	    if (pr.value_page != (uint32_t)-1)
	    {
		*value_page = pr.value_page;
		return 0;
	    }
	    if (pr.key_page == (uint32_t)-1)
	    {
		*direct_value = pr.direct_value;
		return 0;
	    }
	    
//	    TRACE << "Carrying on with reduced key '" << key->ToString()
//		  << "'\n";

	    siblings = block->count;
	    page = pr.key_page;
	    // else page is a key page, carry on
	}

	rc = chain->Add(page, siblings);
    } while (!rc);

    return rc;
}

unsigned int KeyStorage::Store(const std::string& skey, 
			       const std::string& value)
{
    WriteLockChain wlc(m_locking);

    Key key(skey);
    std::string direct_value;
    unsigned int value_page = 0;
    unsigned int rc = FindPage(&wlc, &key, &direct_value, &value_page);
    if (rc && rc != ENOENT)
	return rc;

    unsigned int old_page = wlc.GetChildPage();
    DataBlock *block = (DataBlock*)m_file->Page(old_page);
    unsigned int new_page;

    page_contents_t contents;
    
    if (block->type == SUPERBLOCK)
    {
	// This is the very first key -- treat specially
	contents[key.ToString()] = PageResult(value,0,0);
	rc = s_dph.ComposePage(contents, m_allocator, m_file, &new_page);
	if (rc)
	    return rc;
	ReplacePageNumberInBlock(block, 0, new_page);
	return 0;
    }

    const PageHandler *handler = Handler(block->type);
    if (!handler)
    {
	TRACE << "Don't understand block type " << block->type << "\n";
	return EINVAL;
    }

    rc = handler->DecomposePage(block, &contents);
    if (rc)
	return rc;

    //TRACE << "Whole key '" << skey << "' page key '" << key << "'\n";

    /** @todo Tidy up old value: remove value page, etc. */

    contents[key.ToString()] = PageResult(value,0,0);
	
    rc = s_dph.ComposePage(contents, m_allocator, m_file, &new_page);
    if (rc)
	return rc;
    
    ReplacePageNumberInBlock((DataBlock*)m_file->Page(wlc.GetParentPage()),
			     old_page, new_page);

    m_allocator->Free(old_page);

    return 0;
}

unsigned int KeyStorage::Fetch(const std::string& skey, std::string *pValue)
{
    ReadLockChain rlc(m_locking);

    Key key(skey);
    unsigned int value_page = 0;
    unsigned int rc = FindPage(&rlc, &key, pValue, &value_page);
    if (rc)
	return rc;

    if (value_page != (unsigned int)-1)
    {
	TRACE << "Don't understand indirect value\n";
	return EINVAL;
    }

    return 0;
}

unsigned int KeyStorage::Delete(const std::string& skey)
{
//    TRACE << "Deleting '" << skey << "'\n";

    DeleteLockChain dlc(m_locking);
    Key key(skey);
    
    std::string direct_value;
    unsigned int value_page = 0;
    unsigned int rc = FindPage(&dlc, &key, &direct_value, &value_page);
    if (rc)
    {
//	TRACE << "Delendum not found\n";
	return rc;
    }
    
    // @todo Free value page if any

    unsigned int old_page;
    DataBlock *block;
    unsigned int deleted_page = 0;

    for (;;) {
	old_page = dlc.GetChildPage();
	block = (DataBlock*)m_file->Page(old_page);

	if (block->count > 1 || dlc.Count() == 1)
	    break;

	dlc.ReleaseChildPage();
//	TRACE << "Discarding page " << old_page << " altogether\n";
	m_allocator->Free(old_page);
	deleted_page = old_page;
    }

    if (dlc.Count() == 1)
    {
	// Deleted everything

	TRACE << "Deleted everything\n";
	SuperBlock *sb = (SuperBlock*)m_file->Page(0);
	sb->root_data_page = 0;
	return 0;
    }

    while (dlc.Count() > 2)
	dlc.ReleaseParentPage();

    page_contents_t contents;

    rc = s_dph.DecomposePage(block, &contents);

/*
    TRACE << "initial contents\n";
    for (page_contents_t::iterator i = contents.begin();
	 i != contents.end();
	 ++i)
    {
	if (i->second.key_page)
	    TRACE << " " << i->first << " -> " << i->second.key_page << "\n";
	else
	    TRACE << " " << i->first << "\n";
    }
*/

    if (deleted_page)
    {
//	TRACE << "Rewriting page " << old_page << " not to reference "
//	      << deleted_page << "\n";
	bool found = false;
	for (page_contents_t::iterator i = contents.begin();
	     i != contents.end();
	     ++i)
	{
	    if (i->second.key_page == deleted_page)
	    {
		found = true;
		contents.erase(i);
		break;
	    }
	}
	if (!found)
	{
	    TRACE << "Can't find reference to " << deleted_page << " in "
		  << old_page << "\n";
	}
	assert(found);
    }
    else
    {
//	TRACE << "Rewriting page " << old_page << " not to contain '"
//	      << key.ToString() << "'\n";
	contents.erase(key.ToString());

	assert(!contents.empty());
    }
	
/*
    TRACE << "final contents\n";
    for (page_contents_t::iterator i = contents.begin();
	 i != contents.end();
	 ++i)
    {
	if (i->second.key_page)
	    TRACE << " " << i->first << " -> " << i->second.key_page << "\n";
	else
	    TRACE << " " << i->first << "\n";
    }
*/

    unsigned int new_page;
    rc = s_dph.ComposePage(contents, m_allocator, m_file, &new_page);
    if (rc)
	return rc;

//    TRACE << "Rewriting old " << old_page << " as new " << new_page
//	  << " in parent " << dlc.GetParentPage() << "\n";

/*
    if (dlc.GetParentPage())
    {
	s_dph.DecomposePage((DataBlock*)m_file->Page(dlc.GetParentPage()),
			    &contents);
	
	TRACE << "initial contents of *parent*\n";
	for (page_contents_t::iterator i = contents.begin();
	     i != contents.end();
	     ++i)
	{
	    if (i->second.key_page)
		TRACE << " " << i->first << " -> " << i->second.key_page << "\n";
	    else
		TRACE << " " << i->first << "\n";
	}
    }
*/
    
    ReplacePageNumberInBlock((DataBlock*)m_file->Page(dlc.GetParentPage()),
			     old_page, new_page);

	/*
    if (dlc.GetParentPage())
    {
	rc = s_dph.DecomposePage((DataBlock*)m_file->Page(dlc.GetParentPage()),
				 &contents);
	TRACE << "final contents of *parent*\n";
	for (page_contents_t::iterator i = contents.begin();
	     i != contents.end();
	     ++i)
	{
	    if (i->second.key_page)
		TRACE << " " << i->first << " -> " << i->second.key_page << "\n";
	    else
		TRACE << " " << i->first << "\n";
	}
    }
	*/

    m_allocator->Free(old_page);

    return 0;
}

void KeyStorage::Dump(unsigned int page, const std::string& prefix) const
{
    const DataBlock *block = (const DataBlock*)m_file->Page(page);
    if (!block)
    {
	TRACE << "*** Dump can't read page " << page << "\n";
	return;
    }
    printf("%s (%u):\n", prefix.c_str(), page);

    if (block->type == SUPERBLOCK)
    {
	assert(prefix.empty());
	const SuperBlock *sb = (const SuperBlock*)block;
	Dump(sb->root_data_page, "");
	return;
    }

    page_contents_t contents;

    const PageHandler *handler = Handler(block->type);
    if (!handler)
    {
	TRACE << "*** Don't understand block type " << block->type << "\n";
	return;
    }

    unsigned int rc = handler->DecomposePage(block, &contents);
    if (rc)
    {
	TRACE << "*** Can't decompose page " << page << ": " << rc << "\n";
	return;
    }

    for (page_contents_t::const_iterator i = contents.begin();
	 i != contents.end();
	 ++i)
    {
	if (i->second.value_page)
	    printf("%s:%s => [%u]\n", prefix.c_str(), i->first.c_str(), 
		   i->second.value_page);
	else if (i->second.key_page)
	{
	    printf("%s:%s => %u\n", prefix.c_str(), i->first.c_str(),
		   i->second.key_page);
	    Dump(i->second.key_page, prefix + i->first);
	}
	else
	    printf("%s:%s => %s\n", prefix.c_str(), i->first.c_str(), 
		   i->second.direct_value.c_str());
    }
}

KeyStorage::~KeyStorage()
{
    if (m_ok)
    {
	WriteLock lock(m_locking, 0);

	SuperBlock *superblock = (SuperBlock*)lock.Lock();
	if (superblock)
	    superblock->bogus_shutdown = 0;
    }
}

} // namespace isam


#ifdef TEST

#define BERKELEY 0

# include <boost/format.hpp>
# include "libutil/worker_thread_pool.h"
#if BERKELEY
# include <fcntl.h>
# include <db.h>

class Berkeley
{
    DB *m_db;

public:
    Berkeley()
    {
	BTREEINFO info;
	memset(&info, '\0', sizeof(info));
	info.psize = 4096;
	info.maxkeypage = 4096;
	info.cachesize = 10*1024*1024;
	m_db = dbopen("berkeley.isam", O_CREAT|O_RDWR, 0600, DB_BTREE, &info);
    }

    ~Berkeley()
    {
	m_db->close(m_db);
    }

    unsigned int Store(const std::string& key, const std::string& value)
    {
	DBT k,v;
	k.data = key.c_str();
	k.size = key.size();
	v.data = value.c_str();
	v.size = value.size();
	int rc = m_db->put(m_db, &k, &v, 0);
	if (rc<0)
	    return (unsigned int)errno;
	return 0;
    }

    unsigned int Fetch(const std::string& key, std::string *pvalue)
    {
	DBT k,v;
	k.data = key.c_str();
	k.size = key.size();

	int rc = m_db->get(m_db, &k, &v, 0);
	if (rc<0)
	    return (unsigned int)errno;
	if (rc==1)
	    return ENOENT;
	*pvalue = std::string((const char*)v.data, 
			      ((const char*)v.data) + v.size);
	return 0;
    }
};
#endif

const char vowels[] = "aeiouyaeiouaeiou";
const char consonants[] = "bcdefghjklmnpqrstvwxzbcdfgjklmnprstvwbcdfgjklmnprst";

std::string RandomString()
{
    std::string result;

    if (rand() & 1)
	goto vowel;

consonant:
    result += consonants[rand() % strlen(consonants)];
    if (rand() & 1)
	result += consonants[rand() % strlen(consonants)];
    if (!(rand() % 20))
	return result;

vowel:
    result += vowels[rand() % strlen(vowels)];
    if (!(rand() % 40))
	return result;
    goto consonant;
}

util::Mutex s_mutex;
util::Condition s_finished;
unsigned int s_nfinished = 0;
uint64_t s_total = 0;

enum {
    NUM_REQUESTS = 100000,
    NUM_THREADS = 100
};

typedef std::map<std::string, std::string> map_t;

class KeyTestTask: public util::Task
{
    isam::KeyStorage *m_storage;
    unsigned int m_which;

public:
    KeyTestTask(isam::KeyStorage *storage, unsigned int which);
    unsigned int Run();
};

KeyTestTask::KeyTestTask(isam::KeyStorage *storage, unsigned int which)
    : m_storage(storage),
      m_which(which)
{
}

unsigned int KeyTestTask::Run()
{
    std::string suffix = (boost::format("-%u") % m_which).str();
    map_t mymap;

    time_t start = time(NULL);
    unsigned int done = 0;

    do {
	std::string key = RandomString() + suffix;
	std::string value = RandomString();
	unsigned int rc = m_storage->Store(key, value);
	assert(rc == 0);
	mymap[key] = value;

	for (map_t::const_iterator j = mymap.begin(); j != mymap.end(); ++j)
	{
	    rc = m_storage->Fetch(j->first, &value);
	    if (rc)
	    {
		fprintf(stderr, "Can't fetch %s: %u\n", j->first.c_str(), rc);
		exit(1);
	    }
	    assert(rc == 0);
	    if (value != j->second)
	    {
		fprintf(stderr, "Key '%s' expected '%s' got '%s'\n",
			j->first.c_str(), j->second.c_str(), value.c_str());
		m_storage->Dump(0,"");
		exit(1);
	    }
	    assert(value == j->second);
	}
	++done;
    } while ((time(NULL) - start) < 20);

//    TRACE << "Did " << done << "\n";

    util::Mutex::Lock lock(s_mutex);
    s_total += done;
    ++s_nfinished;
    s_finished.NotifyOne();
    return 0;
}

class Timer
{
    struct timeval m_start;

public:
    Timer()
    {
	::gettimeofday(&m_start, 0);
    }
    ~Timer()
    {
	struct timeval end;
	::gettimeofday(&end, 0);
	uint64_t start_usec = m_start.tv_usec + 1000000ull*m_start.tv_sec;
	uint64_t end_usec = end.tv_usec + 1000000ull*end.tv_sec;
	uint64_t usec = end_usec - start_usec;
	printf("%u.%06us\n", (unsigned int)(usec/1000000),
	       (unsigned int)(usec%1000000));
    }
};

int main()
{
    {
	isam::File file;

	unlink("key_storage.isam");

	unsigned int rc = file.Open("key_storage.isam", 0);
	assert(rc == 0);

	isam::PageAllocator pa(&file);
	isam::PageLocking pl(&file);

	isam::KeyStorage ks(&file, &pa, &pl);

	rc = ks.InitialiseFile();
	assert(rc == 0);

	rc = ks.Store("foo", "bar");
	assert(rc == 0);

	std::string value;
	rc = ks.Fetch("foo", &value);
	assert(rc == 0);
	assert(value == "bar");

	rc = ks.Store("fob", "oar");
	assert(rc == 0);

	rc = ks.Fetch("foo", &value);
	assert(rc == 0);
	assert(value == "bar");
	rc = ks.Fetch("fob", &value);
	assert(rc == 0);
	assert(value == "oar");

	rc = ks.Store("for", "oab");
	assert(rc == 0);

	rc = ks.Fetch("foo", &value);
	assert(rc == 0);
	assert(value == "bar");
	rc = ks.Fetch("fob", &value);
	assert(rc == 0);
	assert(value == "oar");
	rc = ks.Fetch("for", &value);
	assert(rc == 0);
	assert(value == "oab");

	rc = ks.Store("fob", "oaroar");
	assert(rc == 0);

	rc = ks.Fetch("foo", &value);
	assert(rc == 0);
	assert(value == "bar");
	rc = ks.Fetch("fob", &value);
	assert(rc == 0);
	assert(value == "oaroar");
	rc = ks.Fetch("for", &value);
	assert(rc == 0);
	assert(value == "oab");

	rc = ks.Delete("foo");
	assert(rc == 0);

	rc = ks.Fetch("foo", &value);
	assert(rc == ENOENT);
	rc = ks.Fetch("fob", &value);
	assert(rc == 0);
	assert(value == "oaroar");

	rc = ks.Delete("fob");
	assert(rc == 0);

	rc = ks.Fetch("foo", &value);
	assert(rc == ENOENT);
	rc = ks.Fetch("fob", &value);
	assert(rc == ENOENT);

	rc = ks.Delete("for");
	assert(rc == 0);

	rc = ks.Fetch("foo", &value);
	assert(rc == ENOENT);
	rc = ks.Fetch("fob", &value);
	assert(rc == ENOENT);
	rc = ks.Fetch("for", &value);
	assert(rc == ENOENT);
    }

    {
	isam::File file;

	unlink("key_storage2.isam");

	unsigned int rc = file.Open("key_storage2.isam", 0);
	assert(rc == 0);

	isam::PageAllocator pa(&file);
	isam::PageLocking pl(&file);

	isam::KeyStorage ks(&file, &pa, &pl);

	rc = ks.InitialiseFile();
	assert(rc == 0);

	srand(42);

	std::string key, value;
	map_t map;

	{
	    Timer t;

	    for (unsigned int i=0; i<NUM_REQUESTS; ++i)
	    {
		key = RandomString();
		value = RandomString();
		map[key] = value;
		rc = ks.Store(key, value);
//		TRACE << i << " stored " << key << " = " << value << "\n";
		assert(rc == 0);

		if (!(i % 10000))
		{
		    TRACE << i << "/" << NUM_REQUESTS << "\n";

#if 1		
		    for (map_t::const_iterator j = map.begin();
			 j != map.end();
			 ++j)
		    {
			rc = ks.Fetch(j->first, &value);
			if (rc)
			{
			    TRACE << "Can't fetch " << j->first << ": " << rc << "\n";
			    ks.Dump(0,"");
			}
			assert(rc == 0);
			if (value != j->second)
			{
			    TRACE << "Key '" << j->first << "' expected '"
				  << j->second << "' got '" << value << "'\n";
			    ks.Dump(0,"");
			    exit(1);
			}
			assert(value == j->second);
		    }
#endif
		}
	    }

	    for (map_t::const_iterator j = map.begin();
		 j != map.end();
		 ++j)
	    {
		rc = ks.Delete(j->first);
		if (rc)
		    TRACE << "Can't delete '" << j->first << "': " << rc << "\n";
		assert(rc == 0);
		rc = ks.Fetch(j->first, &value);
		assert(rc == ENOENT);
	    }

	    printf("%u iterations: ", NUM_REQUESTS);
	}
    }

#if BERKELEY
    {
	Berkeley ks;

	srand(42);

	std::string key, value;
	map_t map;

	{
	    Timer t;

	for (unsigned int i=0; i<NUM_REQUESTS; ++i)
	{
	    key = RandomString();
	    value = RandomString();
	    map[key] = value;
	    unsigned int rc = ks.Store(key, value);
//	    TRACE << "Stored " << key << " = " << value << "\n";
	    assert(rc == 0);

	    for (map_t::const_iterator j = map.begin(); j != map.end(); ++j)
	    {
		rc = ks.Fetch(j->first, &value);
		if (rc)
		{
		    TRACE << "Can't fetch " << j->first << ": " << rc << "\n";
		}
		assert(rc == 0);
		if (value != j->second)
		{
		    TRACE << "Key '" << j->first << "' expected '"
			  << j->second << "' got '" << value << "'\n";
		}
		assert(value == j->second);
	    }
	}
	}
    }
#endif

    {
	isam::File file;

	unlink("key_storage3.isam");

	unsigned int rc = file.Open("key_storage3.isam", 0);
	assert(rc == 0);

	isam::PageAllocator pa(&file);
	isam::PageLocking pl(&file);

	isam::KeyStorage ks(&file, &pa, &pl);

	rc = ks.InitialiseFile();
	assert(rc == 0);

	srand(42);

	util::WorkerThreadPool wtp(util::WorkerThreadPool::NORMAL, 
				   NUM_THREADS);

	for (unsigned int i=0; i<NUM_THREADS; ++i)
	{
	    util::TaskPtr tp(new KeyTestTask(&ks, i));
	    wtp.PushTask(tp);
	}
	    
	util::Mutex::Lock lock(s_mutex);
	while (s_nfinished < NUM_THREADS)
	{
	    s_finished.Wait(lock, 60);
//	    TRACE << s_nfinished << "/" << NUM_THREADS << " finished\n";
	}

	printf("%u threads did %u iterations total\n", NUM_THREADS, 
	       (unsigned int)s_total);
    }

    return 0;
}

#endif
