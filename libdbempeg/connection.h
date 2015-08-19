#ifndef LIBDBEMPEG_CONNECTION_H
#define LIBDBEMPEG_CONNECTION_H 1

#include "libutil/http_server.h"
#include "libutil/counted_pointer.h"
#include "libempeg/protocol_client.h"
#include "libdbsteam/db.h"
#include "libdb/query.h"
#include "allocate_id.h"

namespace db {
namespace empeg {

class Connection: public util::http::ContentFactory
{
    util::http::Server *m_server;
    ::empeg::ProtocolClient m_client;
    db::steam::Database m_db;
    db::empeg::AllocateID m_aid;

    /** All Empeg DB fields, in Empeg tag order */
    std::vector<std::string> m_field_names;

    /** mediadb::FIELD equivalents, in Empeg tag order (-1 = no equivalent) */
    std::vector<int> m_field_nos;

    unsigned m_generation;
    bool m_writing;

    static unsigned sm_empeg_generation;

    static const char *ChoraleCodecToEmpegCodec(unsigned int chorale_codec);
    static const char *ChoraleTypeToEmpegType(unsigned int chorale_type);

    unsigned int EnsureWritable();

public:
    explicit Connection(util::http::Server*);
    ~Connection();
    
    unsigned int Init(const util::IPAddress&);

    std::auto_ptr<util::Stream> OpenRead(unsigned int id);
    std::auto_ptr<util::Stream> OpenWrite(unsigned int id);
    std::string GetURL(unsigned int id);

    unsigned int AllocateID() { return m_aid.Allocate(); }

    unsigned int Delete(db::RecordsetPtr);
    unsigned int RewriteData(db::RecordsetPtr);

    db::QueryPtr CreateLocalQuery();
    db::RecordsetPtr CreateLocalRecordset();

    // Being a ContentFactory
    bool StreamForPath(const util::http::Request*, util::http::Response*);
};

} // namespace db::empeg
} // namespace empeg

#endif

