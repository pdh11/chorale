#ifndef WRITE_TAGS_MP3_H
#define WRITE_TAGS_MP3_H

#include <string>
#include "tags.h"

namespace import {

/** Classes for importing MP3 files and their metadata.
 */
namespace mp3 {

class Tags: public import::Tags
{
public:
    Tags(const std::string& filename): import::Tags(filename) {}

    // Being a Tags
    unsigned Read(db::RecordsetPtr);
    unsigned Write(db::RecordsetPtr);
};

} // namespace mp3
} // namespace import

#endif
