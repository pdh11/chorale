#include "db.h"
#include "libutil/trace.h"
#include "libutil/socket.h"
#include "libutil/http_fetcher.h"
#include "libutil/http_stream.h"
#include "query.h"
#include <boost/tokenizer.hpp>
#include <boost/format.hpp>
#include "libmediadb/schema.h"

LOG_DECL(RECEIVER);

namespace db {
namespace receiver {

Database::Database(util::http::Client *http)
    : m_http(http)
{
}

Database::~Database()
{
}

static const struct {
    const char *receivertag;
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
    { "codec",          mediadb::CODEC }, ///< Different representation
    { "mood",           mediadb::MOOD },
    { "composer",       mediadb::COMPOSER },
    { "lyricist",       mediadb::LYRICIST },
    { "conductor",      mediadb::CONDUCTOR },
    { "ensemble",       mediadb::ENSEMBLE },
    { "remixed",        mediadb::REMIXED },
    { "originalartist", mediadb::ORIGINALARTIST },
    { "bitrate",        mediadb::BITSPERSEC }, ///< Different representation
};

unsigned int Database::Init(const util::IPEndPoint& ep)
{
    m_ep = ep;

    std::map<std::string, int> tagmap2;

    for (unsigned int i=0; i<(sizeof(tagmap)/sizeof(tagmap[0])); ++i)
    {
	tagmap2[tagmap[i].receivertag] = tagmap[i].choraletag;
	m_tag_to_fieldname_map[tagmap[i].choraletag]
	    = tagmap[i].receivertag;
    }

    std::string url = "http://" + m_ep.ToString() + "/tags";
    util::http::Fetcher hc(m_http, url);
    std::string tagstring;
    unsigned int rc = hc.FetchToString(&tagstring);
    if (rc)
    {
	TRACE << "Can't fetch tags: " << rc << "\n";
	return rc;
    }

//	TRACE << "Tags are: '''" << tagstring << "'''\n";

    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

    boost::char_separator<char> sep("\n");

    tokenizer tok(tagstring, sep);

    unsigned int index = 0;
    for (tokenizer::iterator i = tok.begin(); i != tok.end(); ++i)
    {
	std::map<std::string, int>::iterator j = tagmap2.find(*i);
	if (j != tagmap2.end())
	{
	    int choraletag = j->second;
	    m_has_tags.insert(choraletag);
	    m_server_to_media_map[index] = choraletag;
	}
	else
	    m_server_to_media_map[index] = 0;

//	    TRACE << index << " " << m_server_to_media_map[index] << " "
//		  << *i << "\n";
	++index;
    }

    return 0;
}

db::RecordsetPtr Database::CreateRecordset()
{
    return db::RecordsetPtr();
}

db::QueryPtr Database::CreateQuery()
{
    return db::QueryPtr(new Query(this));
}

bool Database::HasTag(int which)
{
    return m_has_tags.count(which) != 0;
}

std::string Database::GetURL(unsigned int id)
{
    return "http://" + m_ep.ToString() + "/content/"
	+ (boost::format("%x") % id).str();
}

util::SeekableStreamPtr Database::OpenRead(unsigned int id)
{
    LOG(RECEIVER) << "OpenRead(" << id << ")\n";

    std::string url = GetURL(id);

    util::http::StreamPtr stm;
    unsigned rc = util::http::Stream::Create(&stm, m_http, url.c_str());
    if (rc)
	return util::SeekableStreamPtr();

    LOG(RECEIVER) << "OpenRead(" << id << ") returns stream for " << url
		  << "\n";
    return stm;
}

util::SeekableStreamPtr Database::OpenWrite(unsigned int)
{
    return util::SeekableStreamPtr();
}

} // namespace receiver
} // namespace db
