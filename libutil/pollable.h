#ifndef LIBUTIL_POLLABLE_H
#define LIBUTIL_POLLABLE_H 1

namespace util {

class Scheduler;
template <class T> class CountedPointer;
template <class T> class PtrCallback;
class Task;
typedef CountedPointer<Task> TaskPtr;
typedef PtrCallback<TaskPtr> TaskCallback;

/** Something which can be waited-for
 */
class Pollable
{
public:
    virtual ~Pollable() {}

    virtual void Wait(Scheduler*, const TaskCallback&) = 0;
};

/*
class PollableStreamSource: public Pollable
{
public:
    virtual unsigned int Read(void *buffer, size_t len, size_t *pread) 
	ATTRIBUTE_WARNUNUSED = 0;
};

class PollableStreamTarget: public Pollable
{
public:
    virtual unsigned int Write(const void *buffer, size_t len, size_t *pwrote)
	ATTRIBUTE_WARNUNUSED = 0;
};*/

/*---------*/

typedef int PollHandle;

enum { NOT_POLLABLE = -1 };

class Pollable2
{
public:
    virtual ~Pollable2() {}

    virtual int GetHandle() { return NOT_POLLABLE; }
};

} // namespace util

#endif
