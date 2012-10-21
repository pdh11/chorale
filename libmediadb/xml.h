/* libmediadb/xml.h
 *
 * XML-based serialisation and reloading of databases
 */
#ifndef MEDIADB_XML_H
#define MEDIADB_XML_H

#include <stdio.h>

namespace db { class Database; };

namespace mediadb {

class Database;

bool WriteXML(db::Database*, unsigned int schema, ::FILE *f);
bool ReadXML(db::Database*, const char *filename);

};

#endif
