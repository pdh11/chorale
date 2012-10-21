#ifndef LIBUTIL_HAL_H
#define LIBUTIL_HAL_H

#include <string>

namespace util {

namespace dbus { class Connection; };

/** Classes for interacting with HAL for hardware enumeration and
 * notifications.
 */
namespace hal {

class Observer
{
public:
    virtual ~Observer() {}
    virtual void OnDeviceAdded(const char*) {}
    virtual void OnDeviceRemoved(const char*) {}
};

class DeviceObserver
{
public:
    virtual ~DeviceObserver() {}

    virtual void OnDevice(const std::string& udi) = 0;
};

class Context
{
    class Impl;
    Impl *m_impl;

public:
    explicit Context(dbus::Connection*);
    ~Context();

    unsigned int Init();

    void AddObserver(Observer*);
    void RemoveObserver(Observer*);

    void GetAllDevices(DeviceObserver*);
    void GetMatchingDevices(const char *key, const char *value,
			    DeviceObserver*);

    std::string DeviceGetPropertyString(const char *udi,
					const char *key);
    unsigned int DeviceGetPropertyInt(const char *udi,
				      const char *property);
};

}; // namespace hal

}; // namespace util

#endif
