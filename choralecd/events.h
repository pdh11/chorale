#ifndef EVENTS_H
#define EVENTS_H

#include <qevent.h>
#include "liboutput/playstate.h"

enum {
    EVENT_PROGRESS = QEvent::User,
    EVENT_DISCOVERY,
    EVENT_TIMECODE,
    EVENT_PLAYSTATE
};

namespace util { class Task; }

/** For marshalling an import::RippingControlObserver */
class ProgressEvent: public QEvent
{
    unsigned m_track;
    unsigned m_type;
    unsigned m_num;
    unsigned m_denom;
    
public:
    ProgressEvent(unsigned track, unsigned event_type,
		  unsigned num, unsigned denom)
	: QEvent((QEvent::Type)EVENT_PROGRESS), 
	  m_track(track), m_type(event_type), m_num(num), m_denom(denom) {}
    unsigned GetTrack() const { return m_track; }
    unsigned GetType() const  { return m_type; }
    unsigned GetNum() const   { return m_num; }
    unsigned GetDenom() const { return m_denom; }
};

class TimeCodeEvent: public QEvent
{
    unsigned int m_index;
    unsigned int m_seconds;

public:
    TimeCodeEvent(unsigned int index, unsigned int seconds) 
	: QEvent((QEvent::Type)EVENT_TIMECODE), m_index(index), m_seconds(seconds) 
    {}
    
    unsigned GetIndex() const { return m_index; }
    unsigned GetSeconds() const { return m_seconds; }
};

class PlayStateEvent: public QEvent
{
    output::PlayState m_state;

public:
    PlayStateEvent(output::PlayState state)
	: QEvent((QEvent::Type)EVENT_PLAYSTATE), m_state(state)
	{}

    output::PlayState GetState() const { return m_state; }
};

#endif
