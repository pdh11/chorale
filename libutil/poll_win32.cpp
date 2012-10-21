#include "config.h"
#include "poll.h"
#include "trace.h"

#ifdef WIN32

#include <windows.h>

namespace util {

namespace win32 {

PollerCore::PollerCore()
    : m_array(NULL),
      m_count(0)
{
}

unsigned int PollerCore::SetUpArray(const std::map<int,Pollable*> *pollables,
				    const std::map<int,unsigned int>*)
{
    void **newarray = (void**)realloc(m_array,
				      pollables->size() * sizeof(void*));
    if (!newarray)
	return ENOMEM;
    m_array = newarray;
    
    size_t i = 0;
    for (std::map<int,Pollable*>::const_iterator j = pollables->begin();
	 j != pollables->end();
	 ++j)
    {
	m_array[i] = (void*)(j->first);
	++i;
    }

    m_count = i;

    return 0;
}

unsigned int PollerCore::Poll(unsigned int timeout_ms)
{
//    TRACE << "Polling for " << timeout_ms << "\n";

    DWORD rc = ::WaitForMultipleObjects(m_count, m_array, false, timeout_ms);
    if (rc == WAIT_FAILED)
    {
	TRACE << "WFMO failed " << GetLastError() << "\n";
	return GetLastError();
    }

    if (rc == WAIT_TIMEOUT)
	m_which = -1;
    else
    {
//	TRACE << "Poll returned " << (rc - WAIT_OBJECT_0) << "\n";
	m_which = rc - WAIT_OBJECT_0;
    }

    return 0;
}

unsigned int PollerCore::DoCallbacks(const std::map<int,Pollable*> *pollables)
{
    if (m_which == -1)
	return 0;

    std::map<int,Pollable*>::const_iterator j
	= pollables->find((int)m_array[m_which]);
    if (j != pollables->end())
	return j->second->OnActivity();
    TRACE << "No callback found\n";
    return 0;
}

PollerCore::~PollerCore()
{
    free(m_array);
}

PollWaker::PollWaker(PollerInterface *p, Pollable *pollable)
    : m_pollable(pollable),
      m_event(::CreateEvent(NULL, false, false, NULL))
{
    p->AddHandle((int)m_event, this);
}

PollWaker::~PollWaker()
{
    ::CloseHandle(m_event);
}

unsigned PollWaker::OnActivity()
{
    TRACE << "PollWaker::OnActivity\n";
    ::ResetEvent(m_event);
    return m_pollable ? m_pollable->OnActivity() : 0;
}

void PollWaker::Wake()
{
    ::SetEvent(m_event);
}

} // namespace util::win32

} // namespace util

#endif // WIN32
