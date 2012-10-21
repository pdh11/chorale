#ifndef IMPORT_FILE_NOTIFIER
#define IMPORT_FILE_NOTIFIER 1

#include "libutil/poll.h"

namespace import {

class FileNotifier: public util::Pollable
{
public:
    class Observer;

private:
    Observer *m_obs;
    int m_fd;

public:
    FileNotifier();
    ~FileNotifier();

    unsigned int Init(util::PollerInterface*);

    void Watch(const char *directory);

    class Observer
    {
    public:
	virtual ~Observer() {}
	
	virtual void OnChange() = 0;
    };

    void SetObserver(Observer *obs) { m_obs = obs; }

    // being a Pollable
    unsigned OnActivity();
};

} // namespace import

#endif
