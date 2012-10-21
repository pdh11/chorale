#ifndef CHORALED_WEB_H
#define CHORALED_WEB_H

#include "libutil/web_server.h"

class RootContentFactory: public util::ContentFactory
{
public:
    util::SeekableStreamPtr StreamForPath(const char *path);
};

#endif
