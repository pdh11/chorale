#ifndef LIBUTIL_BUFFER_CHAIN_H
#define LIBUTIL_BUFFER_CHAIN_H

#include "counted_object.h"
#include "bind.h"
#include "not_thread_safe.h"
#include "trace.h"
#include <boost/intrusive/list.hpp>

namespace util {

class BufferAllocator;

typedef uint32_t bufsize_t;

struct Buffer: public CountedObject<NoLocking>
{
    void *data;
    bufsize_t actual_len;

    explicit Buffer(bufsize_t n) : actual_len(n) { data = malloc(n); }
    ~Buffer() { free(data); }
};

struct BufferPtr: public boost::intrusive_ptr<Buffer>
{
    bufsize_t start;
    bufsize_t len;

    explicit BufferPtr(Buffer *b)
	: boost::intrusive_ptr<Buffer>(b),
	  start(0), 
	  len(b ? b->actual_len : 0)
    {}
    BufferPtr() {}
};

class BufferChain
{
public:
    BufferPtr AllocateBuffer(bufsize_t rq);
};

/** A Gate is like a single-thread condition variable.
 *
 * It can be waited-on, and later opened (waiter released).
 */
class Gate: public boost::intrusive::list_base_hook<>
{
    Callback m_waiter;

    friend class Gatekeeper;

public:
    Gate() {}
    ~Gate() {}
};

class Gatekeeper: public NotThreadSafe
{
    boost::intrusive::list<Gate> m_open_gates;

public:
    void WaitForGate(Gate* gate, const Callback& waiter);
    void Open(Gate* gate);

    /** Releases waiters from any open gates; returns true if anything was
     * done.
     */
    bool Activate();
};

class BufferSink
{
    Gate m_sink_gate;

public:
    virtual ~BufferSink() {}

    Gate& SinkGate() { return m_sink_gate; }
    virtual unsigned int OnBuffer(BufferPtr) = 0;
};

class SocketWriter: public BufferSink
{
public:
//    explicit SocketWriter(SocketPtr s);

    unsigned int OnBuffer(BufferPtr p);
};

} // namespace util

#endif
