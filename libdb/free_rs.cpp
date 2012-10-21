#include "free_rs.h"
#include <sstream>
#include <errno.h>

namespace db {

db::RecordsetPtr FreeRecordset::Create()
{
    return db::RecordsetPtr(new FreeRecordset());
}

bool FreeRecordset::IsEOF()
{
    return m_eof;
}

uint32_t FreeRecordset::GetInteger(int which)
{
    std::string s = GetString(which);
    return strtoul(s.c_str(), NULL, 10);
}

std::string FreeRecordset::GetString(int which)
{
    if ((unsigned)which >= m_strings.size())
	return "";
    return m_strings[which];
}

unsigned int FreeRecordset::SetInteger(int which, uint32_t value)
{
    std::ostringstream os;
    os << value;
    return SetString(which, os.str());
}

unsigned int FreeRecordset::SetString(int which, const std::string& value)
{
    if (which < 0)
	return 0;
    if (which >= (int)m_strings.size())
	m_strings.resize(which+1);
    m_strings[which] = value;
    return 0;
}

void FreeRecordset::MoveNext()
{
    m_eof = true;
}

unsigned int FreeRecordset::AddRecord()
{
    return EPERM;
}

unsigned int FreeRecordset::Commit()
{
    return 0;
}

unsigned int FreeRecordset::Delete()
{
    m_eof = true;
    return 0;
}

} // namespace db
