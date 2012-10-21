#ifndef LIBUPNP_DIDL_H
#define LIBUPNP_DIDL_H 1

#include <map>
#include <string>
#include <list>

namespace util { template<class> class CountedPointer; }
namespace db { class Recordset; }
namespace db { typedef util::CountedPointer<Recordset> RecordsetPtr; }

namespace mediadb {

class Database;

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

/** Parse the DIDL XML into a more tractable form.
 */
MetadataList Parse(const std::string&);

/** Convert one object's data into the standard mediadb schema.
 *
 * Does not call AddRecord() or Commit(); the caller must do that. The caller
 * must also set the ID field itself; ToRecord doesn't handle the mapping from
 * (the server's arbitrary string) ObjectIDs to (mediadb integer) IDs.
 */
unsigned int ToRecord(const Metadata&, db::RecordsetPtr);

/** A filter for generating DIDL subsets.
 */
enum Filter {
    /* id, title and class can't be filtered out */
    SEARCHCLASS = 0x1,
    ARTIST = 0x2,
    ALBUM = 0x4,
    GENRE = 0x8,
    TRACKNUMBER = 0x10,
    AUTHOR = 0x20,
    DATE = 0x40,
    RES = 0x80,
    CHANNELID = 0x100,
    STORAGEUSED = 0x200,
    RES_SIZE = 0x400,
    RES_DURATION = 0x800,
    RES_PROTOCOL = 0x1000,

    ALL = 0x1FFF
};

/** Construct a DIDL fragment representing the contents of the recordset.
 *
 * The result is a single DIDL <%item> or <%container>; to make it valid DIDL,
 * you need to prepend s_header and append s_footer.
 *
 * Assumes that the recordset has the standard MediaDB schema.
 */
std::string FromRecord(mediadb::Database *db, db::RecordsetPtr rs,
		       const char *urlprefix = NULL,
		       unsigned int filter = ALL);

extern const char s_header[];
extern const char s_footer[];

} // namespace didl
} // namespace mediadb

#endif
