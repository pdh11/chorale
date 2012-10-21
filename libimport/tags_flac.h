#ifndef WRITE_TAGS_FLAC_H
#define WRITE_TAGS_FLAC_H

#include <string>
#include "tag_writer.h"

namespace import {

/** Classes for importing FLAC files and their metadata.
 */
namespace flac {

class TagWriter: public import::TagWriterBase
{
public:
    unsigned Write(const std::string& filename, const db::Recordset*);
};

} // namespace flac
} // namespace import

#endif
