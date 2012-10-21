#include "sync.h"
#include "fake_database.h"
# include "db.h"
# include "xml.h"
# include "schema.h"
# include "libutil/trace.h"
# include <boost/format.hpp>

#ifdef TEST

class TestSyncObserver: public mediadb::SyncObserver
{
    std::set<unsigned int> m_delenda, m_addenda, m_mutanda;

    static std::string SetToString(const std::set<unsigned int> *ids)
    {
	if (ids->empty())
	    return "";
	std::string str;
	for (std::set<unsigned int>::const_iterator i = ids->begin();
	     i != ids->end();
	     ++i)
	{
	    std::string num = (boost::format("%u") % *i).str();
	    if (!str.empty())
		str += ',';
	    str += num;
	}
	return str;
    }

public:
    void OnDelete(unsigned int destid)
    {
	assert(m_delenda.find(destid) == m_delenda.end());
	m_delenda.insert(destid);
    }
    void OnAddPlaylist(unsigned int srcid, size_t, size_t)
    {
	assert(m_addenda.find(srcid) == m_addenda.end());
	m_addenda.insert(srcid);
    }
    void OnAddFile(unsigned int srcid, size_t, size_t)
    {
	assert(m_addenda.find(srcid) == m_addenda.end());
	m_addenda.insert(srcid);
    }
    void OnAmendPlaylist(unsigned int destid, size_t, size_t)
    {
	assert(m_mutanda.find(destid) == m_mutanda.end());
	m_mutanda.insert(destid);
    }

    std::string GetDelenda() const
    {
	return SetToString(&m_delenda);
    }
    std::string GetAddenda() const
    {
	return SetToString(&m_addenda);
    }
    std::string GetMutanda() const
    {
	return SetToString(&m_mutanda);
    }
};

void Test(const char *src, const char *dest,
	  const char *delenda, const char *addenda, const char *mutanda)
{
    mediadb::FakeDatabase dbsrc;
    util::StringStreamPtr streamsrc = util::StringStream::Create();
    streamsrc->str() = src;
    mediadb::ReadXML(&dbsrc, streamsrc);

    mediadb::FakeDatabase dbdest;
    util::StringStreamPtr streamdest = util::StringStream::Create();
    streamdest->str() = dest;
    mediadb::ReadXML(&dbdest, streamdest);
    
    mediadb::Synchroniser sync(&dbsrc, &dbdest, false);

    TestSyncObserver tso;
    sync.AddObserver(&tso);
    
    unsigned int rc = sync.Synchronise();
    assert(rc == 0);

    if (tso.GetDelenda() != delenda)
    {
	TRACE << "** Expected delenda " << delenda << "\n   got "
	      << tso.GetDelenda() << "\n";
    }
    assert(tso.GetDelenda() == delenda);
    if (tso.GetAddenda() != addenda)
    {
	TRACE << "** Expected addenda " << addenda << "\n   got "
	      << tso.GetAddenda() << "\n";
    }
    assert(tso.GetAddenda() == addenda);
    if (tso.GetMutanda() != mutanda)
    {
	TRACE << "** Expected mutanda " << mutanda << "\n   got "
	      << tso.GetMutanda() << "\n";
    }
    assert(tso.GetMutanda() == mutanda);

//    mediadb::WriteXML(&dbdest, 1, stdout);

    // As we just synchronised them, a second sync should do no work

//    TRACE << "Resync\n";

    mediadb::Synchroniser sync2(&dbsrc, &dbdest, false);

    TestSyncObserver tso2;
    sync2.AddObserver(&tso2);
    
    rc = sync2.Synchronise();
    assert(rc == 0);

    assert(tso2.GetDelenda().empty());
    assert(tso2.GetAddenda().empty());
    assert(tso2.GetMutanda().empty());
}

/** Test databases.
 *
 * Each sync is done both ways round, with separate lists of expected
 * a/d/m each way. (One way's delenda is the other way's addenda, and
 * vice versa, but the mutanda will be different as it's always
 * numbered using the source's IDs for the playlists.)
 */
static const struct {
    const char *src;
    const char *dest;

    const char *delenda;
    const char *addenda;
    const char *mutanda;

    const char *delenda_rev;
    const char *addenda_rev;
    const char *mutanda_rev;
} tests[] = {

// 0. Can we add a single tune?
    {
"<db schema=1>"
"<record><id>256</id><title>root</title><type>playlist</type>"
" <children><child>260</child></children></record>"
"<record><id>260</id><title>Artists</title><type>playlist</type>"
" <children><child>270</child></children></record>"
"<record><id>270</id><title>Abba</title><type>playlist</type>"
" <children><child>280</child></children></record>"
"<record><id>280</id><title>Fernando</title><type>tune</type></record>"
"</db>",

"<db schema=1>"
"<record><id>256</id><title>root</title><type>playlist</type>"
" <children><child>1260</child></children></record>"
"<record><id>1260</id><title>Artists</title><type>playlist</type>"
" <children><child>1270</child></children></record>"
"<record><id>1270</id><title>Abba</title><type>playlist</type></record>"
"</db>",

"",
"280",
"270",

"280",
"",
"1270",
    },

// 1. A playlist tree
    {
"<db schema=1>"
"<record><id>256</id><title>root</title><type>playlist</type>"
" <children><child>260</child></children></record>"
"<record><id>260</id><title>Artists</title><type>playlist</type>"
" <children><child>270</child></children></record>"
"<record><id>270</id><title>Abba</title><type>playlist</type>"
" <children><child>280</child></children></record>"
"<record><id>280</id><title>Abba</title><type>tune</type></record>"
"</db>",

"<db schema=1>"
"<record><id>256</id><title>root</title><type>playlist</type></record>"
"</db>",

"",
"260,270,280",
"256",

"260,270,280",
"",
"256"
    },

// 2. Check Fernando isn't deleted, because a reference remains in Abba
    {
"<db schema=1>"
"<record><id>256</id><title>root</title><type>playlist</type>"
" <children><child>260</child><child>270</child></children></record>"
"<record><id>260</id><title>Abba</title><type>playlist</type>"
" <children><child>280</child></children></record>"
"<record><id>270</id><title>Party</title><type>playlist</type>"
" <children><child>280</child></children></record>"
"<record><id>280</id><title>Fernando</title><type>tune</type></record>"
"</db>",

"<db schema=1>"
"<record><id>256</id><title>root</title><type>playlist</type>"
" <children><child>1260</child></children></record>"
"<record><id>1260</id><title>Abba</title><type>playlist</type>"
" <children><child>1280</child></children></record>"
"<record><id>1280</id><title>Fernando</title><type>tune</type></record>"
"</db>",

"",
"270",
"256",

"270",
"",
"256"
    },


// 3. Remove one reference, but Fernando stays alive because there's a 2nd
    {
"<db schema=1>"
"<record><id>256</id><title>root</title><type>playlist</type>"
" <children><child>270</child><child>260</child></children></record>"
"<record><id>270</id><title>Party</title><type>playlist</type>"
" <children><child>280</child></children></record>"
"<record><id>260</id><title>Abba</title><type>playlist</type>"
" <children><child>280</child></children></record>"
"<record><id>280</id><title>Fernando</title><type>tune</type></record>"
"</db>",

"<db schema=1>"
"<record><id>256</id><title>root</title><type>playlist</type>"
" <children><child>1260</child></children></record>"
"<record><id>1260</id><title>Abba</title><type>playlist</type>"
" <children><child>1280</child></children></record>"
"<record><id>1280</id><title>Fernando</title><type>tune</type></record>"
"</db>",

"",
"270",
"256",

"270",
"",
"256"
    },

// 4. Replace tune with new rip
    {
"<db schema=1>"
"<record><id>256</id><title>root</title><type>playlist</type>"
" <children><child>260</child><child>270</child></children></record>"
"<record><id>260</id><title>Abba</title><type>playlist</type>"
" <children><child>280</child></children></record>"
"<record><id>270</id><title>Party</title><type>playlist</type>"
" <children><child>280</child></children></record>"
"<record><id>280</id><title>Fernando</title><type>tune</type></record>"
"</db>",

"<db schema=1>"
"<record><id>256</id><title>root</title><type>playlist</type>"
" <children><child>1260</child><child>1270</child></children></record>"
"<record><id>1260</id><title>Abba</title><type>playlist</type>"
" <children><child>1280</child></children></record>"
"<record><id>1270</id><title>Party</title><type>playlist</type>"
" <children><child>1280</child></children></record>"
"<record><id>1280</id><title>Fernando</title><type>tune</type>"
" <sizebytes>42</sizebytes></record>"
"</db>",

"",
"280",
"",

"",
"1280",
""
    },

// 5. New track coincidentally has name of old playlist
    {
"<db schema=1>"
"<record><id>256</id><title>root</title><type>playlist</type>"
" <children><child>260</child></children></record>"
"<record><id>260</id><title>Abba</title><type>playlist</type>"
" <children><child>280</child></children></record>"
"<record><id>280</id><title>Fernando</title><type>tune</type></record>"
"</db>",

"<db schema=1>"
"<record><id>256</id><title>root</title><type>playlist</type>"
" <children><child>1260</child></children></record>"
"<record><id>1260</id><title>Abba</title><type>playlist</type>"
" <children><child>1280</child></children></record>"
"<record><id>1280</id><title>Fernando</title><type>playlist</type>"
" <children><child>1290</child></children></record>"
"<record><id>1290</id><title>Fernandino</title><type>tune</type></record>"
"</db>",

"1280,1290",
"280",
"260",

"280",
"1280,1290",
"1260",
    },

// 6. New track coincidentally has name of old playlist, which still exists too
    {
"<db schema=1>"
"<record><id>256</id><title>root</title><type>playlist</type>"
" <children><child>260</child><child>270</child></children></record>"
"<record><id>260</id><title>Abba</title><type>playlist</type>"
" <children><child>280</child></children></record>"
"<record><id>270</id><title>Party</title><type>playlist</type>"
" <children><child>300</child></children></record>"
"<record><id>280</id><title>Fernando</title><type>playlist</type>"
" <children><child>290</child></children></record>"
"<record><id>290</id><title>Fernandino</title><type>tune</type></record>"
"<record><id>300</id><title>Fernando</title><type>tune</type></record>"
"</db>",

"<db schema=1>"
"<record><id>256</id><title>root</title><type>playlist</type>"
" <children><child>1260</child><child>1270</child></children></record>"
"<record><id>1260</id><title>Abba</title><type>playlist</type>"
" <children><child>1280</child></children></record>"
"<record><id>1270</id><title>Party</title><type>playlist</type>"
" <children><child>1280</child></children></record>"
"<record><id>1280</id><title>Fernando</title><type>playlist</type>"
" <children><child>1290</child></children></record>"
"<record><id>1290</id><title>Fernandino</title><type>tune</type></record>"
"</db>",

"",
"300",
"270",

"300",
"",
"1270",
    },

// 7. New playlist coincidentally has name of old track, which still exists too
    {
"<db schema=1>"
"<record><id>256</id><title>root</title><type>playlist</type>"
" <children><child>260</child><child>270</child></children></record>"
"<record><id>260</id><title>Abba</title><type>playlist</type>"
" <children><child>280</child></children></record>"
"<record><id>270</id><title>Party</title><type>playlist</type>"
" <children><child>300</child></children></record>"
"<record><id>280</id><title>Fernando</title><type>playlist</type>"
" <children><child>290</child></children></record>"
"<record><id>290</id><title>Fernandino</title><type>tune</type></record>"
"<record><id>300</id><title>Fernando</title><type>tune</type></record>"
"</db>",

"<db schema=1>"
"<record><id>256</id><title>root</title><type>playlist</type>"
" <children><child>1260</child><child>1270</child></children></record>"
"<record><id>1260</id><title>Abba</title><type>playlist</type>"
" <children><child>1280</child></children></record>"
"<record><id>1270</id><title>Party</title><type>playlist</type>"
" <children><child>1280</child></children></record>"
"<record><id>1280</id><title>Fernando</title><type>tune</type></record>"
"</db>",

"",
"280,290",
"260",

"280,290",
"",
"1260",
    },

// 8. Can we add a single tune, getting TUNEHIGH right?
    {
"<db schema=1>"
"<record><id>256</id><title>root</title><type>playlist</type>"
" <children><child>260</child></children></record>"
"<record><id>260</id><title>Artists</title><type>playlist</type>"
" <children><child>270</child></children></record>"
"<record><id>270</id><title>Abba</title><type>playlist</type>"
" <children><child>280</child></children></record>"
"<record><id>280</id><title>Fernando</title><type>tune</type>"
" <sizebytes>1</sizebytes><idhigh>20280</idhigh></record>"
"<record><id>20280</id><title>Fernando</title><type>tune</type>"
" <sizebytes>10</sizebytes></record>"
"</db>",

"<db schema=1>"
"<record><id>256</id><title>root</title><type>playlist</type>"
" <children><child>1260</child></children></record>"
"<record><id>1260</id><title>Artists</title><type>playlist</type>"
" <children><child>1270</child></children></record>"
"<record><id>1270</id><title>Abba</title><type>playlist</type></record>"
"</db>",

"",
"20280",
"270",

"280",
"",
"1270",
    },

// 9. Can we add a new rip, getting TUNEHIGH right?
    {
"<db schema=1>"
"<record><id>256</id><title>root</title><type>playlist</type>"
" <children><child>260</child></children></record>"
"<record><id>260</id><title>Artists</title><type>playlist</type>"
" <children><child>270</child></children></record>"
"<record><id>270</id><title>Abba</title><type>playlist</type>"
" <children><child>280</child></children></record>"
"<record><id>280</id><title>Fernando</title><type>tune</type>"
" <sizebytes>1</sizebytes><idhigh>20280</idhigh></record>"
"<record><id>20280</id><title>Fernando</title><type>tune</type>"
" <sizebytes>10</sizebytes></record>"
"</db>",

"<db schema=1>"
"<record><id>256</id><title>root</title><type>playlist</type>"
" <children><child>1260</child></children></record>"
"<record><id>1260</id><title>Artists</title><type>playlist</type>"
" <children><child>1270</child></children></record>"
"<record><id>1270</id><title>Abba</title><type>playlist</type>"
" <children><child>280</child></children></record>"
"<record><id>280</id><title>Fernando</title><type>tune</type>"
" <sizebytes>1</sizebytes></record>"
"</db>",

"",
"20280",
"",

"",
"",
"",
    },

// 10. Can we add a new rip, getting TUNEHIGH right, when it also appears
//     elsewhere?
    {
"<db schema=1>"
"<record><id>256</id><title>root</title><type>playlist</type>"
" <children><child>260</child><child>290</child></children></record>"
"<record><id>260</id><title>Artists</title><type>playlist</type>"
" <children><child>270</child></children></record>"
"<record><id>270</id><title>Abba</title><type>playlist</type>"
" <children><child>280</child></children></record>"
"<record><id>280</id><title>Fernando</title><type>tune</type>"
" <sizebytes>1</sizebytes><idhigh>20280</idhigh></record>"
"<record><id>20280</id><title>Fernando</title><type>tune</type>"
" <sizebytes>10</sizebytes></record>"
"<record><id>290</id><title>Party</title><type>playlist</type>"
" <children><child>280</child></children></record>"
"</db>",

"<db schema=1>"
"<record><id>256</id><title>root</title><type>playlist</type>"
" <children><child>1260</child></children></record>"
"<record><id>1260</id><title>Artists</title><type>playlist</type>"
" <children><child>1270</child></children></record>"
"<record><id>1270</id><title>Abba</title><type>playlist</type>"
" <children><child>1280</child></children></record>"
"<record><id>1280</id><title>Fernando</title><type>tune</type>"
" <sizebytes>1</sizebytes></record>"
"</db>",

"",
"290,20280",
"256",

"290",
"",
"256",
    },

};

int main()
{
    for (unsigned int i=0; i<sizeof(tests)/sizeof(tests[0]); ++i)
    {
//	TRACE << "Test #" << i << "\n";
	Test(tests[i].src, tests[i].dest, tests[i].delenda,
	     tests[i].addenda, tests[i].mutanda);
//	TRACE << "Test #" << i << "bis\n";
	Test(tests[i].dest, tests[i].src, tests[i].delenda_rev,
	     tests[i].addenda_rev, tests[i].mutanda_rev);
    }

    return 0;
}

#endif
