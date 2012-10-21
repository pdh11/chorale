#ifndef IMPORT_TAG_WRITER_H
#define IMPORT_TAG_WRITER_H

#include <string>

namespace db { class Recordset; }

namespace import {

class TagWriterBase
{
public:
    virtual ~TagWriterBase() {}
    virtual unsigned Write(const std::string&, const db::Recordset*) = 0;
};

} // namespace import

#endif
