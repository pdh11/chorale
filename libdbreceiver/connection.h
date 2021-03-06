#ifndef LIBDBRECEIVER_CONNECTION_H
#define LIBDBRECEIVER_CONNECTION_H 1

#include "libutil/ip.h"
#include <set>
#include <map>
#include <memory>

namespace util { class Stream; }
namespace util { namespace http { class Client; } }

namespace db {

/** A database representing the files on a remote Rio Receiver server.
 *
 * In other words, this is Chorale's implementation of the client end of the
 * Rio Receiver protocol, for which see http://www.dasnet.org/node/69
 */
namespace receiver {

class Recordset;
class CollateRecordset;
class RestrictionRecordset;

/** Connection to a remote Receiver server
 */
class Connection
{
    util::http::Client *m_http;
    util::IPEndPoint m_ep;

    std::set<int> m_has_tags;
    std::map<int, int> m_server_to_media_map;
    std::map<int, std::string> m_tag_to_fieldname_map;

    bool HasTag(int which);

//    std::map<int, int> m_id_to_type_map;
//    std::map<int, std::string> m_id_to_title_map;

    friend class Recordset;
    friend class CollateRecordset;
    friend class RestrictionRecordset;

public:
    explicit Connection(util::http::Client *http);
    ~Connection();

    unsigned int Init(const util::IPEndPoint&);

    std::string GetFieldName(int mediadbtag) { return m_tag_to_fieldname_map[mediadbtag]; }

    std::string GetURL(unsigned int id);
    std::unique_ptr<util::Stream> OpenRead(unsigned int id);
};

} // namespace receiver
} // namespace db

#endif
