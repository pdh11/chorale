#ifndef LIBUTIL_HAL_H
#define LIBUTIL_HAL_H

#include <string>
#include "counted_object.h"

namespace util {

namespace dbus { class Connection; }

/** Classes for interacting with HAL for hardware enumeration and
 * notifications.
 */
namespace hal {

class Observer;
class DeviceObserver;

class Context
{
    class Impl;
    Impl *m_impl;

    friend class Device;

public:
    explicit Context(dbus::Connection*);
    ~Context();

    unsigned int Init();

    void AddObserver(Observer*);
    void RemoveObserver(Observer*);

    void GetAllDevices(DeviceObserver*);
    void GetMatchingDevices(const char *key, const char *value,
			    DeviceObserver*);
};

class Device: public util::CountedObject<>
{
    std::string m_udi;
    Context::Impl *m_ctx;

public:
    Device(Context::Impl*, const char *udi);
    ~Device();

    std::string GetString(const char *key);
    unsigned int GetInt(const char *key);

    const std::string& GetUDI() { return m_udi; }
};

typedef boost::intrusive_ptr<Device> DevicePtr;

class Observer
{
public:
    virtual ~Observer() {}
    virtual void OnDeviceAdded(DevicePtr) {}
    virtual void OnDeviceRemoved(DevicePtr) {}
};

class DeviceObserver
{
public:
    virtual ~DeviceObserver() {}

    virtual void OnDevice(DevicePtr) = 0;
};

} // namespace hal

} // namespace util

#endif
