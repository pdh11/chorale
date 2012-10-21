#include "data.h"
#include <string.h>
#include <stdlib.h>

namespace upnp {

static int CaselessCompare(const void *n1, const void *n2)
{
    return strcasecmp(*(const char**)n1, *(const char**)n2);
}

unsigned int EnumeratedString::Find(const char *name) const
{
    const char *const *ptr = 
	(const char*const*)::bsearch(&name, alternatives, n,
				     sizeof(const char*), 
				     &CaselessCompare);
    if (!ptr)
	return n;
    return (unsigned int)(ptr - alternatives);
}

} // namespace upnp
