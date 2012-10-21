#include "config.h"
#include "xml.h"
#include "libdb/db.h"
#include "libdb/free_rs.h"
#include "schema.h"
#include <string>
#include <boost/static_assert.hpp>
#include <libxml++/libxml++.h>
#include "libutil/trace.h"
#include "libutil/xmlescape.h"
#include <stdio.h>

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
    "radio"
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
    "jpeg"
};

enum { NCODECS = sizeof(codecmap)/sizeof(codecmap[0]) };

BOOST_STATIC_ASSERT((int)NCODECS == (int)mediadb::CODEC_COUNT);

bool WriteXML(db::Database *db, unsigned int schema, ::FILE *f)
{
    fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(f, "<db schema=\"%u\">\n", schema);
    
    db::RecordsetPtr rs = db->CreateRecordset();

    while (!rs->IsEOF())
    {
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
			fprintf(f, "  <child>%u</child>\n", vec[j]);
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
	rs->MoveNext();
    }

    fprintf(f, "</db>\n");
    return !ferror(f);
}

class MediaDBParser: public xmlpp::SaxParser
{
    db::Database *m_db;
    int m_field;
    db::RecordsetPtr m_rs;
    std::vector<unsigned int> m_children;
    bool m_in_child;
    std::map<std::string, unsigned int> m_rev_typemap;
    std::map<std::string, unsigned int> m_rev_codecmap;
    std::string m_value;

public:
    explicit MediaDBParser(db::Database *thedb)
	: m_db(thedb), m_field(-1), m_in_child(false)
    {
	for (unsigned int i=0; i < NTYPES; ++i)
	    m_rev_typemap[typemap[i]] = i;
	for (unsigned int i=0; i < NCODECS; ++i)
	    m_rev_codecmap[codecmap[i]] = i;
    }

    void on_start_element(const Glib::ustring& name,
			  const AttributeList&)
    {
	m_field = -1;
	m_in_child = false;
	m_value.clear();

	if (name == "record")
	{
	    m_rs = m_db->CreateRecordset();
	    m_rs->AddRecord();
	}
	else if (name == "children")
	{
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
		    m_field = (int)i;
		    break;
		}
	    }
	}
    }

    void on_end_element(const Glib::ustring& name)
    {
	if (m_field >= 0)
	{
//	    TRACE << tagmap[m_field] << " = '" << m_value << "'\n";

	    if (m_rs)
	    {
		if (m_field == mediadb::TYPE)
		{
		    m_rs->SetInteger(m_field, m_rev_typemap[m_value]);
		}
		else if (m_field == mediadb::CODEC)
		{
		    m_rs->SetInteger(m_field, m_rev_codecmap[m_value]);
		}
		else
		{
		    m_rs->SetString(m_field, m_value);
		}
	    }
	}
	else if (m_in_child)
	{
	    m_children.push_back(atoi(m_value.c_str()));
	}

	m_field = -1;
	m_in_child = false;
	if (name == "record" && m_rs)
	{
	    m_rs->Commit();
	    m_rs.reset();
	}
	else if (name == "children")
	{
	    if (m_rs)
	    {
		m_rs->SetString(mediadb::CHILDREN,
				mediadb::VectorToChildren(m_children));
	    }
	}
    }

    void on_characters(const Glib::ustring& text)
    {
	if (m_field >= 0 || m_in_child)
	{
	    m_value += text.raw();
	}
    }
};

bool ReadXML(db::Database *db, const char *filename)
{
    try
    {
	MediaDBParser parser(db);
	parser.set_substitute_entities(true);
	parser.parse_file(filename);
    }
    catch (const xmlpp::exception& ex)
    {
	TRACE << "Caught libxml++ exception: " << ex.what() << "\n";
    }
    catch (...)
    {
	TRACE << "Caught something (a-choo)\n";
    }
    return true;
}

}; // namespace mediadb

#ifdef TEST

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
    assert(tdb2.m_rs->GetInteger(mediadb::TYPE) == mediadb::TUNE);
    assert(tdb2.m_rs->GetInteger(mediadb::CODEC) == mediadb::FLAC);

    std::vector<unsigned int> cv2;
    mediadb::ChildrenToVector(tdb2.m_rs->GetString(mediadb::CHILDREN), &cv2);
    assert(cv2 == cv);

    unlink("test.xml");

    return 0;
}

#endif
