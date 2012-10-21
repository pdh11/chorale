#ifndef LIBOUTPUT_REGISTRY_H
#define LIBOUTPUT_REGISTRY_H

#include "libutil/registry.h"

namespace output {

class Queue;

class Registry: public util::Registry<Queue>
{
public:
    Registry();
    ~Registry();
};

} // namespace mediadb

#endif
