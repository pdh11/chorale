#include "trace.h"
#include <boost/thread/mutex.hpp>

boost::mutex Tracer::sm_mutex;

void Hex::Dump(const void *address, size_t nbytes)
{
    unsigned int offset = 0;

    while (nbytes)
    {
	unsigned int line = 16;
	if (line > nbytes)
	    line = (unsigned int)nbytes;
	printf("%08x ", offset);
	unsigned int i;
	for (i=0; i<line; ++i)
	    printf(" %02x", ((const unsigned char*)address)[offset+i]);
	for ( ; i<16; ++i)
	    printf("   ");
	printf(" |");
	for (i=0; i<line; ++i)
	{
	    unsigned char c = ((const unsigned char*)address)[offset+i];
	    if (c<32 || c >= 127)
		printf(".");
	    else
		printf("%c", c);
	}
	for ( ; i<16; ++i)
	    printf(" ");
	printf("|\n");

	nbytes -= line;
	offset += line;
    }
}
