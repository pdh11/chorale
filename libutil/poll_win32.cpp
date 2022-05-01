#include "config.h"
#include "poll.h"
#include "trace.h"

#ifdef WIN32

#include <windows.h>

LOG_DECL(POLL);

#undef IN
#undef OUT

namespace util {

namespace win32 {

PollerCore::PollerCore()
    : m_array(NULL),
      m_count(0),
      m_which(0),
      m_which_handle(NULL),
      m_wakeup_event(::CreateEvent(NULL, false, false, NULL))
{
}

unsigned int PollerCore::SetUpArray(std::vector<PollRecord> *pollables)
{
    m_count = pollables->size() + 1;

    void **newarray = (void**)realloc(m_array, m_count * sizeof(void*));
    if (!newarray)
	return ENOMEM;
    m_array = newarray;

    m_array[0] = m_wakeup_event;
    
    for (size_t i = 1; i<m_count; ++i)
    {
	PollRecord& record = (*pollables)[i-1];
	if (!record.internal)
	{
	    record.internal = WSACreateEvent();
	    if (record.direction == util::PollRecord::IN)
		WSAEventSelect(record.h, record.internal,
			       FD_READ | FD_ACCEPT | FD_CLOSE);
	    else
		WSAEventSelect(record.h, record.internal, FD_WRITE | FD_CLOSE);
	}
	m_array[i] = record.internal;
    }

    return 0;
}

void PollerCore::Deleting(PollRecord *r)
{
    if (r->internal)
    {
	WSAEventSelect(r->h, NULL, 0);
	WSACloseEvent((WSAEVENT)r->internal);
    }
}

unsigned int PollerCore::Poll(unsigned int timeout_ms)
{
    LOG(POLL) << "Polling for " << timeout_ms << "ms, " << m_count << " handles\n";

    DWORD count = m_count;
    if (count > MAXIMUM_WAIT_OBJECTS)
	count = MAXIMUM_WAIT_OBJECTS;

    DWORD rc = ::WaitForMultipleObjects(count, m_array, false, timeout_ms);
    if (rc == WAIT_FAILED)
    {
	TRACE << "WFMO failed " << GetLastError() << "\n";
	return GetLastError();
    }

    if (rc == WAIT_TIMEOUT)
    {
//	TRACE << "Poll timed out\n";
	m_which = -1;
    }
    else if (rc == WAIT_OBJECT_0)
    {
	m_which = -1; // Wake-up call only
	::ResetEvent(m_wakeup_event);
    }
    else
    {
//	TRACE << "Poll returned " << (rc - WAIT_OBJECT_0 - 1) << "\n";
	m_which = rc - WAIT_OBJECT_0 - 1;
	m_which_handle = m_array[m_which];
    }

    return 0;
}

unsigned int PollerCore::DoCallbacks(std::vector<PollRecord> *pollables,
				     bool valid)
{
    if (!valid)
    {
//      TRACE << "Warning, pollables changed, searching for handle\n";
	m_which = -1;

	for (unsigned int i=0; i<pollables->size(); ++i)
	{
	    if ((*pollables)[i].internal == m_which_handle)
	    {
		m_which = i;
		break;
	    }
	}
    }

    if (m_which == -1)
	return 0;
    
//  TRACE << "Making callback " << m_which << "\n";
    PollRecord& record = (*pollables)[m_which];
    ::ResetEvent(record.internal);

    if (record.oneshot)
	record.direction = 0; // Mark for deletion

    if (record.tc.IsValid())
	record.tc();
    return 0;
}

void PollerCore::Wake()
{
    ::SetEvent(m_wakeup_event);
}

PollerCore::~PollerCore()
{
    free(m_array);
    ::CloseHandle(m_wakeup_event);
}

} // namespace util::win32

} // namespace util

#endif // WIN32
