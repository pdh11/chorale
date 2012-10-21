/* libimport/tags.h */

#ifndef IMPORT_TAGS_H
#define IMPORT_TAGS_H

#include <string>
#include <errno.h>
#include "libutil/choose_by_extension.h"

namespace db { class Recordset; }

/** Classes for reading media files and their metadata.
 *
 * Including files and CDs.
 */
namespace import {

class TagReaderBase;
class TagWriterBase;

class TagReader
{
    util::ChooseByExtension<TagReaderBase> m_chooser;

public:
    TagReader();
    ~TagReader();
    unsigned int Init(const std::string& filename);
    unsigned int Read(db::Recordset*);
};

class TagWriter
{
    util::ChooseByExtension<TagWriterBase> m_chooser;

public:
    TagWriter();
    ~TagWriter();
    unsigned int Init(const std::string& filename);
    unsigned int Write(const db::Recordset*);
};

} // namespace import

#endif
