#ifndef WRITE_TAGS_FLAC_H
#define WRITE_TAGS_FLAC_H

#include <string>
#include "tags.h"

namespace import {

/** Classes for importing FLAC files and their metadata.
 */
namespace flac {

class Tags: public import::Tags
{
public:
    Tags(const std::string& filename): import::Tags(filename) {}

    // Being a Tags
    unsigned Write(db::RecordsetPtr);

    // No Read() -- use the default one for now
};

} // namespace flac
} // namespace import

#endif
