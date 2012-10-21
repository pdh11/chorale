#include "readonly_rs.h"
#include <errno.h>

namespace db {

unsigned int ReadOnlyRecordset::SetInteger(int, uint32_t)
{ return EPERM; }

unsigned int ReadOnlyRecordset::SetString(int, const std::string&)
{ return EPERM; }

unsigned int ReadOnlyRecordset::AddRecord()
{ return EPERM; }

unsigned int ReadOnlyRecordset::Commit()
{ return EPERM; }

unsigned int ReadOnlyRecordset::Delete()
{ return EPERM; }

}; // namespace db
