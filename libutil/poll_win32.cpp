#include "config.h"
#include "poll.h"
#include "poller_core.h"
#include "trace.h"

#ifdef WIN32

#include <windows.h>

#undef IN
#undef OUT

namespace util {

namespace win32 {

PollerCore::PollerCore()
    : m_array(NULL),
      m_count(0)
{
}

unsigned int PollerCore::SetUpArray(const std::vector<PollRecord> *pollables)
{
    m_count = pollables->size();

    void **newarray = (void**)realloc(m_array,
				      pollables->size() * sizeof(void*));
    if (!newarray)
	return ENOMEM;
    m_array = newarray;
    
    for (size_t i = 0; i<m_count; ++i)
	m_array[i] = (*pollables)[i].h;

    return 0;
}

unsigned int PollerCore::Poll(unsigned int timeout_ms)
{
//    TRACE << "Polling for " << timeout_ms << "\n";

    DWORD rc = ::WaitForMultipleObjects(m_count, m_array, false, timeout_ms);
    if (rc == WAIT_FAILED)
    {
//	TRACE << "WFMO failed " << GetLastError() << "\n";
	return GetLastError();
    }

    if (rc == WAIT_TIMEOUT)
	m_which = -1;
    else
    {
//	TRACE << "Poll returned " << (rc - WAIT_OBJECT_0) << "\n";
	m_which = rc - WAIT_OBJECT_0;
	m_which_handle = m_array[m_which];
    }

    return 0;
}

unsigned int PollerCore::DoCallbacks(const std::vector<PollRecord> *pollables,
				     bool valid)
{
    if (m_which == -1)
	return 0;
    
    if (valid)
	return (*pollables)[m_which].c();

//    TRACE << "Warning, pollables changed, searching for handle\n";

    for (unsigned int i=0; i<pollables->size(); ++i)
    {
	if ((*pollables)[i].h == m_which_handle)
	{
	    return (*pollables)[i].c();
	}
    }
    return 0;
}

PollerCore::~PollerCore()
{
    free(m_array);
}

PollWaker::PollWaker(PollerInterface *p)
    : m_event(::CreateEvent(NULL, false, false, NULL))
{
    p->Add(this, Bind<PollWaker, &PollWaker::OnActivity>(this),
	   util::PollerInterface::IN);
}

PollWaker::~PollWaker()
{
    ::CloseHandle(m_event);
}

unsigned PollWaker::OnActivity()
{
//    TRACE << "PollWaker::OnActivity\n";
    ::ResetEvent(m_event);
    return 0;
}

void PollWaker::Wake()
{
    ::SetEvent(m_event);
}

} // namespace util::win32

} // namespace util

#endif // WIN32
