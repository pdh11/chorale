#include "db.h"
#include <assert.h>
#include "libutil/trace.h"

namespace db {
namespace steam {

void Test()
{
    db::steam::Database sdb(2);
    
    sdb.SetFieldInfo(0, db::steam::FIELD_INT|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(1, db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);

    db::RecordsetPtr rs = sdb.CreateRecordset();
    rs->AddRecord();
    rs->SetInteger(0, 42);
    rs->SetString(1, "foo");
    rs->Commit();

    rs->AddRecord();
    rs->SetInteger(0, 111);
    rs->SetString(1, "zachary");
    rs->Commit();

    rs->AddRecord();
    rs->SetInteger(0, 222222);
    rs->SetString(1, "beyonc\xC3\xA9");
    rs->Commit();

    /* Check string ordering */

    db::QueryPtr qp = sdb.CreateQuery();
    qp->OrderBy(1);
    rs = qp->Execute();

    assert(!rs->IsEOF());
    assert(rs->GetInteger(0) == 222222);
    assert(rs->GetString(1) == "beyonc\xC3\xA9");
    rs->MoveNext();
    assert(!rs->IsEOF());
    assert(rs->GetInteger(0) == 42);
    assert(rs->GetString(1) == "foo");
    rs->MoveNext();
    assert(!rs->IsEOF());
    assert(rs->GetInteger(0) == 111);
    assert(rs->GetString(1) == "zachary");
    rs->MoveNext();
    assert(rs->IsEOF());

    rs->GetString(1); // test eof handling
    rs->GetInteger(0);

    /* Check int ordering */

    qp = sdb.CreateQuery();
    qp->OrderBy(0);
    rs = qp->Execute();

    assert(!rs->IsEOF());
    assert(rs->GetInteger(0) == 42);
    assert(rs->GetString(1) == "foo");
    rs->MoveNext();
    assert(!rs->IsEOF());
    assert(rs->GetInteger(0) == 111);
    assert(rs->GetString(1) == "zachary");
    rs->MoveNext();
    assert(!rs->IsEOF());
    assert(rs->GetInteger(0) == 222222);
    assert(rs->GetString(1) == "beyonc\xC3\xA9");
    rs->MoveNext();
    assert(rs->IsEOF());

    /* Query by string */

    qp = sdb.CreateQuery();
    qp->Where(qp->Restrict(1, db::EQ, "zachary"));
    rs = qp->Execute();
    assert(!rs->IsEOF());
    assert(rs->GetInteger(0) == 111);
    assert(rs->GetString(1) == "zachary");
    rs->MoveNext();
    assert(rs->IsEOF());

    /* Query by int */

    qp = sdb.CreateQuery();
    qp->Where(qp->Restrict(0, db::EQ, 42));
    rs = qp->Execute();
    assert(!rs->IsEOF());
    assert(rs->GetInteger(0) == 42);
    assert(rs->GetString(1) == "foo");
    rs->MoveNext();
    assert(rs->IsEOF());

    /* Two-factor query */

    qp = sdb.CreateQuery();
    unsigned int rc = qp->Where(qp->And(qp->Restrict(0, db::EQ, 111),
					qp->Restrict(1, db::EQ, "zachary")));
    assert(rc == 0);
    rs = qp->Execute();
    assert(!rs->IsEOF());
    assert(rs->GetInteger(0) == 111);
    assert(rs->GetString(1) == "zachary");
    rs->MoveNext();
    assert(rs->IsEOF());

    /* Two-factor query, not found */

    qp = sdb.CreateQuery();
    rc = qp->Where(qp->And(qp->Restrict(0, db::EQ, 111222),
			   qp->Restrict(1, db::EQ, "zachary")));
    assert(rc == 0);
    rs = qp->Execute();
    assert(rs->IsEOF());

    /* Two-factor query, oritive */

    qp = sdb.CreateQuery();
    rc = qp->Where(qp->Or(qp->Restrict(0, db::EQ, 111222),
			  qp->Restrict(1, db::EQ, "zachary")));
    assert(rc == 0);
    rs = qp->Execute();
    assert(!rs->IsEOF());
    assert(rs->GetInteger(0) == 111);
    assert(rs->GetString(1) == "zachary");
    rs->MoveNext();
    assert(rs->IsEOF());

    /* Delete a record */

    qp = sdb.CreateQuery();
    qp->Where(qp->Restrict(0, db::EQ, 42));
    rs = qp->Execute();
    assert(!rs->IsEOF());
    assert(rs->GetInteger(0) == 42);
    assert(rs->GetString(1) == "foo");
    rs->Delete();
    assert(rs->IsEOF());

    /* Check ordering again */

    qp = sdb.CreateQuery();
    qp->OrderBy(1);
    rs = qp->Execute();

    assert(!rs->IsEOF());
    assert(rs->GetInteger(0) == 222222);
    assert(rs->GetString(1) == "beyonc\xC3\xA9");
    rs->MoveNext();
    assert(!rs->IsEOF());
    assert(rs->GetInteger(0) == 111);
    assert(rs->GetString(1) == "zachary");
    // test coercion
    assert(rs->GetString(0) == "111");
    assert(rs->GetInteger(1) == 0);
    rs->SetString(0, "37");
    rs->SetInteger(1, 999);
    assert(rs->GetInteger(0) == 37);
    assert(rs->GetString(1) == "999");
    rs->MoveNext();
    assert(rs->IsEOF());

    /* Check unordered reading */

    rs = sdb.CreateRecordset();
    assert(!rs->IsEOF());
    unsigned int id1 = rs->GetInteger(0);
    assert(id1 == 37 || id1 == 222222);
    rs->MoveNext();
    unsigned int id2 = rs->GetInteger(0);
    assert(id2 == 37 || id2 == 222222);
    assert(id1 != id2);
    rs->MoveNext();
    assert(rs->IsEOF());

    /* Introduce a duplicate string */

    rs = sdb.CreateRecordset();
    rs->AddRecord();
    rs->SetInteger(0, 112);
    rs->SetString(1, "beyonc\xC3\xA9");
    rs->Commit();

    /* Collate-by with duplicate string */

    qp = sdb.CreateQuery();
    qp->CollateBy(1);
    rs = qp->Execute();
    assert(!rs->IsEOF());
    assert(rs->GetString(0) == "999");
    rs->MoveNext();
    assert(!rs->IsEOF());
    assert(rs->GetString(0) == "beyonc\xC3\xA9");
    rs->MoveNext();
    assert(rs->IsEOF());

    /* Check ordering again */

    qp = sdb.CreateQuery();
    qp->OrderBy(1);
    rs = qp->Execute();

    assert(!rs->IsEOF());
    assert(rs->GetInteger(0) == 37);
    assert(rs->GetString(1) == "999");
    rs->MoveNext();
    assert(rs->GetString(1) == "beyonc\xC3\xA9");
    rs->MoveNext();
    assert(!rs->IsEOF());
    assert(rs->GetString(1) == "beyonc\xC3\xA9");
    rs->MoveNext();
    assert(rs->IsEOF());

    /* Collate-by with LIKE query */

    qp = sdb.CreateQuery();
    qp->Where(qp->Restrict(1, db::LIKE, "b.*"));
    qp->CollateBy(1);
    rs = qp->Execute();
    assert(!rs->IsEOF());
    assert(rs->GetString(1) == "beyonc\xC3\xA9");
    rs->MoveNext();
    assert(rs->IsEOF());

    /* Introduce a duplicate int */

    rs = sdb.CreateRecordset();
    rs->AddRecord();
    rs->SetInteger(0, 112);
    rs->SetString(1, "ptang");
    rs->Commit();

    /* Look for it */

    qp = sdb.CreateQuery();
    qp->Where(qp->Restrict(0, db::EQ, 112));
    rs = qp->Execute();
    assert(!rs->IsEOF());
    assert(rs->GetInteger(0) == 112);
    std::string s1 = rs->GetString(1);
    assert(s1 == "ptang" || s1 == "beyonc\xC3\xA9");
    rs->MoveNext();
    assert(rs->GetInteger(0) == 112);
    std::string s2 = rs->GetString(1);
    assert(s2 == "ptang" || s2 == "beyonc\xC3\xA9");
    assert(s1 != s2);
    rs->MoveNext();
    assert(rs->IsEOF());
    rs->MoveNext();

    /* Collate-by with duplicate int */

    qp = sdb.CreateQuery();
    qp->CollateBy(0);
    rs = qp->Execute();
    assert(!rs->IsEOF());
    assert(rs->GetInteger(0) == 37);
    rs->MoveNext();
    assert(!rs->IsEOF());
    assert(rs->GetInteger(0) == 112);
    rs->MoveNext();
    assert(!rs->IsEOF());
    assert(rs->GetInteger(0) == 222222);
    rs->MoveNext();
    assert(rs->IsEOF());
    rs->MoveNext();
    rs->Delete();
}

} // namespace steam
} // namespace db
