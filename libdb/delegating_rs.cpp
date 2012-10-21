#include "delegating_rs.h"

namespace db {

bool DelegatingRecordset::IsEOF() { return m_rs->IsEOF(); }

uint32_t DelegatingRecordset::GetInteger(field_t which) { return m_rs->GetInteger(which); }

std::string DelegatingRecordset::GetString(field_t which) { return m_rs->GetString(which); }

unsigned int DelegatingRecordset::SetInteger(field_t which, uint32_t value)
{
    return m_rs->SetInteger(which, value);
}

unsigned int DelegatingRecordset::SetString(field_t which, const std::string& value)
{
    return m_rs->SetString(which, value);
}

void DelegatingRecordset::MoveNext()
{
    m_rs->MoveNext();
}

unsigned int DelegatingRecordset::AddRecord()
{
    return m_rs->AddRecord();
}

unsigned int DelegatingRecordset::Commit()
{
    return m_rs->Commit();
}

unsigned int DelegatingRecordset::Delete()
{
    return m_rs->Delete();
}

} // namespace db
