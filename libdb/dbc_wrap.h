/* dbc_wrap.h
 *
 * Wrap a DB for the benefit of the C API
 */
#ifndef DBC_WRAP_H
#define DBC_WRAP_H

#include "db.h"
#include "dbc.h"

db_Database db_Database_Wrap(db::DatabasePtr);

#endif
