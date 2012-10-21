#ifndef WRITE_TAGS_MP3_H
#define WRITE_TAGS_MP3_H

#include <string>
#include "tag_reader.h"
#include "tag_writer.h"

namespace import {

/** Classes for importing MP3 files and their metadata.
 */
namespace mp3 {

class TagWriter: public import::TagWriterBase
{
public:
    unsigned Write(const std::string& filename, const db::Recordset*);
};

class TagReader: public import::TagReaderBase
{
public:
    unsigned Read(const std::string& filename, db::Recordset*);
};

} // namespace mp3
} // namespace import

#endif
