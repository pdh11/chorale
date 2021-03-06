/* db/dbc.h
 *
 * C API for databases
 */
#ifndef DB_DBC_H
#define DB_DBC_H 1

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct db__Recordset *db_Recordset; // Opaque type

int         db_Recordset_IsEOF(db_Recordset);
uint32_t    db_Recordset_GetInteger(db_Recordset, unsigned int which);
const char *db_Recordset_GetString(db_Recordset, unsigned int which);
void        db_Recordset_MoveNext(db_Recordset);
void        db_Recordset_Free(db_Recordset*);


enum db_RestrictionType
{
    db_EQ,
    db_NE,
    db_LIKE
};

typedef struct db__Query *db_Query; // Opaque type

typedef const struct db__QueryRep *db_QueryExpr; // Opaque type

db_QueryExpr db_Query_Restrict(db_Query, unsigned int which, 
			      db_RestrictionType rt, const char *val);
db_QueryExpr db_Query_Restrict2(db_Query, unsigned int which,
			       db_RestrictionType rt, uint32_t val);
db_QueryExpr db_Query_And(db_Query, const db_QueryExpr, const db_QueryExpr);
db_QueryExpr db_Query_Or(db_Query, const db_QueryExpr, const db_QueryExpr);

unsigned int db_Query_Where(const db_QueryExpr);
db_Recordset db_Query_Execute(db_Query);
void         db_Query_Free(db_Query*);


typedef struct db__Database *db_Database; // Opaque type

db_Recordset db_Database_CreateRecordset(db_Database);
db_Query     db_Database_CreateQuery(db_Database);
void         db_Database_Free(db_Database*);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
