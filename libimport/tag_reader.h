#ifndef IMPORT_TAG_READER_H
#define IMPORT_TAG_READER_H

#include <string>

namespace db { class Recordset; }

namespace import {

class TagReaderBase
{
public:
    virtual ~TagReaderBase() {}
    virtual unsigned int Read(const std::string& filename, db::Recordset*);
};

} // namespace import

#endif
