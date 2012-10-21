#include "content_factory.h"
#include "libutil/memory_stream.h"
#include "libutil/string_stream.h"
#include "libmediadb/db.h"
#include "libmediadb/schema.h"
#include "libutil/trace.h"
#include "libutil/stream.h"
#include "libutil/urlescape.h"
#include "libreceiver/tags.h"
#include <sstream>
#include <algorithm>
#include <boost/format.hpp>

namespace receiverd {

static const char tagstring[] = 
    "fid\n"
    "title\n"
    "artist\n"
    "source\n"
    "tracknr\n"
					   
    "genre\n"
    "year\n"
    "duration\n"
    "codec\n"
    "length\n"
					   
    "bitrate\n"
    "samplerate\n"
    "type\n"
    "mood\n"
    "originalartist\n"
    
    "remixed\n"
    "conductor\n"
    "composer\n"
    "ensemble\n"
    "lyricist\n";

void StreamAddString(util::StringStreamPtr st, int receiver_tag, 
		     const std::string& s)
{
    if (!s.empty())
    {
	st->str() += (char)receiver_tag;
	st->str() += (char)s.length();
	st->str() += s;
    }
}

void StreamAddInt(util::StringStreamPtr s, int receiver_tag, unsigned int n)
{
    if (n)
    {
	std::ostringstream os;
	os << n;
	StreamAddString(s, receiver_tag, os.str());
    }
}

void StreamAddChildren(util::StringStreamPtr s, int receiver_tag,
		       const std::string& ch)
{
    std::vector<unsigned int> vec;
    mediadb::ChildrenToVector(ch, &vec);
    
    std::string playlist;
    playlist.reserve(vec.size()*8 + 4);

    // Because our fids might not end in *0, we need to use New Playlists

    playlist.push_back(0xFF);
    playlist.push_back(0x02);
    playlist.push_back(0);
    playlist.push_back(0);

    for (unsigned int i=0; i<vec.size(); ++i)
    {
	unsigned int id = playlist[i];

	playlist.push_back(id & 0xFF);
	playlist.push_back((id>>8) & 0xFF);
	playlist.push_back((id>>16) & 0xFF);
	playlist.push_back((id>>24) & 0xFF);

	playlist.push_back(0);
	playlist.push_back(0);
	playlist.push_back(0);
	playlist.push_back(0);
    }
    StreamAddString(s, receiver_tag, playlist);
}

util::SeekableStreamPtr TagsStream(mediadb::Database *db, unsigned int id)
{
    db::QueryPtr qp = db->CreateQuery();
    qp->Restrict(mediadb::ID, db::EQ, id);
    db::RecordsetPtr rs = qp->Execute();
    if (!rs || rs->IsEOF())
    {
	TRACE << "Can't find FID " << id << "\n";
	return util::SeekableStreamPtr();
    }

    util::StringStreamPtr ss = util::StringStream::Create();
    
    StreamAddInt(ss, receiver::FID, id);
    StreamAddString(ss, receiver::TITLE, rs->GetString(mediadb::TITLE));
    unsigned int mtype = rs->GetInteger(mediadb::TYPE);
    const char *type = "tune";
    switch (mtype)
    {
    case mediadb::PLAYLIST:
    case mediadb::DIR:
	type = "playlist";
	break;
    }
    StreamAddString(ss, receiver::TYPE, type);
    
    if (mtype != mediadb::PLAYLIST && mtype != mediadb::DIR)
    {
	StreamAddInt(ss, receiver::DURATION,
		     rs->GetInteger(mediadb::DURATIONMS));
	const char *codec = "mp3";
	switch (rs->GetInteger(mediadb::CODEC))
	{
	case mediadb::FLAC: codec = "flac"; break;
	}
	StreamAddString(ss, receiver::CODEC, codec);
	StreamAddInt(ss, receiver::LENGTH, rs->GetInteger(mediadb::SIZEBYTES));
	StreamAddInt(ss, receiver::SAMPLERATE,
		     rs->GetInteger(mediadb::SAMPLERATE));
	std::ostringstream os;
	os << "vs" << (rs->GetInteger(mediadb::BITSPERSEC)/1000);
	StreamAddString(ss, receiver::BITRATE, os.str());
	
	StreamAddString(ss, receiver::ARTIST, rs->GetString(mediadb::ARTIST));
	StreamAddString(ss, receiver::SOURCE, rs->GetString(mediadb::ALBUM));
	StreamAddInt(ss, receiver::TRACKNR,
		     rs->GetInteger(mediadb::TRACKNUMBER));
	StreamAddString(ss, receiver::GENRE, rs->GetString(mediadb::GENRE));
	StreamAddInt(ss, receiver::YEAR, rs->GetInteger(mediadb::YEAR));
	StreamAddString(ss, receiver::MOOD, rs->GetString(mediadb::MOOD));
	StreamAddString(ss, receiver::ORIGINALARTIST,
			rs->GetString(mediadb::ORIGINALARTIST));
	StreamAddString(ss, receiver::REMIXED,
			rs->GetString(mediadb::REMIXED));
	// ... CONDUCTOR COMPOSER ENSEMBLE LYRICIST
    }

    unsigned char term = 0xFF;
    ss->str() += (char)term;
    ss->Seek(0);
    /*
    unsigned int len = ss->GetLength();
    TRACE << "response " << len << " bytes:\n";
    for (unsigned int i=0; i<len; ++i)
    {
	unsigned char ch;
	size_t n;
	ss->Read(&ch, 1, &n);
	if (n)
	    printf("  %02x", ch);
    }
    printf("\n");
    ss->Seek(0);
    */
    return ss;
}

util::SeekableStreamPtr QueryStream(mediadb::Database *db,
				    const char *path)
{
    unsigned int field = mediadb::FIELD_COUNT;
    std::string value;

    const char *query = strchr(path, '?');
    if (query)
    {
	const char *equals = strchr(query, '=');
	if (equals)
	{
	    const char *ampers = strchr(equals, '&');
	    if (ampers)
		value = std::string(equals+1, ampers-equals-1);
	    else
		value = std::string(equals+1);
	    value = util::URLUnEscape(value);

	    std::string sfield(query+1, equals-query-1);
	    if (sfield == "title")
		field = mediadb::TITLE;
	    else if (sfield == "artist")
		field = mediadb::ARTIST;
	    else if (sfield == "source")
		field = mediadb::ALBUM; // Note different name
	    else if (sfield == "genre")
		field = mediadb::GENRE;
	}
    }

    if (field == mediadb::FIELD_COUNT)
    {
	TRACE << "Don't understand query '" << path << "'\n";
	return util::SeekableStreamPtr();
    }

    db::QueryPtr qp = db->CreateQuery();
    qp->CollateBy(field);
    if (!value.empty())
	qp->Restrict(field, db::LIKE, value + ".*");
    db::RecordsetPtr rs = qp->Execute();

    if (!rs)
    {
	TRACE << "Can't do query\n";
	return util::SeekableStreamPtr();
    }

    util::StringStreamPtr ss = util::StringStream::Create();

    ss->str() += "matches=\n";
    unsigned int i=0;
    
    while (!rs->IsEOF())
    {
	std::ostringstream os;

	/** DO NOT convert from UTF-8 here, as the same strings will be sent
	 * back to us as search terms. This is ugly, but the alternatives are
	 * uglier.
	 */
	os << i << "=1,0,0:" << rs->GetString(0) << "\n";
	ss->str() += os.str();
	
	++i;
	rs->MoveNext();
    }

    ss->Seek(0);
    return ss;
}

util::SeekableStreamPtr ResultsStream(mediadb::Database *db,
				      const char *path)
{
    unsigned int field = mediadb::FIELD_COUNT;

    bool extended2 = (strstr(path, "_extended=2") != NULL);

    const char *query = strchr(path, '?');
    if (!query)
    {
	TRACE << "Don't like results url '" << path << "'\n";
	return util::SeekableStreamPtr();
    }
    const char *fieldname = query+1;
    const char *ampers = strchr(fieldname, '&');
    if (*fieldname == '_')
    {
	fieldname = ampers+1;
	ampers = strchr(fieldname, '&');
    }
    const char *equals = strchr(fieldname, '=');
    if (!equals)
    {
	TRACE << "Don't like results url '" << path << "'\n";
	return util::SeekableStreamPtr();
    }
    
    std::string sfield(fieldname, equals-fieldname);
    if (sfield == "title")
	field = mediadb::TITLE;
    else if (sfield == "artist")
	field = mediadb::ARTIST;
    else if (sfield == "source")
	field = mediadb::ALBUM; // Note different name
    else if (sfield == "genre")
	field = mediadb::GENRE;
    else
    {
	TRACE << "Don't like results field in '" << path << "'\n";
	return util::SeekableStreamPtr();
    }

    std::string value;
    if (ampers)
	value = std::string(equals+1, ampers-equals-1);
    else
	value = std::string(equals+1);
    value = util::URLUnEscape(value);

    db::QueryPtr qp = db->CreateQuery();
    qp->Restrict(field, db::EQ, value);
    qp->Restrict(mediadb::TYPE, db::EQ, mediadb::TUNE);
    db::RecordsetPtr rs = qp->Execute();

    if (!rs)
    {
	TRACE << "Can't do query\n";
	return util::SeekableStreamPtr();
    }

    util::StringStreamPtr ss = util::StringStream::Create();

    while (!rs->IsEOF())
    {
	if (extended2)
	{
	    std::string binary;
	    unsigned int childid = rs->GetInteger(mediadb::ID);
	    binary.push_back(childid & 0xFF);
	    binary.push_back((childid>>8) & 0xFF);
	    binary.push_back((childid>>16) & 0xFF);
	    binary.push_back((childid>>24) & 0xFF);

	    unsigned int size = rs->GetInteger(mediadb::SIZEBYTES);
	    binary.push_back(size & 0xFF);
	    binary.push_back((size>>8) & 0xFF);
	    binary.push_back((size>>16) & 0xFF);
	    binary.push_back((size>>24) & 0xFF);
	    
	    binary.push_back(0);
	    binary.push_back(0);
	    binary.push_back(0);
	    binary.push_back(0);
	    ss->str() += binary;
	}
	else
	{
	    /** @todo Upgrade to FLAC if advanced client
	     */
	    std::string line = (boost::format("%x=T%s\n")
				% rs->GetInteger(mediadb::ID)
				% rs->GetString(mediadb::TITLE)).str();
	    ss->str() += line;	
	}
	
	rs->MoveNext();
    }

    ss->Seek(0);
    return ss;
}

util::SeekableStreamPtr ContentStream(mediadb::Database *db,
				      unsigned int id, const char *path)
{
    db::QueryPtr qp = db->CreateQuery();
    qp->Restrict(mediadb::ID, db::EQ, id);
    db::RecordsetPtr rs = qp->Execute();
    if (!rs || rs->IsEOF())
    {
	TRACE << "Can't find FID " << id << "\n";
	return util::SeekableStreamPtr();
    }
    
    unsigned int type = rs->GetInteger(mediadb::TYPE);
    if (type == mediadb::DIR || type == mediadb::PLAYLIST)
    {
	std::vector<unsigned int> vec;
	mediadb::ChildrenToVector(rs->GetString(mediadb::CHILDREN), &vec);
	
	util::StringStreamPtr ss = util::StringStream::Create();

	bool is_utf8 = (strstr(path, "_utf8=1") != NULL);

	bool is_extended = (strstr(path, "_extended=1") != NULL);

	std::string playlist;
	
	if (!is_extended)
	{
	    /** Because our fids might not end in *0, we need to use
	     * New Playlists. Fortunately real Receivers always ask for
	     * extended content.
	     */
	    playlist.push_back(0xFF);
	    playlist.push_back(0x02);
	    playlist.push_back(0);
	    playlist.push_back(0);
	    ss->str() += playlist;
	}

	for (unsigned int i=0; i<vec.size(); ++i)
	{
	    qp = db->CreateQuery();
	    qp->Restrict(mediadb::ID, db::EQ, vec[i]);
	    rs = qp->Execute();

	    type = rs->GetInteger(mediadb::TYPE);
	    char chtype = 0;
	    switch (type)
	    {
	    case mediadb::DIR:
	    case mediadb::PLAYLIST:
		chtype = 'P';
		break;
	    case mediadb::TUNE:
	    case mediadb::TUNEHIGH:
	    case mediadb::SPOKEN:
	    case mediadb::RADIO:
		chtype = 'T';
		break;
	    }
	    if (!chtype)
		continue; // Don't offer images or videos

	    /** If client understands UTF-8, it understands FLAC. So
	     * upgrade the file if possible.
	     */
	    unsigned int childid = vec[i];
	    if (is_utf8)
	    {
		unsigned int newid = rs->GetInteger(mediadb::IDHIGH);
		if (newid)
		    childid = newid;
	    }

	    if (is_extended)
	    {
		std::ostringstream os;
		os << std::hex << childid << "=" << chtype
		   << rs->GetString(mediadb::TITLE) << "\n";
		ss->str() += os.str();
	    }
	    else
	    {
		playlist.clear();
		playlist.push_back(childid & 0xFF);
		playlist.push_back((childid>>8) & 0xFF);
		playlist.push_back((childid>>16) & 0xFF);
		playlist.push_back((childid>>24) & 0xFF);

		playlist.push_back(0);
		playlist.push_back(0);
		playlist.push_back(0);
		playlist.push_back(0);
		ss->str() += playlist;
	    }
	}

	ss->Seek(0);
	return ss;
    }

    // Must be a file then
    return db->OpenRead(id);
}

class Flattener
{
public:
    virtual ~Flattener() {}
    virtual void OnItem(unsigned int id, db::RecordsetPtr rs) = 0;
};

void Flatten(mediadb::Database *db, unsigned int id, bool upgrade,
	     Flattener *f)
{
    db::QueryPtr qp = db->CreateQuery();
    qp->Restrict(mediadb::ID, db::EQ, id);
    db::RecordsetPtr rs = qp->Execute();  

    unsigned int type = rs->GetInteger(mediadb::TYPE);
    if (type == mediadb::DIR || type == mediadb::PLAYLIST)
    {	
	std::vector<unsigned int> vec;
	mediadb::ChildrenToVector(rs->GetString(mediadb::CHILDREN), &vec);
	for (unsigned int i=0; i<vec.size(); ++i)
	    Flatten(db, vec[i], upgrade, f);
    }
    else
    {
	if (upgrade)
	{
	    unsigned int newid = rs->GetInteger(mediadb::IDHIGH);
	    if (newid)
	    {
		qp = db->CreateQuery();
		qp->Restrict(mediadb::ID, db::EQ, newid);
		rs = qp->Execute();
		if (rs && !rs->IsEOF())
		    id = newid;
	    }
	}
	f->OnItem(id, rs);
    }
}

struct PlaylistReplyExtended
{
    uint32_t fid;
    uint32_t length;
    uint32_t offset;
};

class PREFlattener: public Flattener
{
    std::vector<PlaylistReplyExtended> *m_pvec;

public:
    PREFlattener(std::vector<PlaylistReplyExtended> *pvec) 
	: m_pvec(pvec) {}

    void OnItem(unsigned int id, db::RecordsetPtr);
};

void PREFlattener::OnItem(unsigned int id, db::RecordsetPtr rs)
{
    PlaylistReplyExtended pre;
    pre.fid = id;
    pre.length = rs->GetInteger(mediadb::SIZEBYTES);
    pre.offset = 0;
    m_pvec->push_back(pre);
}

util::SeekableStreamPtr ListStream(mediadb::Database *db,
				   unsigned int id, const char *path)
{
    db::QueryPtr qp = db->CreateQuery();
    qp->Restrict(mediadb::ID, db::EQ, id);
    db::RecordsetPtr rs = qp->Execute();

    bool extended = (strstr(path, "_extended=2") != NULL);
    bool shuffle = (strstr(path, "shuffle=1") != NULL);
    bool utf8 = (strstr(path, "_utf8=1") != NULL);

    util::MemoryStreamPtr ms;
    util::MemoryStream::Create(&ms, 1000);
    unsigned int rc;

    if (extended)
    {
	std::vector<PlaylistReplyExtended> vec;
	PREFlattener pf(&vec);
	Flatten(db, id, utf8, &pf);

	if (shuffle)
	    std::random_shuffle(vec.begin(), vec.end());

	unsigned int size = vec.size();
	if (size > 999)
	    size = 999;

	PlaylistReplyExtended *pre = &vec[0];
	rc = ms->WriteAll(pre, size * sizeof(PlaylistReplyExtended));
    }
    ms->Seek(0);
    return ms;
}

util::SeekableStreamPtr ContentFactory::StreamForPath(const char *path)
{
    unsigned int id = 0;

    TRACE << path << "\n";

    if (!strcmp(path, "/tags"))
    {
	util::StringStreamPtr ss = util::StringStream::Create();
	ss->str() += tagstring;
	ss->Seek(0);
	return ss;
    }
    else if (sscanf(path, "/tags/%x", &id) == 1)
    {
//	TRACE << "tags for " << id << "\n";
	return TagsStream(m_db, id);
    }
    else if (!strncmp(path, "/query?", 7))
    {
	return QueryStream(m_db, path);
    }
    else if (!strncmp(path, "/results?", 9))
    {
	return ResultsStream(m_db, path);
    }
    else if (sscanf(path, "/content/%x", &id))
    {
	return ContentStream(m_db, id, path);
    }
    else if (sscanf(path, "/list/%x", &id))
    {
	return ListStream(m_db, id, path);
    }
    else
    {
	TRACE << "** receiverd doesn't like URL " << path << "\n";
    }
    return util::SeekableStreamPtr();
}

}; // namespace receiverd

#ifdef TEST

# include "libdbsteam/db.h"
# include "libmediadb/xml.h"
# include "libmediadb/schema.h"
# include "libmediadb/localdb.h"

static const struct {
    const char *url;
    const char *result;
} tests[] = {
    { "/tags",

      receiverd::tagstring },

    { "/content/100&_extended=1",

      "121=P1992-2002\n"
      "122=PEverything Is\n"
      "123=PSongsStartingWithB\n"
      "125=PThe Garden\n"
      "126=PViolator\n"

    },

    { "/content/126&_extended=1",

      "16e=TWorld In My Eyes\n"
      "170=TSweetest Perfection\n"
      "172=TPersonal Jesus\n"
      "173=THalo\n"
      "174=TWaiting For The Night\n"
      "176=TEnjoy The Silence\n"
      "177=TPolicy Of Truth\n"
      "14b=TBlue Dress\n"
      "179=TClean\n"

    },

    { "/content/126&_extended=1&_utf8=1",

      "16f=TWorld In My Eyes\n"
      "171=TSweetest Perfection\n"
      "182=TPersonal Jesus\n"
      "180=THalo\n"
      "17a=TWaiting For The Night\n"
      "17f=TEnjoy The Silence\n"
      "183=TPolicy Of Truth\n"
      "15f=TBlue Dress\n"
      "181=TClean\n"

      /* Note different IDs from above -- these are the FLACs */

    },

    { "/content/125&_extended=1",

      "145=TFutures\n"
      "147=TThrow It All Away\n"
      "149=TSeeing Things\n"
      "14c=TThe Pageant Of The Bizarre\n"
      "14e=TYou\xe2\x80\x99re My Flame\n"
      "151=TLeft Behind\n"
      "152=TToday\n"
      "154=TThis Fine Social Scene\n"
      "157=TYour Place\n"
      "159=TIf I Can\xe2\x80\x99t Have You\n"
      "15c=TCrosses\n"
      "15e=TWaiting To Die\n"
    },

    { "/query?genre=",

      "matches=\n"
      "0=1,0,0:Ambient\n"
      "1=1,0,0:Electronica\n"
      "2=1,0,0:Rock\n"
    },

    { "/query?artist=",

      "matches=\n"
      "0=1,0,0:Depeche Mode\n"
      "1=1,0,0:Nine Black Alps\n"
      "2=1,0,0:Underworld\n"
      "3=1,0,0:Zero 7\n"
    },

    { "/query?source=",

      "matches=\n"
      "0=1,0,0:1992-2002\n"
      "1=1,0,0:Everything Is\n"
      "2=1,0,0:The Garden\n"
      "3=1,0,0:Violator\n"
    },

    { "/query?source=[8tuv]",

      "matches=\n"
      "0=1,0,0:The Garden\n"
      "1=1,0,0:Violator\n"
    },

    { "/query?source=[8tuv][4ghi][6mno]",

      "matches=\n"
      "0=1,0,0:Violator\n"
    },

    { "/results?source=Violator",

      "14b=TBlue Dress\n"
      "16e=TWorld In My Eyes\n"
      "170=TSweetest Perfection\n"
      "172=TPersonal Jesus\n"
      "173=THalo\n"
      "174=TWaiting For The Night\n"
      "176=TEnjoy The Silence\n"
      "177=TPolicy Of Truth\n"
      "179=TClean\n"
    },

    { "/results?source=The%20Garden",

      "145=TFutures\n"
      "147=TThrow It All Away\n"
      "149=TSeeing Things\n"
      "14c=TThe Pageant Of The Bizarre\n"
      "14e=TYou\xe2\x80\x99re My Flame\n"
      "151=TLeft Behind\n"
      "152=TToday\n"
      "154=TThis Fine Social Scene\n"
      "157=TYour Place\n"
      "159=TIf I Can\xe2\x80\x99t Have You\n"
      "15c=TCrosses\n"
      "15e=TWaiting To Die\n"
    },
};

enum { NTESTS = sizeof(tests)/sizeof(tests[0]) };


/* Tests which can return 0-bytes aren't as neat */

#define RESULT(txt) txt, sizeof(txt)-1

static const char tags100result[] = "\x00\x03" "256"
				    "\x01\x03" "mp3"
				    "\x0c\x08" "playlist"
				    "\xff";

static const char tags13Eresult[] =
"\x00\x03" "318"
"\x01\x07" "Cowgirl"
"\x0c\x04" "tune"
"\x07\x06" "510000"
"\x08\x04" "flac"
"\x09\x08" "63095852"
"\x0b\x05" "44100"
"\x0a\x05" "vs989"
"\x02\x0a" "Underworld"
"\x03\x09" "1992-2002"
"\x04\x01" "8"
"\x05\x0b" "Electronica"
"\x06\x04" "1994"
"\xff";

static const struct {
    const char *url;
    const char *result;
    size_t resultsize;
} tests2[] = {
    { "/tags/100", RESULT(tags100result) },
    { "/tags/13e", RESULT(tags13Eresult) },
};

enum { NTESTS2 = sizeof(tests2)/sizeof(tests2[0]) };


std::string Printable(const std::string& s)
{
    std::string result;
    result.reserve(s.length());

    for (unsigned int i=0; i<s.length(); ++i)
    {
	unsigned char c = s[i];
	if (c == 10 || (c>=32 && c<=126))
	    result += c;
	else
	{
	    std::string s2  =(boost::format("\\x%02x") % (int)c).str();
//	    TRACE << "c=" << c << " s2='" << s2 << "'\n";
	    result += s2;
	}
    }
    return result;
}

int main()
{
    db::steam::Database sdb(mediadb::FIELD_COUNT);
    sdb.SetFieldInfo(mediadb::ID, 
		     db::steam::FIELD_INT|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::PATH,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::ARTIST,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::ALBUM,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::GENRE,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::TITLE,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);

    mediadb::ReadXML(&sdb, SRCROOT "/libmediadb/example.xml");

    mediadb::LocalDatabase mdb(&sdb);

    receiverd::ContentFactory rcf(&mdb);

    for (unsigned int i=0; i<NTESTS; ++i)
    {
	util::SeekableStreamPtr s = rcf.StreamForPath(tests[i].url);

	assert(s);
	
	util::StringStreamPtr ss = util::StringStream::Create();
	ss->Copy(s);

	if (ss->str() != tests[i].result)
	{
	    TRACE << "URL " << tests[i].url << " gave:\n"
		  << Printable(ss->str()) 
		  << "\nshould be\n" 
		  << Printable(tests[i].result) << "\n\n";
	}

	assert(ss->str() == tests[i].result);
    }

    for (unsigned int i=0; i<NTESTS2; ++i)
    {
	util::SeekableStreamPtr s = rcf.StreamForPath(tests2[i].url);

	assert(s);
	
	util::StringStreamPtr ss = util::StringStream::Create();
	ss->Copy(s);

	std::string expected(tests2[i].result, tests2[i].resultsize);

	if (ss->str() != expected)
	{
	    TRACE << "URL " << tests[i].url << " gave:\n"
		  << Printable(ss->str()) 
		  << "\nshould be\n" 
		  << Printable(expected) << "\n\n";
	}

	assert(ss->str() == expected);
    }
    return 0;
}

#endif
