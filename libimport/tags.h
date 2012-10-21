/* libimport/tags.h */

#ifndef IMPORT_TAGS_H
#define IMPORT_TAGS_H

#include <string>

namespace db { class Recordset; }

/** Classes for reading media files and their metadata.
 *
 * Including files and CDs.
 */
namespace import {

class Tags
{
public:
    class Impl
    {
    protected:
	std::string m_filename;
    public:
	Impl(const std::string& filename) : m_filename(filename) {}
	virtual ~Impl() {}

	virtual unsigned int Read(db::Recordset*);
	virtual unsigned int Write(const db::Recordset*) = 0;
    };

private:
    Impl *m_impl;

public:
    Tags();
    ~Tags();

    unsigned Open(const std::string& filename);

    unsigned Read(db::Recordset*);
    unsigned Write(const db::Recordset*);
};

} // namespace import

#endif
