#ifndef LIBEMPEG_DISCOVERY_H
#define LIBEMPEG_DISCOVERY_H

#include <string>
#include "libutil/counted_pointer.h"


namespace util { struct IPAddress; }
namespace util { class Scheduler; }

/** Classes for communicating with Empeg car-players over Ethernet.
 */
namespace empeg {

class Discovery
{
    class Task;
    typedef util::CountedPointer<Task> TaskPtr;
    TaskPtr m_task;

public:
    Discovery();
    ~Discovery();

    class Callback
    {
    public:
	virtual ~Callback() {}
	virtual void OnDiscoveredEmpeg(const util::IPAddress&,
				       const std::string& name) = 0;
    };

    unsigned Init(util::Scheduler*, Callback*);
};

} // namespace empeg

#endif
