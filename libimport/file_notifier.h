#ifndef IMPORT_FILE_NOTIFIER
#define IMPORT_FILE_NOTIFIER 1

#include "libutil/pollable.h"

namespace util { class PollerInterface; }

namespace import {

class FileNotifier: public util::Pollable
{
public:
    class Observer;

private:
    Observer *m_obs;
    util::PollHandle m_fd;

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

    unsigned OnActivity();

    // Being a util::Pollable
    util::PollHandle GetReadHandle() { return m_fd; }
};

} // namespace import

#endif
