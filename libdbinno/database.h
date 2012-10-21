#ifndef LIBDBINNO_DATABASE_H
#define LIBDBINNO_DATABASE_H 1

#include "libdb/db.h"
#include <stdint.h>

namespace db {

/** Database implementation using Embedded InnoDB library.
 */
namespace inno {

enum {
    FIELD_STRING   = 0x0, ///< The default type
    FIELD_INT      = 0x1,
    FIELD_TYPEMASK = 0x7,
    FIELD_INDEXED  = 0x8
};

struct FieldInfo {
    unsigned int flags;
};

class InnoDBUser;

class Database: public db::Database
{
    InnoDBUser *m_user;
    uint64_t m_table_id;

public:
    Database();
    ~Database();

    /** Embedded InnoDB uses global variables, and so only allows for
     * one instance per binary. As we only use a flatfile table, we
     * implement having several db::Databases by having one InnoDB
     * "database" with several *tables*.
     */
    unsigned int Init(const char *filename,
		      const char *tablename,
		      const FieldInfo *fields,
		      unsigned int num_fields);

    // Being a db::Database
    RecordsetPtr CreateRecordset();
    QueryPtr CreateQuery();
};

} // namespace inno
} // namespace db

#endif
