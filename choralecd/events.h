#ifndef EVENTS_H
#define EVENTS_H

#include <qevent.h>
#include <string>

namespace choraleqt {

template <class T>
class CustomEvent: public QEvent
{
    static int sm_event_type;

public:
    static int EventType()
    {
	if (!sm_event_type)
	    sm_event_type = QEvent::registerEventType();
	return sm_event_type;
    }

    CustomEvent()
	: QEvent((QEvent::Type)EventType())
    {}
    
    CustomEvent(const T& tt)
	: QEvent((QEvent::Type)EventType()),
	  m_data(tt)
    {}

    T m_data;

    const T& GetData() const { return m_data; }
};

template <class T>
int CustomEvent<T>::sm_event_type = 0;

struct ProgressPayload
{
    unsigned track;
    unsigned type;
    unsigned num;
    unsigned denom;
};

/** For marshalling an import::RippingControlObserver */
class ProgressEvent: public CustomEvent<ProgressPayload>
{
public:
    ProgressEvent(unsigned track, unsigned event_type,
		  unsigned num, unsigned denom)
    {
	m_data.track = track;
	m_data.type = event_type;
	m_data.num = num;
	m_data.denom = denom;
    }
    unsigned GetTrack() const { return m_data.track; }
    unsigned GetType() const  { return m_data.type; }
    unsigned GetNum() const   { return m_data.num; }
    unsigned GetDenom() const { return m_data.denom; }
};

typedef CustomEvent<std::string> StringEvent;

} // namespace choraleqt

#endif
