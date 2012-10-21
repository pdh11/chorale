#include "libdbsteam/db.h"
#include "libmediadb/xml.h"
#include "libmediadb/schema.h"
#include "libmediadb/localdb.h"

int main(int, char *argv[])
{
    db::steam::Database sdb(mediadb::FIELD_COUNT);
    sdb.SetFieldInfo(mediadb::ID, 
		     db::steam::FIELD_INT|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::PATH,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);

    mediadb::ReadXML(&sdb, argv[1]);

    return 0;
}
