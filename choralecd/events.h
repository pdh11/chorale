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

namespace util { class Task; };

class ProgressEvent: public QEvent
{
    const util::Task *m_task;
    unsigned m_num;
    unsigned m_denom;
    
public:
    ProgressEvent(const util::Task *task, unsigned num, unsigned denom)
	: QEvent((QEvent::Type)EVENT_PROGRESS), 
	  m_task(task), m_num(num), m_denom(denom) {}
    const util::Task *GetTask() const { return m_task; }
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
