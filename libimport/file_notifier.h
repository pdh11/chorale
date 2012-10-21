#ifndef IMPORT_FILE_NOTIFIER
#define IMPORT_FILE_NOTIFIER 1

#include "libutil/pollable.h"
#include "libutil/task.h"

namespace util { class Scheduler; }

namespace import {

class FileNotifierTask: public util::Task, private util::Pollable
{
public:
    class Observer
    {
    public:
	virtual ~Observer() {}
	
	virtual void OnChange() = 0;
    };

private:
    util::Scheduler *m_scheduler;
    Observer *m_obs;
    util::PollHandle m_fd;

    explicit FileNotifierTask(util::Scheduler*);

    // Being a util::Pollable
    util::PollHandle GetHandle() { return m_fd; }

    unsigned Run();

public:
    ~FileNotifierTask();

    typedef util::CountedPointer<FileNotifierTask> FileNotifierPtr;
    static FileNotifierPtr Create(util::Scheduler*);

    unsigned int Init();

    void Watch(const char *directory);

    void SetObserver(Observer *obs) { m_obs = obs; }
};

typedef util::CountedPointer<FileNotifierTask> FileNotifierPtr;

} // namespace import

#endif
