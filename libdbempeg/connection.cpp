#include "connection.h"
#include "libdb/query.h"
#include "libdb/recordset.h"
#include "libutil/trace.h"
#include "libempeg/fid_stream.h"
#include "libmediadb/xml.h"
#include "libmediadb/schema.h"
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>

namespace db {
namespace empeg {

unsigned Connection::sm_empeg_generation = 0;
  
Connection::Connection(util::http::Server *server)
    : m_server(server),
      m_db(mediadb::FIELD_COUNT),
      m_aid(&m_db),
      m_generation(sm_empeg_generation++),
      m_writing(false)
{
    std::string prefix = (boost::format("/empeg/%u/") % m_generation).str();
    server->AddContentFactory(prefix, this);

    m_db.SetFieldInfo(mediadb::ID, 
		      db::steam::FIELD_INT|db::steam::FIELD_INDEXED);
    m_db.SetFieldInfo(mediadb::PATH,
		      db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    m_db.SetFieldInfo(mediadb::ARTIST,
		      db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    m_db.SetFieldInfo(mediadb::ALBUM,
		      db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    m_db.SetFieldInfo(mediadb::GENRE,
		      db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    m_db.SetFieldInfo(mediadb::TITLE,
		      db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    m_db.SetFieldInfo(mediadb::REMIXED,
		      db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    m_db.SetFieldInfo(mediadb::ORIGINALARTIST,
		      db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    m_db.SetFieldInfo(mediadb::MOOD,
		      db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    m_db.SetFieldInfo(mediadb::TRACKNUMBER,
		      db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
}

Connection::~Connection()
{
    if (m_writing)
    {
	/// @todo Build database fids on PC
	m_client.RebuildDatabase();
	m_client.RestartPlayer();
    }
}

/** @todo This is duplicated in libdbreceiver/db.cpp
 */
static const struct {
    const char *empegtag;
    int choraletag;
} tagmap[] = {
    { "fid", mediadb::ID },
    { "tracknr", mediadb::TRACKNUMBER },
    { "length", mediadb::SIZEBYTES },
    { "duration", mediadb::DURATIONMS },
    { "mtime", mediadb::MTIME },
    { "ctime", mediadb::CTIME },
    { "samplerate", mediadb::SAMPLERATE },
    { "bitspersec", mediadb::BITSPERSEC },
    { "channels", mediadb::CHANNELS },
    { "playlist", mediadb::CHILDREN }, ///< Different representation
    { "type", mediadb::TYPE }, ///< Different representation
    { "title", mediadb::TITLE },
    { "artist", mediadb::ARTIST },
    { "year", mediadb::YEAR },
    { "genre", mediadb::GENRE },
    { "comment", mediadb::COMMENT },
    { "source",         mediadb::ALBUM },
    { "codec",          mediadb::AUDIOCODEC }, ///< Different representation
    { "mood",           mediadb::MOOD },
    { "composer",       mediadb::COMPOSER },
    { "lyricist",       mediadb::LYRICIST },
    { "conductor",      mediadb::CONDUCTOR },
    { "ensemble",       mediadb::ENSEMBLE },
    { "remixed",        mediadb::REMIXED },
    { "originalartist", mediadb::ORIGINALARTIST },
    { "bitrate",        mediadb::BITSPERSEC }, ///< Different representation
};

unsigned int Connection::Init(const util::IPAddress& ip)
{
    unsigned int rc = m_client.Init(ip);
    if (rc != 0)
	return rc;

    uint32_t size;
    boost::scoped_array<char> buffer;

    /* Load tags */
    
    rc = m_client.ReadFidToBuffer(::empeg::FID_TAGS, &size, &buffer);
    if (rc != 0)
    {
	TRACE << "Can't read tags " << rc << "\n";
	return rc;
    }
    std::string tagstring(buffer.get(), buffer.get() + size);

    std::map<std::string, int> tagmap2;

    for (unsigned int i=0; i<(sizeof(tagmap)/sizeof(tagmap[0])); ++i)
    {
	tagmap2[tagmap[i].empegtag] = tagmap[i].choraletag;
    }

    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    boost::char_separator<char> sep("\n");
    tokenizer tok(tagstring, sep);

    for (tokenizer::iterator i = tok.begin(); i != tok.end(); ++i)
    {
	if (i->empty())
	    break;

	m_field_names.push_back(*i);
	
	std::map<std::string, int>::iterator j = tagmap2.find(*i);
	if (j == tagmap2.end())
	    m_field_nos.push_back(-1);
	else
	    m_field_nos.push_back(j->second);
    }

    /* Load database / playlists */

    rc = m_client.ReadFidToBuffer(::empeg::FID_DATABASE, &size, &buffer);
    if (rc != 0)
    {
	TRACE << "Can't read tags " << rc << "\n";
	return rc;
    }

    db::RecordsetPtr rsp = m_db.CreateRecordset();

    uint32_t fid = 0;
    unsigned char *ptr = (unsigned char*)buffer.get();
    unsigned char *end = ptr + size;

    uint32_t psize;
    boost::scoped_array<char> playlists;
    rc = m_client.ReadFidToBuffer(::empeg::FID_PLAYLISTS, &psize, &playlists);
    if (rc != 0)
    {
	TRACE << "Can't read playlists " << rc << "\n";
	return rc;
    }
    unsigned char *pptr = (unsigned char*)playlists.get();
    unsigned char *pend = pptr + psize;

    while (ptr < end)
    {
	unsigned char field = *ptr++;

	if (field == 0xFF)
	{
	    fid += 0x10;
	    continue;
	}

	rsp->AddRecord();
	rsp->SetInteger(mediadb::ID, fid);

	while (field != 0xFF)
	{
	    unsigned char len = *ptr++;
	    std::string value((char*)ptr, (char*)(ptr+len));
	    ptr += len;

	    if (field >= m_field_nos.size())
	    {
		TRACE << "Bogus field " << field << " (max " 
		      << m_field_nos.size() << "\n";
	    }
	    else
	    {
		int choralefield = m_field_nos[field];

		/* Some fields have different representation on Empegs */
		switch (choralefield)
		{
		case mediadb::TYPE:
		    if (value == "playlist")
			rsp->SetInteger(mediadb::TYPE, mediadb::PLAYLIST);
		    else if (value == "tune")
			rsp->SetInteger(mediadb::TYPE, mediadb::TUNE);
		    else
			rsp->SetInteger(mediadb::TYPE, mediadb::FILE);
		    break;

		case mediadb::AUDIOCODEC:
		    if (value == "mp3")
			rsp->SetInteger(mediadb::AUDIOCODEC, mediadb::MP3);
		    else if (value == "flac")
			rsp->SetInteger(mediadb::AUDIOCODEC, mediadb::FLAC);
		    else
			rsp->SetInteger(mediadb::AUDIOCODEC, mediadb::NONE);
		    break;
		    
		default:
		    if (choralefield >= 0)
			rsp->SetString(choralefield, value);
		    break;
		}
	    }
	    field = *ptr++;
	}

//	TRACE << "FID " << fid << " title=" << rsp->GetString(mediadb::TITLE)
//	      << "\n";

	if (rsp->GetInteger(mediadb::TYPE) == mediadb::PLAYLIST)
	{
	    unsigned int plen = rsp->GetInteger(mediadb::SIZEBYTES);
	    if (plen % 4)
	    {
		TRACE << "FID " << rsp->GetInteger(mediadb::ID)
		      << " funny playlist length " << plen << "\n";
	    }
	    else
	    {
		std::vector<unsigned int> children;

		for (unsigned int i=0; i<plen/4; ++i)
		{
                    unsigned int child = 0;
                    memcpy(&child, pptr + i*4, 4);
		    children.push_back(i);
		}

		rsp->SetString(mediadb::CHILDREN,
			       mediadb::VectorToChildren(children));

		pptr += plen;

//		TRACE << "FID " << rsp->GetInteger(mediadb::ID) << ": "
//		      << (plen/4) << " children\n";
	    }
	}

	rsp->Commit();
	fid += 0x10;
    }

    if (pptr != pend)
	TRACE << "Playlist mismatch: " << (pptr-pend) << " bytes\n";

    TRACE << "DB loaded\n";

    FILE *f = fopen("dbempeg.xml", "w+");
    if (f)
    {
	mediadb::WriteXML(&m_db, mediadb::SCHEMA_VERSION, f);
	fclose(f);
    }
    
    return 0;
}

unsigned int Connection::EnsureWritable()
{
    if (m_writing)
	return 0;

    TRACE << "Locking UI\n";
    unsigned int rc = m_client.LockUI(true);
    TRACE << "Lock returned " << rc << "\n";

    TRACE << "Remounting rw\n";
    rc = m_client.EnableWrite(true);
    TRACE << "Remount returned " << rc << "\n";
    if (!rc)
	m_writing = true;
    return rc;
}

unsigned int Connection::Delete(db::RecordsetPtr rs)
{
    unsigned int rc = EnsureWritable();
    if (rc)
	return rc;

//    TRACE << "Chickening out of deleting\n";
//    return 0;
    
    return m_client.Delete(rs->GetInteger(mediadb::ID));
}

const char *Connection::ChoraleTypeToEmpegType(unsigned int ctype)
{
    assert(ctype < mediadb::TYPE_COUNT);

    static const char *const array[] = {
	"file",
	"tune",
	"tune",
	"tune",
	"playlist",
	"playlist",
	"file",
	"file",
	"tune",
	"tune",
        "file",
        "file"
    };

    BOOST_STATIC_ASSERT(sizeof(array)/sizeof(*array) == mediadb::TYPE_COUNT);

    return array[ctype];
}

const char *Connection::ChoraleCodecToEmpegCodec(unsigned int ccodec)
{
    assert(ccodec < mediadb::AUDIOCODEC_COUNT);

    static const char *const array[] = {
	"none",
	"mp3",
	"mp3",
	"flac",
	"vorbis",
	"wave",
	"wave",
	"none",
        "none"
    };

    BOOST_STATIC_ASSERT(sizeof(array)/sizeof(*array) == mediadb::AUDIOCODEC_COUNT);

    return array[ccodec];
}

static void AddIfNotEmpty(std::string *s, const char *key, const std::string& value)
{
    if (!value.empty())
    {
	*s += key;
	*s += '=';
	*s += value;
	*s += '\n';
    }
}

static void AddIfNonZero(std::string *s, const char *key, unsigned int value)
{
    if (value)
    {
	*s += (boost::format("%s=%u\n") % key % value).str();
    }
}

unsigned int Connection::RewriteData(db::RecordsetPtr rs)
{
    unsigned int rc = EnsureWritable();
    if (rc)
	return rc;

    unsigned int type = rs->GetInteger(mediadb::TYPE);
    uint32_t fid = rs->GetInteger(mediadb::ID);

    std::vector<unsigned int> children;
    unsigned int length = rs->GetInteger(mediadb::SIZEBYTES);
    if (type == mediadb::PLAYLIST || type == mediadb::DIR)
    {
	mediadb::ChildrenToVector(rs->GetString(mediadb::CHILDREN),
				  &children);
	length = (unsigned int)children.size() * 4;
    }

    std::string onefile;
    if (type == mediadb::TUNE)
    {
	onefile = (boost::format("type=tune\n"
				 "title=%s\n"
				 "length=%u\n"
				 "bitrate=%c%c%u\n"
				 "codec=%s\n"
				 "duration=%u\n"
				 "samplerate=%u\n")
		   % rs->GetString(mediadb::TITLE)
		   % length
		   % 'v'
		   % 's'
		   % (rs->GetInteger(mediadb::BITSPERSEC) / 1000)
		   % ChoraleCodecToEmpegCodec(rs->GetInteger(mediadb::AUDIOCODEC))
		   % rs->GetInteger(mediadb::DURATIONMS)
		   % rs->GetInteger(mediadb::SAMPLERATE)).str();
	AddIfNotEmpty(&onefile, "artist", rs->GetString(mediadb::ARTIST));
	AddIfNotEmpty(&onefile, "source", rs->GetString(mediadb::ALBUM));
	AddIfNotEmpty(&onefile, "genre", rs->GetString(mediadb::GENRE));
	AddIfNonZero(&onefile, "year", rs->GetInteger(mediadb::YEAR));
	AddIfNonZero(&onefile, "tracknr", rs->GetInteger(mediadb::TRACKNUMBER));
    }
    else
    {
	onefile = (boost::format("type=%s\n"
				 "title=%s\n"
				 "length=%u\n")
		   % ChoraleTypeToEmpegType(type)
		   % rs->GetString(mediadb::TITLE)
		   % length).str();
    }

    TRACE << "FID " << fid << " *1 file:\n" << onefile;

    rc = m_client.WriteFidFromString(fid | 1, onefile);
    TRACE << "Wrote FID " << (fid|1) << " rc=" << rc << "\n";
    if (rc)
	return rc;
    
    if (type == mediadb::PLAYLIST || type == mediadb::DIR)
    {
	rc = m_client.Prepare(fid, length);
	uint32_t nwritten;
	if (!rc)
	    rc = m_client.StartFastWrite(fid, length);
	if (!rc)
	    rc = m_client.Write(fid, 0, length, &children[0], &nwritten);
    }

    return rc;
}

bool Connection::StreamForPath(const util::http::Request *rq,
				   util::http::Response *rs)
{
    unsigned int generation, fid;
    if (sscanf(rq->path.c_str(), "/empeg/%u/%u", &generation, &fid) == 2
	&& generation == m_generation)
    {
	rs->body_source = OpenRead(fid);
	return true;
    }
    return false; 
}

std::unique_ptr<util::Stream> Connection::OpenWrite(unsigned int id)
{
    std::unique_ptr<util::Stream> sp;

    unsigned int rc = EnsureWritable();
    if (rc)
	return sp;
    rc = ::empeg::FidStream::CreateWrite(&m_client, id, &sp);
    if (rc)
    {
	TRACE << "Can't open FID " << id << " for writing\n";
    }
    return sp;
}

std::unique_ptr<util::Stream> Connection::OpenRead(unsigned int id)
{
    std::unique_ptr<util::Stream> result;
    unsigned int rc = ::empeg::FidStream::CreateRead(&m_client, id, &result);
    (void)rc;
    return result;
}

std::string Connection::GetURL(unsigned int id)
{
    /// @todo Leave it up to the Queue to choose the IP address
    return (boost::format("http://127.0.0.1:%u/empeg/%u/%u")
	    % m_server->GetPort()
	    % m_generation
	    % id).str();
}

db::QueryPtr Connection::CreateLocalQuery()
{
    return m_db.CreateQuery();
}

db::RecordsetPtr Connection::CreateLocalRecordset()
{
    return m_db.CreateRecordset();
}

} // namespace db::empeg
} // namespace db
