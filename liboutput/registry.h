#ifndef LIBOUTPUT_REGISTRY_H
#define LIBOUTPUT_REGISTRY_H

#include "libutil/registry.h"

namespace output {

class URLPlayer;

class Registry: public util::Registry<URLPlayer>
{
public:
    Registry();
    ~Registry();
};

} // namespace mediadb

#endif
