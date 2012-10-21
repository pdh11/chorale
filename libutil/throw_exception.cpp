#include <boost/throw_exception.hpp>
#include <stdlib.h>
#include <stdio.h>

#ifdef BOOST_NO_EXCEPTIONS

void boost::throw_exception(const std::exception& e)
{
    fprintf(stderr, "exception: %s\n", e.what());
    exit(1);
}

#endif
