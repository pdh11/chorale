#ifndef LIBDB_RECORDSET_H
#define LIBDB_RECORDSET_H 1

#include <stdint.h>
#include <string>
#include "libutil/counted_object.h"

namespace db {

/** Models a cursor into the results of a query.
 *
 * Note that there is no separate class for a "Record" -- as all records are
 * accessed via cursors, it's simpler to elide cursor->record->getfield into
 * just cursor->getfield.
 */
class Recordset: public util::CountedObject
{
public:
    virtual ~Recordset() {}
    virtual bool IsEOF() const = 0;

    virtual uint32_t GetInteger(unsigned int which) const = 0;
    virtual std::string GetString(unsigned int which) const = 0;

    virtual unsigned int SetInteger(unsigned int which, uint32_t value) = 0;
    virtual unsigned int SetString(unsigned int which,
				   const std::string& value) = 0;

    virtual void MoveNext() = 0;
    virtual unsigned int AddRecord() = 0;
    virtual unsigned int Commit() = 0;

    /** Deletes current record, if any. Implicitly does MoveNext.
     */
    virtual unsigned int Delete() = 0;
};

} // namespace db

#endif
