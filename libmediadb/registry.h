#ifndef MEDIADB_REGISTRY_H
#define MEDIADB_REGISTRY_H

#include "libutil/registry.h"

namespace mediadb {

class Database;

class Registry: public util::Registry<Database>
{
public:
    Registry();
};

} // namespace mediadb

#endif
