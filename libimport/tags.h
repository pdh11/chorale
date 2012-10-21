/* libimport/tags.h */

#ifndef IMPORT_TAGS_H
#define IMPORT_TAGS_H

#include <string>
#include "libdb/db.h"
#include <boost/thread/mutex.hpp>
#include "libutil/counted_object.h"

namespace import {

class Tags: public CountedObject
{
protected:
    std::string m_filename;

    Tags(const std::string& filename) : m_filename(filename) {}
public:
    virtual ~Tags() {}
    virtual unsigned Read(db::RecordsetPtr);
    virtual unsigned Write(db::RecordsetPtr);

    typedef ::boost::intrusive_ptr<Tags> Pointer;

    static Pointer Create(const std::string& filename);
};

typedef Tags::Pointer TagsPtr;

unsigned ReadTags(const std::string& filename, db::RecordsetPtr tags);
unsigned WriteTags(const std::string& filename, db::RecordsetPtr tags);

/** This shouldn't be necessary, but TagLib's string operations aren't
 * thread-safe.
 */
extern boost::mutex s_taglib_mutex;

} // namespace import

#endif
