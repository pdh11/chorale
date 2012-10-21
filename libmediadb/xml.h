/* libmediadb/xml.h
 *
 * XML-based serialisation and reloading of databases
 */
#ifndef MEDIADB_XML_H
#define MEDIADB_XML_H

#include <stdio.h>

namespace util { class Stream; }
namespace db { class Database; }

namespace mediadb {

/** Write a db::Database (assumed to be a mediadb::Database) to a file as XML.
 */
unsigned int WriteXML(db::Database*, unsigned int schema, ::FILE *f);

/** Read a db::Database (assumed to be a mediadb::Database) from an XML file.
 */
unsigned int ReadXML(db::Database*, const char *filename);

/** Read a db::Database (assumed to be a mediadb::Database) from a stream.
 */
unsigned int ReadXML(db::Database*, util::Stream*);

} // namespace mediadb

#endif
