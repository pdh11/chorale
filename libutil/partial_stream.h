#ifndef PARTIAL_STREAM_H
#define PARTIAL_STREAM_H 1

#include "stream.h"
#include <memory>

namespace util {

/** A stream representing a portion of another stream.
 *
 * If the underlying stream isn't SEEKABLE, begin must be 0.
 */
std::unique_ptr<Stream> CreatePartialStream(Stream *underlying,
					  uint64_t begin,
					  uint64_t end);

} // namespace util

#endif
