#ifndef LIBUPNP_DIDL_H
#define LIBUPNP_DIDL_H 1

#include <map>
#include <string>
#include <list>
#include "libdb/db.h"

namespace mediadb { class Database; }

namespace upnp {

/** Classes implementing DIDL, a metadata standard used in UPnP.
 */
namespace didl {

typedef std::map<std::string, std::string> Attributes;

struct Field {
    std::string tag;
    Attributes attributes;
    std::string content;
};

typedef std::list<Field> Metadata;
typedef std::list<Metadata> MetadataList;

MetadataList Parse(const std::string&);

/** Construct a DIDL fragment representing the contents of the recordset.
 *
 * The result is a single DIDL <%item> or <%container>; to make it valid DIDL,
 * you need to prepend s_header and append s_footer.
 *
 * Assumes the recordset has the standard MediaDB schema.
 */
std::string FromRecord(mediadb::Database *db, db::RecordsetPtr rs,
		       const char *urlprefix = NULL);

extern const char s_header[];
extern const char s_footer[];

} // namespace didl
} // namespace upnp

#endif
