#ifndef PARTIAL_STREAM_H
#define PARTIAL_STREAM_H 1

#include "stream.h"

namespace util {

SeekableStreamPtr CreatePartialStream(SeekableStreamPtr underlying,
				      unsigned long long begin,
				      unsigned long long end);

StreamPtr CreatePartialStream(StreamPtr underlying,
			      unsigned long long length);

} // namespace util

#endif
