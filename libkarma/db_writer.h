#ifndef LIB_KARMA_DB_WRITER_H
#define LIB_KARMA_DB_WRITER_H 1

#include "libdb/db.h"

#include <vector>
#include <string>
#include <stdint.h>

namespace util { class Stream; }

namespace karma {

struct Field 
{
    std::vector<int32_t> intdata;
    std::vector<std::string> stringdata;
};

class DBWriter
{
    std::vector<Field> m_fields;
    unsigned m_nRecords;

    unsigned writeStringPool(util::Stream*, unsigned field);
    unsigned writeBinaryPool(util::Stream*, unsigned field);

public:
    DBWriter();
    ~DBWriter();

    void addRecord(const db::RecordsetPtr&);

    unsigned write(util::Stream*);
};

} // namespace karma

#endif
