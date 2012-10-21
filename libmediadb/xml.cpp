#include "config.h"
#include "xml.h"
#include "libdb/recordset.h"
#include "schema.h"
#include <string>
#include <boost/static_assert.hpp>
#include "libutil/trace.h"
#include "libutil/file_stream.h"
#include "libutil/xml.h"
#include "libutil/xmlescape.h"
#include <stdio.h>

LOG_DECL(DBLOCAL);

namespace mediadb {

static const char *const tagmap[] = {
    "id",
    "title",
    "artist",
    "album",
    "tracknumber",
    "genre",
    "comment",
    "year",
    "durationms",
    "codec",
    "sizebytes",
    "bitspersec",
    "samplerate",
    "channels",
    "path",
    "mtime",
    "ctime",
    "type",
    "mood",
    "originalartist",
    "remixed",
    "conductor",
    "composer",
    "ensemble",
    "lyricist",
    "children",
    "idhigh",
    "idparent",
};

enum { NTAGS = sizeof(tagmap)/sizeof(tagmap[0]) };

BOOST_STATIC_ASSERT((int)NTAGS == (int)mediadb::FIELD_COUNT);

static const char *const typemap[] = {
    "",
    "tune",
    "tunehigh",
    "spoken",
    "playlist",
    "dir",
    "image",
    "video",
    "radio",
    "pending"
};

enum { NTYPES = sizeof(typemap)/sizeof(typemap[0]) };

BOOST_STATIC_ASSERT((int)NTYPES == (int)mediadb::TYPE_COUNT);

static const char *const codecmap[] = {
    "",
    "mp2",
    "mp3",
    "flac",
    "vorbis",
    "wav",
    "pcm",
    "jpeg"
};

enum { NCODECS = sizeof(codecmap)/sizeof(codecmap[0]) };

BOOST_STATIC_ASSERT((int)NCODECS == (int)mediadb::CODEC_COUNT);

unsigned int WriteXML(db::Database *db, unsigned int schema, ::FILE *f)
{
    fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(f, "<db schema=\"%u\">\n", schema);

    for (db::RecordsetPtr rs = db->CreateRecordset();
	 !rs->IsEOF();
	 rs->MoveNext())
    {
	// Don't write special IDs
	if (rs->GetInteger(mediadb::ID) < 0x100)
	    continue;

	// Don't write radio stations
	if (rs->GetInteger(mediadb::TYPE) == mediadb::RADIO)
	    continue;

	fprintf(f, "<record>\n");
	for (unsigned int i=0; i<mediadb::FIELD_COUNT; ++i)
	{
	    std::string value;

	    /** Yes, i know this is a for-switch loop, but only a very few
	     * i's need any special treatment.
	     */
	    switch (i)
	    {
	    case mediadb::TYPE:
		value = typemap[rs->GetInteger(i)];
		break;
	    case mediadb::CODEC:
		value = codecmap[rs->GetInteger(i)];
		break;
	    case mediadb::CHILDREN:
		value = rs->GetString(i);
		if (!value.empty())
		{
		    fprintf(f, "<children>\n");
		    std::vector<unsigned int> vec;
		    mediadb::ChildrenToVector(value, &vec);
		    for (unsigned int j=0; j<vec.size(); ++j)
		    {
			if (vec[j] >= 0x100)
			    fprintf(f, "  <child>%u</child>\n", vec[j]);
		    }
		    fprintf(f, "</children>\n");
		    value.clear();
		}
		break;
	    default:
		value = rs->GetString(i);
		break;
	    }

	    if (!value.empty())
	    {
		value = util::XmlEscape(value);
		fprintf(f, "<%s>%s</%s>\n", tagmap[i], value.c_str(),
			tagmap[i]);
	    }
	}
	fprintf(f, "</record>\n\n");
    }

    fprintf(f, "</db>\n");
    if (ferror(f))
	return (unsigned)errno;
    return 0;
}

class MediaDBParser: public xml::SaxParserObserver
{
    db::Database *m_db;
    unsigned int m_field;
    std::vector<unsigned int> m_children;
    bool m_in_child;
    std::map<std::string, unsigned int> m_rev_typemap;
    std::map<std::string, unsigned int> m_rev_codecmap;
    std::string m_value;
    std::set<uint32_t> m_ids;
    std::string m_fields[mediadb::FIELD_COUNT];

public:
    explicit MediaDBParser(db::Database *thedb)
	: m_db(thedb), m_field(mediadb::FIELD_COUNT), m_in_child(false)
    {
	for (unsigned int i=0; i < NTYPES; ++i)
	    m_rev_typemap[typemap[i]] = i;
	for (unsigned int i=0; i < NCODECS; ++i)
	    m_rev_codecmap[codecmap[i]] = i;
    }

    unsigned int OnBegin(const char *tag)
    {
	std::string name = tag;

	m_field = mediadb::FIELD_COUNT;
	m_in_child = false;
	m_value.clear();

	if (name == "record")
	{
	    for (unsigned int i=0; i<mediadb::FIELD_COUNT; ++i)
		m_fields[i].clear();
	    m_children.clear();
	}
	else if (name == "child")
	{
	    m_in_child = true;
	}
	else
	{
	    for (unsigned int i=0; i<sizeof(tagmap)/sizeof(tagmap[0]); ++i)
	    {
		if (name == tagmap[i])
		{
		    m_field = i;
		    break;
		}
	    }
	}
	return 0;
    }

    unsigned int OnEnd(const char *tag)
    {
	std::string name = tag;

	if (m_field < mediadb::FIELD_COUNT)
	    m_fields[m_field] = m_value;
	else if (m_in_child)
	    m_children.push_back((unsigned)strtoul(m_value.c_str(), NULL, 10));

	m_field = mediadb::FIELD_COUNT;
	m_in_child = false;

	if (name == "record")
	{
	    uint32_t id = (uint32_t)strtoul(m_fields[mediadb::ID].c_str(),
					    NULL, 10);
	    
	    // Recover from duplicate IDs sometimes caused by old versions
	    if (m_ids.find(id) == m_ids.end())
	    {
		m_ids.insert(id);
		db::RecordsetPtr rs = m_db->CreateRecordset();
		rs->AddRecord();

		for (unsigned int i=0; i<mediadb::FIELD_COUNT; ++i)
		{
		    std::string value = m_fields[i];

		    if (i == mediadb::TYPE)
			rs->SetInteger(i, m_rev_typemap[value]);
		    else if (i == mediadb::CODEC)
			rs->SetInteger(i, m_rev_codecmap[value]);
		    else if (i == mediadb::CHILDREN)
			rs->SetString(i, 
				      mediadb::VectorToChildren(m_children));
		    else
			rs->SetString(i, value);
		}
//		TRACE << "Adding record " << id << "\n";

		LOG(DBLOCAL) << "id " << id << " is '"
			     << m_fields[mediadb::PATH] << "'\n";

		rs->Commit();
	    }
	    else
	    {
		TRACE << "Rejecting duplicate id " << id << "\n";
	    }
	}
	return 0;
    }

    unsigned int OnContent(const char *content)
    {
	if (m_field != mediadb::FIELD_COUNT || m_in_child)
	{
	    m_value += content;
	}
	return 0;
    }

    unsigned int OnAttribute(const char*, const char*) { return 0; }
};

unsigned int ReadXML(db::Database *db, const char *filename)
{
    util::SeekableStreamPtr ssp;
    unsigned int rc = util::OpenFileStream(filename, util::READ, &ssp);
    if (rc != 0)
	return rc;

    return ReadXML(db, ssp);
}

unsigned int ReadXML(db::Database *db, util::StreamPtr sp)
{
    MediaDBParser parser(db);
    xml::SaxParser saxp(&parser);
    return saxp.Parse(sp);
}

} // namespace mediadb


#ifdef TEST

# include "libdb/free_rs.h"
# include "libdb/query.h"

class TestDB: public db::Database
{
public:
    db::RecordsetPtr m_rs;

    TestDB() : m_rs(db::FreeRecordset::Create()) {}

    db::RecordsetPtr CreateRecordset() { return m_rs; }
    db::QueryPtr CreateQuery() { return db::QueryPtr(); }
};

int main()
{
    TestDB tdb;
    tdb.m_rs->SetInteger(mediadb::ID, 256);
    tdb.m_rs->SetString(mediadb::PATH, "You\xE2\x80\x99re My Flame.flac");
    tdb.m_rs->SetString(mediadb::ALBUM, "X&Y");
    tdb.m_rs->SetInteger(mediadb::TYPE, mediadb::TUNE);
    tdb.m_rs->SetInteger(mediadb::CODEC, mediadb::FLAC);

    std::vector<unsigned int> cv;
    cv.push_back(257);
    cv.push_back(258);
    tdb.m_rs->SetString(mediadb::CHILDREN,
			mediadb::VectorToChildren(cv));

    FILE *f = fopen("test.xml", "w+");
    mediadb::WriteXML(&tdb, 0, f);
    fclose(f);

    TestDB tdb2;
    mediadb::ReadXML(&tdb2, "test.xml");
    assert(tdb2.m_rs->GetInteger(mediadb::ID) == 256);
    assert(tdb2.m_rs->GetString(mediadb::PATH) == "You\xE2\x80\x99re My Flame.flac");
    assert(tdb2.m_rs->GetString(mediadb::ALBUM) == "X&Y");
    assert(tdb2.m_rs->GetInteger(mediadb::TYPE) == mediadb::TUNE);
    assert(tdb2.m_rs->GetInteger(mediadb::CODEC) == mediadb::FLAC);

    std::vector<unsigned int> cv2;
    mediadb::ChildrenToVector(tdb2.m_rs->GetString(mediadb::CHILDREN), &cv2);
    assert(cv2 == cv);

    unlink("test.xml");
    return 0;
}

#endif
