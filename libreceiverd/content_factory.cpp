#include "content_factory.h"
#include "libutil/memory_stream.h"
#include "libutil/string_stream.h"
#include "libmediadb/db.h"
#include "libmediadb/schema.h"
#include "libdb/query.h"
#include "libdb/recordset.h"
#include "libutil/trace.h"
#include "libutil/stream.h"
#include "libutil/urlescape.h"
#include "libutil/printf.h"
#include "libutil/counted_pointer.h"
#include "libreceiver/tags.h"
#include <stdio.h>
#include <sstream>
#include <algorithm>
#include <limits.h>

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

static void StreamAddString(std::string *st, int receiver_tag, 
			    const std::string& s)
{
    if (!s.empty())
    {
	*st += (char)receiver_tag;
	*st += (char)s.length();
	*st += s;
    }
}

static void StreamAddInt(std::string *s, int receiver_tag, unsigned int n)
{
    if (n)
    {
	std::ostringstream os;
	os << n;
	StreamAddString(s, receiver_tag, os.str());
    }
}

#if 0
static void StreamAddChildren(std::string *s, int receiver_tag,
			      const std::string& ch)
{
    std::vector<unsigned int> vec;
    mediadb::ChildrenToVector(ch, &vec);
    
    std::string playlist;
    playlist.reserve(vec.size()*8 + 4);

    // Because our fids might not end in *0, we need to use New Playlists

    playlist.push_back('\xFF');
    playlist.push_back(0x02);
    playlist.push_back(0);
    playlist.push_back(0);

    for (unsigned int i=0; i<vec.size(); ++i)
    {
	unsigned int id = playlist[i];

	playlist.push_back((char)id);
	playlist.push_back((char)(id>>8));
	playlist.push_back((char)(id>>16));
	playlist.push_back((char)(id>>24));

	playlist.push_back(0);
	playlist.push_back(0);
	playlist.push_back(0);
	playlist.push_back(0);
    }
    StreamAddString(s, receiver_tag, playlist);
}
#endif

#if 0
static void TraceTags(const std::string& s)
{
    const char *ptr = s.c_str();
    const char *end = ptr + s.length();

    printf("Tags:\n");

    while (ptr < end)
    {
	unsigned char tag = *ptr++;
	printf("<%02x>", tag);
	if (tag == 0xFF)
	    break;
	unsigned char len = *ptr++;
	printf("%.*s\n", len, ptr);
	ptr += len;
    }
    printf("\n");
}
#endif

static
std::unique_ptr<util::Stream> TagsStream(mediadb::Database *db, unsigned int id)
{
    std::unique_ptr<util::Stream> ss;

    db::QueryPtr qp = db->CreateQuery();
    qp->Where(qp->Restrict(mediadb::ID, db::EQ, id));
    db::RecordsetPtr rs = qp->Execute();
    if (!rs || rs->IsEOF())
    {
	TRACE << "Can't find FID " << id << "\n";
	return ss;
    }

    std::string s;

    StreamAddInt(&s, receiver::FID, id);
    StreamAddString(&s, receiver::TITLE, rs->GetString(mediadb::TITLE));
    unsigned int mtype = rs->GetInteger(mediadb::TYPE);
    const char *type = "tune";
    switch (mtype)
    {
    case mediadb::PLAYLIST:
    case mediadb::DIR:
	type = "playlist";
	break;
    case mediadb::IMAGE:
	type = "image";
	break;
    case mediadb::VIDEO:
	type = "video";
	break;
    case mediadb::FILE:
	type = "unknown";
	break;
    }
    StreamAddString(&s, receiver::TYPE, type);
    
    if (mtype != mediadb::PLAYLIST && mtype != mediadb::DIR)
    {
	StreamAddInt(&s, receiver::DURATION,
		     rs->GetInteger(mediadb::DURATIONMS));
	const char *codec = "mp3";
	switch (rs->GetInteger(mediadb::AUDIOCODEC))
	{
	case mediadb::FLAC: codec = "flac"; break;
	case mediadb::WAV: codec = "wave"; break;
	}
	StreamAddString(&s, receiver::CODEC, codec);

	StreamAddInt(&s, receiver::LENGTH, rs->GetInteger(mediadb::SIZEBYTES));
	StreamAddInt(&s, receiver::SAMPLERATE,
		     rs->GetInteger(mediadb::SAMPLERATE));
	std::ostringstream os;
	os << "vs" << (rs->GetInteger(mediadb::BITSPERSEC)/1000);
	StreamAddString(&s, receiver::BITRATE, os.str());
	
	StreamAddString(&s, receiver::ARTIST, rs->GetString(mediadb::ARTIST));
	StreamAddString(&s, receiver::SOURCE, rs->GetString(mediadb::ALBUM));
	StreamAddInt(&s, receiver::TRACKNR,
		     rs->GetInteger(mediadb::TRACKNUMBER));
	StreamAddString(&s, receiver::GENRE, rs->GetString(mediadb::GENRE));
	StreamAddInt(&s, receiver::YEAR, rs->GetInteger(mediadb::YEAR));
	StreamAddString(&s, receiver::MOOD, rs->GetString(mediadb::MOOD));
	StreamAddString(&s, receiver::ORIGINALARTIST,
			rs->GetString(mediadb::ORIGINALARTIST));
	StreamAddString(&s, receiver::REMIXED,
			rs->GetString(mediadb::REMIXED));
	// ... CONDUCTOR COMPOSER ENSEMBLE LYRICIST
    }

    unsigned char term = 0xFF;
    s += (char)term;

//    TraceTags(ss->str());

    ss.reset(new util::StringStream(s));

/*
    unsigned int len = (unsigned int)ss->GetLength();
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

static std::unique_ptr<util::Stream> QueryStream(mediadb::Database *db,
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

    std::unique_ptr<util::Stream> sp;

    if (field == mediadb::FIELD_COUNT)
    {
	TRACE << "Don't understand query '" << path << "'\n";
	return sp;
    }

    db::QueryPtr qp = db->CreateQuery();
    qp->CollateBy(field);
    if (value.empty())
	qp->Where(qp->Restrict(mediadb::TYPE, db::EQ, mediadb::TUNE));
    else
	qp->Where(qp->And(qp->Restrict(field, db::LIKE, value + ".*"),
			  qp->Restrict(mediadb::TYPE, db::EQ, mediadb::TUNE)));
		  
    db::RecordsetPtr rs = qp->Execute();

    if (!rs)
    {
	TRACE << "Can't do query\n";
	return sp;
    }

    std::string s = "matches=\n";
    unsigned int i=0;
    
    while (!rs->IsEOF())
    {
	std::ostringstream os;

	/** DO NOT convert from UTF-8 here, as the same strings will be sent
	 * back to us as search terms. This is ugly, but the alternatives are
	 * uglier.
	 */
	os << i << "=1,0,0:" << rs->GetString(0) << "\n";
	s += os.str();
	
	++i;
	rs->MoveNext();
    }

    sp.reset(new util::StringStream(s));
    return sp;
}

static std::unique_ptr<util::Stream> ResultsStream(mediadb::Database *db,
						 const char *path)
{
    std::unique_ptr<util::Stream> sp;
    
    unsigned int field = mediadb::FIELD_COUNT;

    bool extended2 = (strstr(path, "_extended=2") != NULL);

    const char *query = strchr(path, '?');
    if (!query)
    {
	TRACE << "Don't like results url '" << path << "'\n";
	return sp;
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
	return sp;
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
	return sp;
    }

    std::string value;
    if (ampers)
	value = std::string(equals+1, ampers-equals-1);
    else
	value = std::string(equals+1);
    value = util::URLUnEscape(value);

    db::QueryPtr qp = db->CreateQuery();
    qp->Where(qp->And(qp->Restrict(field, db::EQ, value),
		      qp->Restrict(mediadb::TYPE, db::EQ, mediadb::TUNE)));
    db::RecordsetPtr rs = qp->Execute();

    if (!rs)
    {
	TRACE << "Can't do query\n";
	return sp;
    }

    std::string s;

    while (!rs->IsEOF())
    {
	if (extended2)
	{
	    std::string binary;
	    unsigned int childid = rs->GetInteger(mediadb::ID);
	    binary.push_back((char)childid);
	    binary.push_back((char)(childid>>8));
	    binary.push_back((char)(childid>>16));
	    binary.push_back((char)(childid>>24));

	    unsigned int size = rs->GetInteger(mediadb::SIZEBYTES);
	    binary.push_back((char)size);
	    binary.push_back((char)(size>>8));
	    binary.push_back((char)(size>>16));
	    binary.push_back((char)(size>>24));
	    
	    binary.push_back(0);
	    binary.push_back(0);
	    binary.push_back(0);
	    binary.push_back(0);
	    s += binary;
	}
	else
	{
	    /** @todo Upgrade to FLAC if advanced client
	     */
	    std::string line = util::SPrintf("%x=T%s\n",
					     rs->GetInteger(mediadb::ID),
					     rs->GetString(mediadb::TITLE).c_str());
	    s += line;	
	}
	
	rs->MoveNext();
    }

    sp.reset(new util::StringStream(s));
    return sp;
}

static void GetContentStream(mediadb::Database *db, unsigned int id,
                             const char *path, util::http::Response *rsp)
{
    db::QueryPtr qp = db->CreateQuery();
    qp->Where(qp->Restrict(mediadb::ID, db::EQ, id));
    db::RecordsetPtr rs = qp->Execute();
    if (!rs || rs->IsEOF())
    {
	TRACE << "Can't find FID " << id << "\n";
	return;
    }
    
    unsigned int type = rs->GetInteger(mediadb::TYPE);
    if (type == mediadb::DIR || type == mediadb::PLAYLIST)
    {
	std::vector<unsigned int> vec;
	mediadb::ChildrenToVector(rs->GetString(mediadb::CHILDREN), &vec);
	
	bool is_utf8 = (strstr(path, "_utf8=1") != NULL);

	bool is_extended = (strstr(path, "_extended=1") != NULL);

	std::string playlist;
	std::string s;
	
	if (!is_extended)
	{
	    /** Because our fids might not end in *0, we need to use
	     * New Playlists. Fortunately real Receivers always ask for
	     * extended content.
	     */
	    playlist.push_back('\xFF');
	    playlist.push_back(0x02);
	    playlist.push_back(0);
	    playlist.push_back(0);
	    s += playlist;
	}

//	TRACE << "FID " << id << " has " << vec.size() << " children\n";

	for (unsigned int i=0; i<vec.size(); ++i)
	{
//	    TRACE << "child " << i << ": " << vec[i] << "\n";

	    qp = db->CreateQuery();
	    qp->Where(qp->Restrict(mediadb::ID, db::EQ, vec[i]));
	    rs = qp->Execute();

//	    TRACE << "title is '" << rs->GetString(mediadb::TITLE) << "'\n";

	    type = rs->GetInteger(mediadb::TYPE);
	    char chtype = 0;
	    switch (type)
	    {
	    case mediadb::DIR:
	    case mediadb::PLAYLIST:
		chtype = 'P';
		break;
	    case mediadb::TUNE:
	    case mediadb::SPOKEN:
	    case mediadb::RADIO:
		chtype = 'T';
		break;
	    }

//	    TRACE << "chtype=" << (int)chtype << "\n";

	    if (!chtype)
		continue; // Don't offer images or videos

	    unsigned int codec = rs->GetInteger(mediadb::AUDIOCODEC);

	    /** Real Rio Receivers can only play MP3 (and WMA).
	     */
	    if (!is_utf8 && (chtype == 'T' && codec != mediadb::MP3))
		continue;

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
		s += os.str();
	    }
	    else
	    {
		playlist.clear();
		playlist.push_back((char)childid);
		playlist.push_back((char)(childid>>8));
		playlist.push_back((char)(childid>>16));
		playlist.push_back((char)(childid>>24));

		playlist.push_back(0);
		playlist.push_back(0);
		playlist.push_back(0);
		playlist.push_back(0);
		s += playlist;
	    }
	}

//	TRACE << "'''" << ss->str() << "'''\n";

	rsp->body_source.reset(new util::StringStream(s));
	return;
    }

    /* Rio Receivers expect a bogus form of the Content-Range header,
     * but most UPnP clients need the real one. We use ?_range=1 to
     * force the real one.
     */
    const bool bogus_range = strstr(path, "_range=1") == NULL;
    if (bogus_range)
    {
        rsp->headers["X-Receiver-Range"] = "1";
    }

    // Must be a file then
    rsp->body_source = db->OpenRead(id);
    if (type == mediadb::RADIO)
	rsp->length = INT_MAX; // Not UINT_MAX as Receivers can't handle that
    else
	rsp->length = rs->GetInteger(mediadb::SIZEBYTES);

    switch (rs->GetInteger(mediadb::TYPE))
    {
        case mediadb::TUNE:
        case mediadb::TUNEHIGH:
            switch (rs->GetInteger(mediadb::AUDIOCODEC))
            {
            case mediadb::MP2: rsp->content_type = "audio/x-mp2"; break;
            case mediadb::MP3: rsp->content_type = "audio/mpeg"; break;
            case mediadb::FLAC: rsp->content_type = "audio/x-flac"; break;
            case mediadb::VORBIS: rsp->content_type = "application/ogg"; break;
            case mediadb::PCM: rsp->content_type = "audio/L16"; break;
            case mediadb::WAV: rsp->content_type = "audio/x-wav"; break;
            }
            break;
        case mediadb::VIDEO:
            switch (rs->GetInteger(mediadb::CONTAINER))
            {
            case mediadb::MP4: rsp->content_type = "video/mp4"; break;
            case mediadb::MPEGPS: rsp->content_type = "video/mpeg"; break;
            }
        default:
            break;
    }

    rsp->headers["transferMode.dlna.org"] = "Streaming";
    rsp->headers["contentFeatures.dlna.org"] = "DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01700000000000000000000000000000";
}

class Flattener
{
public:
    virtual ~Flattener() {}
    virtual void OnItem(unsigned int id, db::RecordsetPtr rs) = 0;
};

static void Flatten(mediadb::Database *db, unsigned int id, bool upgrade,
                    Flattener *f)
{
    db::QueryPtr qp = db->CreateQuery();
    qp->Where(qp->Restrict(mediadb::ID, db::EQ, id));
    db::RecordsetPtr rs = qp->Execute();  

    unsigned int type = rs->GetInteger(mediadb::TYPE);
    switch (type)
    {
    case mediadb::DIR:
    case mediadb::PLAYLIST:
    {	
	std::vector<unsigned int> vec;
	mediadb::ChildrenToVector(rs->GetString(mediadb::CHILDREN), &vec);
	for (unsigned int i=0; i<vec.size(); ++i)
	    Flatten(db, vec[i], upgrade, f);
	break;
    }
    case mediadb::TUNE:
    case mediadb::SPOKEN:
    case mediadb::RADIO:
	if (upgrade)
	{
	    unsigned int newid = rs->GetInteger(mediadb::IDHIGH);
	    if (newid)
	    {
		qp = db->CreateQuery();
		qp->Where(qp->Restrict(mediadb::ID, db::EQ, newid));
		rs = qp->Execute();
		if (rs && !rs->IsEOF())
		    id = newid;
	    }
	}
	f->OnItem(id, rs);
	break;
    default:
	// Don't show Receivers videos or images
	break;
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

    void OnItem(unsigned int id, db::RecordsetPtr) override;
};

void PREFlattener::OnItem(unsigned int id, db::RecordsetPtr rs)
{
    PlaylistReplyExtended pre;
    pre.fid = id;
    pre.length = rs->GetInteger(mediadb::SIZEBYTES);
    pre.offset = 0;
    m_pvec->push_back(pre);

//    TRACE << "Returning id " << id << " size " << pre.length << "\n";
}

static std::unique_ptr<util::Stream> ListStream(mediadb::Database *db,
                                                unsigned int id,
                                                const char *path,
                                                std::default_random_engine *rng)
{
    db::QueryPtr qp = db->CreateQuery();
    qp->Where(qp->Restrict(mediadb::ID, db::EQ, id));
    db::RecordsetPtr rs = qp->Execute();

    bool extended = (strstr(path, "_extended=2") != NULL);
    bool shuffle = (strstr(path, "shuffle=1") != NULL);
    bool utf8 = (strstr(path, "_utf8=1") != NULL);

    std::unique_ptr<util::Stream> ms(new util::MemoryStream);

    if (extended)
    {
	std::vector<PlaylistReplyExtended> vec;
	PREFlattener pf(&vec);
	Flatten(db, id, utf8, &pf);

	if (shuffle) {
	    std::shuffle(vec.begin(), vec.end(), *rng);
        }

	size_t size = vec.size();
	if (size > 999)
	    size = 999;

	assert(sizeof(PlaylistReplyExtended) == 12);

	PlaylistReplyExtended *pre = &vec[0];
        (void)!ms->WriteAll(pre, size * sizeof(PlaylistReplyExtended));
    }
    ms->Seek(0);
    return ms;
}

ContentFactory::ContentFactory(mediadb::Database *db)
    : m_db(db)
{
    std::vector<uint32_t> random_data(624); // enough for Mersenne-19937
    std::random_device source;
    std::generate(random_data.begin(), random_data.end(), std::ref(source));
    std::seed_seq seeds(random_data.begin(), random_data.end());
    m_random.seed(seeds);
}

bool ContentFactory::StreamForPath(const util::http::Request *rq,
				   util::http::Response *rs)
{
    unsigned int id = 0;

//    TRACE << rq->path << "\n";

    const char *path = rq->path.c_str();

    if (rq->path == "/tags")
    {
	rs->body_source.reset(new util::StringStream(tagstring));
	return true;
    }
    else if (sscanf(path, "/tags/%x", &id) == 1)
    {
	TRACE << "tags for " << id << "\n";
	rs->body_source = TagsStream(m_db, id);
	return true;
    }
    else if (!strncmp(path, "/query?", 7))
    {
	rs->body_source = QueryStream(m_db, path);
	return true;
    }
    else if (!strncmp(path, "/results?", 9))
    {
	rs->body_source = ResultsStream(m_db, path);
	return true;
    }
    else if (sscanf(path, "/content/%x", &id) == 1)
    {
	GetContentStream(m_db, id, path, rs);
	return true;
    }
    else if (sscanf(path, "/list/%x", &id) == 1)
    {
	rs->body_source = ListStream(m_db, id, path, &m_random);
	return true;
    }
    else
    {
//	TRACE << "** receiverd doesn't like URL " << path << "\n";
    }
    return false;
}

} // namespace receiverd

#ifdef TEST

# include "libdbsteam/db.h"
# include "libmediadb/xml.h"
# include "libmediadb/schema.h"
# include "libdblocal/db.h"
# include "libdbmerge/db.h"
# include "libutil/http_client.h"
# include <boost/format.hpp>

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

static const char list123result[] =
    "\x2e\x01\x00\x00" // FID 302
    "\x03\x1b\x9d\x00" //   len(302) = 10296067 = 0x9d1b03
    "\x00\x00\x00\x00" //   offset(302) = 0
    "\x31\x01\x00\x00" // FID 305 (promoted from 297)
    "\xef\x24\xbb\x04" //   len(305) = 79373551 = 0x4bb24ef
    "\x00\x00\x00\x00" //   offset(305) = 0
    "\x4a\x01\x00\x00" // FID 330 (promoted from 308)
    "\x49\xda\x33\x01" //   len(330) = 20175433 = 0x133da49
    "\x00\x00\x00\x00" //   offset(330) = 0
    "\x5f\x01\x00\x00" // FID 351 (promoted from 331)
    "\x73\x75\x06\x02" //   len(351) = 33977715 = 0x2067573
    "\x00\x00\x00\x00" //   offset(351) = 0
    "";

static const struct {
    const char *url;
    const char *result;
    size_t resultsize;
} tests2[] = {
    { "/tags/100", RESULT(tags100result) },
    { "/tags/13e", RESULT(tags13Eresult) },
    { "/list/123?_extended=2&_utf8=1", RESULT(list123result) },
};

enum { NTESTS2 = sizeof(tests2)/sizeof(tests2[0]) };


static std::string Printable(const std::string& s)
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
	    std::string s2 =(boost::format("\\x%02x") % (int)c).str();
//	    TRACE << "c=" << c << " s2='" << s2 << "'\n";
	    result += s2;
	}
    }
    return result;
}

static void DoTests(mediadb::Database *mdb)
{
    receiverd::ContentFactory rcf(mdb);

    for (unsigned int i=0; i<NTESTS; ++i)
    {
	util::http::Request rq;
	rq.path = tests[i].url;
	util::http::Response rs;
	bool found = rcf.StreamForPath(&rq, &rs);

	assert(found);
	
	util::StringStream ss;
	util::CopyStream(rs.body_source.get(), &ss);

	if (ss.str() != tests[i].result)
	{
	    TRACE << "URL " << tests[i].url << " gave:\n"
		  << Printable(ss.str()) 
		  << "\nshould be\n" 
		  << Printable(tests[i].result) << "\n\n";
	}

	assert(ss.str() == tests[i].result);
    }

    for (unsigned int i=0; i<NTESTS2; ++i)
    {
	util::http::Request rq;
	rq.path = tests2[i].url;
	util::http::Response rs;
	bool found = rcf.StreamForPath(&rq,&rs);

	assert(found);
	
	util::StringStream ss;
	util::CopyStream(rs.body_source.get(), &ss);

	std::string expected(tests2[i].result, tests2[i].resultsize);

	if (ss.str() != expected)
	{
	    TRACE << "URL " << tests2[i].url << " gave:\n"
		  << Printable(ss.str()) 
		  << "\nshould be\n" 
		  << Printable(expected) << "\n\n";
	}

	assert(ss.str() == expected);
    }
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

    util::http::Client client;
    db::local::Database mdb(&sdb, &client);
    DoTests(&mdb);

    db::merge::Database mergedb;
    mergedb.AddDatabase(&mdb);
    DoTests(&mergedb);

    return 0;
}

#endif
