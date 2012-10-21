#include "queue.h"
#include "urlplayer.h"
#include "libutil/trace.h"
#include "libutil/observable.h"
#include "libmediadb/db.h"
#include "libmediadb/schema.h"
#include "libmediadb/registry.h"
#include "libmediadb/didl.h"
#include <vector>

namespace output {

class Queue::Impl: public URLObserver,
		   public util::Observable<QueueObserver>
{
    Queue *m_parent;
    URLPlayer *m_player;
    std::string m_expected_url;
    std::string m_expected_next_url;
    PlayState m_state;

    friend class Queue;

public:
    Impl(Queue *parent, URLPlayer *player) 
	: m_parent(parent), m_player(player), m_state(STOP)
    {
	player->AddObserver(this);
    }

    // Being a URLObserver
    void OnPlayState(output::PlayState state)
    {
	TRACE << "OnPlayState(" << state << ")\n"; 
	m_state = state;
	Fire(&QueueObserver::OnPlayState, state);
    }

    void OnURL(const std::string& url)
    {
	TRACE << "OnURL(" << url << ")\n"; 

	bool current=false, next=false;
	unsigned int index = 0;

	{
	    boost::mutex::scoped_lock lock(m_parent->m_mutex);
	    if (url == m_expected_url)
	    {
		current = true;
		index = m_parent->m_current_index;
	    }
	    else if (url == m_expected_next_url)
	    {
		next = true;
		index = ++m_parent->m_current_index;
		m_expected_url = m_expected_next_url;
		m_expected_next_url.clear();
	    }
	}
	
	if (next)
	{
	    Fire(&QueueObserver::OnTimeCode, index, 0u);
	    m_parent->SetNextURL();
	}
	else if (current)
	{
	    Fire(&QueueObserver::OnTimeCode, index, 0u);
	}
	else
	{
	    TRACE << "** Unexpected URL (not " << m_expected_url << " or "
		  << m_expected_next_url << "), stopping\n";
	    OnPlayState(output::STOP); // Someone else has taken control
	}
    }

    void OnTimeCode(unsigned int s)
    {
//	TRACE << "OnTimeCode(" << s << ")\n";
	Fire(&QueueObserver::OnTimeCode, m_parent->m_current_index, s);
    }
};


        /* Queue itself */


Queue::Queue(URLPlayer *player)
    : m_current_index(0), m_impl(new Impl(this, player))
{
}

Queue::~Queue()
{
    delete m_impl;
}

void Queue::Add(mediadb::Database *db, unsigned int id)
{
    Pair dbp;
    dbp.db = db;
    dbp.id = id;
    {
	boost::mutex::scoped_lock lock(m_mutex);
	m_queue.push_back(dbp);
    }

    if (m_current_index == m_queue.size()-2)
	SetNextURL();
}

void Queue::SetPlayState(output::PlayState state)
{
    switch (state)
    {
    case PLAY:
    {
	if (m_queue.empty())
	    return;

	SetURL();
	SetNextURL();
	TRACE << "calling Play\n";
	m_impl->m_player->SetPlayState(output::PLAY);
	TRACE << "play done\n";
	break;
    }
    case PAUSE:
    case STOP:
	TRACE << "calling SetPlayState\n";
	m_impl->m_player->SetPlayState(state);
	TRACE << "setplaystate done\n";
	break;
	
    default:
	break;
    }
}

void Queue::Seek(unsigned int index, unsigned int /*ms*/)
{
    TRACE << "Seeking to index " << index << "\n";
    {
	boost::mutex::scoped_lock lock(m_mutex);
	if (index < m_queue.size())
	    m_current_index = index;
    }
    /** A renderer is allowed to go to STOP on receipt of a new URL */
    PlayState state = m_impl->m_state;
    SetURL();
    SetNextURL();
    SetPlayState(state);
}

void Queue::AddObserver(QueueObserver *obs)
{
    m_impl->AddObserver(obs);
}

void Queue::SetURL()
{
    Pair dbp;
    {
	boost::mutex::scoped_lock lock(m_mutex);
	if (m_current_index >= m_queue.size())
	    return;
	dbp = m_queue[m_current_index];
    }
    std::string url = dbp.db->GetURL(dbp.id);
    db::QueryPtr qp = dbp.db->CreateQuery();
    qp->Where(qp->Restrict(mediadb::ID, db::EQ, dbp.id));
    db::RecordsetPtr rs = qp->Execute();
    std::string metadata;
    if (rs && !rs->IsEOF())
	metadata = mediadb::didl::s_header
	    + mediadb::didl::FromRecord(dbp.db, rs)
	    + mediadb::didl::s_footer;
    
    {
	boost::mutex::scoped_lock lock(m_mutex);
	m_impl->m_expected_url = url.c_str();
    }
    m_impl->m_player->SetURL(url, metadata);
}

void Queue::SetNextURL()
{
    Pair dbp;
    {
	boost::mutex::scoped_lock lock(m_mutex);
	if ((m_current_index+1) >= m_queue.size())
	    return;
	dbp = m_queue[m_current_index+1];
    }
    std::string url = dbp.db->GetURL(dbp.id);
    db::QueryPtr qp = dbp.db->CreateQuery();
    qp->Where(qp->Restrict(mediadb::ID, db::EQ, dbp.id));
    db::RecordsetPtr rs = qp->Execute();
    std::string metadata;
    if (rs && !rs->IsEOF())
	metadata = mediadb::didl::s_header
	    + mediadb::didl::FromRecord(dbp.db, rs)
	    + mediadb::didl::s_footer;

    {
	boost::mutex::scoped_lock lock(m_mutex);
	m_impl->m_expected_next_url = url.c_str();
    }
    m_impl->m_player->SetNextURL(url, metadata);
}

} // namespace output
