#include "libkarma/db_writer.h"
#include "libmediadb/schema.h"
#include "libdb/recordset.h"
#include "libutil/counted_pointer.h"
#include "libutil/compare.h"
#include "libutil/stream.h"

#include <map>

#include <string.h>
#include <time.h>

namespace karma {

namespace {

enum FieldType {
    FIELD_INT32  = 0,
    FIELD_STRING = 1,
    FIELD_BINARY = 2,
    FIELD_UINT8  = 3,
    FIELD_UINT16 = 4
};

enum FieldFlags {
    FIELD_INDEXED = 1
};

const unsigned NO_FIELD(unsigned(-1));

const struct {
    unsigned mediaField;
    const char *name;
    FieldType type;
    unsigned flags;
} g_fields[] = {
    { mediadb::ID,     "fid",      FIELD_INT32,  FIELD_INDEXED },
    { NO_FIELD,        "offset",   FIELD_INT32,  0 },
    { NO_FIELD,        "trailer",  FIELD_INT32,  0 },
    { mediadb::DURATIONMS,  "duration", FIELD_INT32, 0 },
    { mediadb::SIZEBYTES,   "length",   FIELD_INT32, 0 },
    { mediadb::SAMPLERATE,  "samplerate", FIELD_INT32, 0 },
    { mediadb::CTIME,       "ctime",      FIELD_INT32, FIELD_INDEXED },
    { NO_FIELD,             "options",    FIELD_INT32, 0 },
    { NO_FIELD,             "bpm",        FIELD_INT32, 0 },
    { NO_FIELD,             "rms",        FIELD_INT32, 0 },
    { NO_FIELD,             "stddev",     FIELD_INT32, 0 },
    { NO_FIELD,             "normalisation", FIELD_INT32, 0 },
    { NO_FIELD,             "play_last",     FIELD_INT32, FIELD_INDEXED },
    { NO_FIELD,             "play_count_limit", FIELD_UINT16, 0 },
    { NO_FIELD,             "play_count",       FIELD_UINT16, 0 },
    { mediadb::TRACKNUMBER, "tracknr",          FIELD_UINT8, FIELD_INDEXED },
    { NO_FIELD,             "file_id",          FIELD_UINT8, 0 },
    { NO_FIELD,             "marked",           FIELD_UINT8, 0 },
    { mediadb::PLAYLIST,    "playlist",         FIELD_BINARY, 0 },
    { NO_FIELD,             "profile",          FIELD_BINARY, 0 },
    { mediadb::TYPE,        "type",             FIELD_STRING, 0 },
    { mediadb::AUDIOCODEC,  "codec",            FIELD_STRING, 0 },
    { mediadb::ARTIST,      "artist",   FIELD_STRING, 0 },
    { mediadb::ALBUM,       "source",   FIELD_STRING, 0 },
    { mediadb::TITLE,       "title",            FIELD_STRING, 0 },
    { mediadb::BITSPERSEC,  "bitrate",          FIELD_STRING, 0 },
    { mediadb::GENRE,       "genre",            FIELD_STRING, 0 },
    { mediadb::YEAR,        "year",             FIELD_STRING, 0 },
    { NO_FIELD,             "rid",              FIELD_STRING, 0 },
    { NO_FIELD,             "fid_generation",   FIELD_INT32, 0 },
    { NO_FIELD,             "profiler_version", FIELD_INT32, 0 },
    { NO_FIELD,             "replay_gain_peak", FIELD_INT32, 0 },
    { NO_FIELD,             "replay_gain_radio",FIELD_INT32, 0 },
    { NO_FIELD,             "replay_gain_audiophile", FIELD_INT32, 0 },
    { NO_FIELD,             "bytes_silence_sof", FIELD_INT32, 0 },
    { NO_FIELD,             "bytes_silence_eof", FIELD_INT32, 0 },
};

const unsigned g_nFields = sizeof(g_fields)/sizeof(g_fields[0]);

struct Comparator
{
    bool operator()(const std::string& s1, const std::string& s2) const
    {
        const int cmp(util::Compare(s1.c_str(), s2.c_str(), false));
        return cmp < 0;
    }
};

} // anon namespace

DBWriter::DBWriter()
    : m_nRecords(0)
{
    m_fields.resize(g_nFields);
}

DBWriter::~DBWriter()
{
}

void DBWriter::addRecord(const db::RecordsetPtr& rs)
{
    for (unsigned int i=0; i<g_nFields; ++i) {
        int32_t ival = 0;
        std::string sval;
        switch (g_fields[i].type) {
        case FIELD_INT32:
        case FIELD_UINT16:
        case FIELD_UINT8:
            if (g_fields[i].mediaField != NO_FIELD) {
                ival = rs->GetInteger(g_fields[i].mediaField);
            } else if (!strcmp(g_fields[i].name, "fid_generation")) {
                ival = 1;
            }
            m_fields[i].intdata.push_back(ival);
            break;
        case FIELD_STRING:
        case FIELD_BINARY:
            if (g_fields[i].mediaField != NO_FIELD) {
                sval = rs->GetString(g_fields[i].mediaField);
            }
            m_fields[i].stringdata.push_back(sval);
            break;
        }
    }
    ++m_nRecords;
}

unsigned DBWriter::write(util::Stream *f)
{
    const uint32_t fileformat = 2;
    unsigned rc = f->WriteAll(&fileformat, 4);

    const uint32_t timestamp = uint32_t(time(NULL));
    if (!rc) rc = f->WriteAll(&timestamp, 4);

    const uint32_t schemaMajor = 8;
    if (!rc) rc = f->WriteAll(&schemaMajor, 4);

    const uint32_t schemaMinor = 3;
    if (!rc) rc = f->WriteAll(&schemaMinor, 4);

    // Field definitions

    const uint32_t nFields = g_nFields;
    if (!rc) rc = f->WriteAll(&nFields, 4);

    std::string fields = "";

    for (unsigned int i=0; i<nFields; ++i) {
        const uint32_t type = g_fields[i].type;
        if (!rc) rc = f->WriteAll(&type, 4);

        const uint32_t flags = g_fields[i].flags;
        if (!rc) rc = f->WriteAll(&flags, 4);
        
        fields += g_fields[i].name;
        fields += ' ';
    }

    const uint32_t len(uint32_t(fields.size()));
    if (!rc) rc = f->WriteAll(&len, 4);
    if (!rc) rc = f->WriteAll(fields.c_str(), len);

    // Cache-records (none)

    const uint32_t nCacheRecords = 0;
    if (!rc) rc = f->WriteAll(&nCacheRecords, 4);

    // Data pools

    const uint32_t nPoolRecords(m_nRecords);
    if (!rc) rc = f->WriteAll(&nPoolRecords, 4);
    if (!rc) rc = f->WriteAll(&nPoolRecords, 4);

    for (unsigned i=0; i<nFields; ++i) {
        switch (g_fields[i].type) {
        case FIELD_INT32:
            for (unsigned int j=0; j<m_nRecords; ++j) {
                const int32_t value(m_fields[i].intdata[j]);
                if (!rc) rc = f->WriteAll(&value, 4);
            }
            break;
        case FIELD_UINT16:
            for (unsigned int j=0; j<m_nRecords; ++j) {
                const uint16_t value(uint16_t(m_fields[i].intdata[j]));
                if (!rc) rc = f->WriteAll(&value, 2);
            }
            break;
        case FIELD_UINT8:
            for (unsigned int j=0; j<m_nRecords; ++j) {
                const uint8_t value(uint8_t(m_fields[i].intdata[j]));
                if (!rc) rc = f->WriteAll(&value, 1);
            }
            break;
        case FIELD_STRING:
            if (!rc) rc = writeStringPool(f, i);
            break;
        case FIELD_BINARY:
            if (!rc) rc = writeBinaryPool(f, i);
            break;
        }
    }

    // Deleted records (none)

    if (!rc) rc = f->WriteAll(&nCacheRecords, 4);

    // Timestamp again

    if (!rc) rc = f->WriteAll(&timestamp, 4);

    return rc;
}

unsigned DBWriter::writeStringPool(util::Stream *f, unsigned field)
{
    typedef std::vector<uint32_t> indexes_t;
    typedef std::map<std::string, indexes_t, Comparator> pool_t;

    pool_t pool;

    for (unsigned int i=0; i<m_nRecords; ++i) {
        std::string s(m_fields[field].stringdata[i]);
        indexes_t& ix = pool[s];
        ix.push_back(i);
    }

    m_fields[field].intdata.resize(m_nRecords);
    
    std::string allValues = "";
    
    for (pool_t::const_iterator i = pool.begin(); i != pool.end(); ++i) {
        const unsigned offset(unsigned(allValues.length()));
        allValues += i->first;
        allValues += '\0';
        for (indexes_t::const_iterator j = i->second.begin();
             j != i->second.end();
             ++j) {
            m_fields[field].intdata[*j] = offset;
        }
    }

    const uint32_t len(uint32_t(allValues.length()));
    unsigned rc = f->WriteAll(&len, 4);

    for (unsigned int i=0; i<m_nRecords; ++i) {
        const uint32_t offset(m_fields[field].intdata[i]);
        if (!rc) rc = f->WriteAll(&offset, 4);
    }

    if (!rc) rc = f->WriteAll(allValues.c_str(), len);
    return rc;
}

unsigned DBWriter::writeBinaryPool(util::Stream *f, unsigned field)
{
    std::string allValues = "";
    unsigned rc = 0;
    for (unsigned int i=0; i<m_nRecords; ++i) {
        const uint32_t offset(uint32_t(allValues.length()));
        allValues += m_fields[field].stringdata[i];
        if (!rc) rc = f->WriteAll(&offset, 4);
    }

    const uint32_t len(uint32_t(allValues.length()));
    if (!rc) rc = f->WriteAll(&len, 4);
    if (!rc) rc = f->WriteAll(allValues.c_str(), len);

    return 0;
}

} // namespace karma
