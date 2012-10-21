#ifndef WRITE_TAGS_MP3_H
#define WRITE_TAGS_MP3_H

#include <string>
#include "tags.h"

namespace import {

/** Classes for importing MP3 files and their metadata.
 */
namespace mp3 {

class Tags: public import::Tags::Impl
{
public:
    Tags(const std::string& filename): import::Tags::Impl(filename) {}

    // Being a Tags::Impl
    unsigned Read(db::Recordset*);
    unsigned Write(const db::Recordset*);
};

} // namespace mp3
} // namespace import

#endif
