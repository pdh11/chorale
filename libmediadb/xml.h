/* libmediadb/xml.h
 *
 * XML-based serialisation and reloading of databases
 */
#ifndef MEDIADB_XML_H
#define MEDIADB_XML_H

#include <stdio.h>

namespace db { class Database; }

namespace mediadb {

/** Write a db::Database (assumed to be a mediadb::Database) to a file as XML.
 */
bool WriteXML(db::Database*, unsigned int schema, ::FILE *f);

/** Read a db::Database (assumed to be a mediadb::Database) from an XML file.x
 */
bool ReadXML(db::Database*, const char *filename);

} // namespace mediadb

#endif
