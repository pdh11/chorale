#include "queue.h"
#include "urlplayer.h"
#include "libutil/trace.h"
#include "libutil/observable.h"
#include "libutil/counted_pointer.h"
#include "libdb/query.h"
#include "libdb/recordset.h"
#include "libmediadb/db.h"
#include "libmediadb/schema.h"
#include "libmediadb/didl.h"
#include <vector>
#include <algorithm>

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
//	TRACE << "OnPlayState(" << state << ")\n"; 
	m_state = state;
	Fire(&QueueObserver::OnPlayState, state);
    }

    void OnURL(const std::string& url)
    {
//	TRACE << "OnURL(" << url << ")\n"; 

	bool current=false, next=false;
	unsigned int index = 0;

	{
	    util::RecursiveMutex::Lock lock(m_parent->m_mutex);
	    if (url == m_expected_url && !url.empty())
	    {
		current = true;
		index = m_parent->m_current_index;
		m_parent->m_entries[m_parent->m_queue[index]].flags |= PLAYED;
//		TRACE << "Got expected URL, index=" << index << "\n";
	    }
	    else if (url == m_expected_next_url && !url.empty())
	    {
		next = true;
		index = ++m_parent->m_current_index;
		m_expected_url = m_expected_next_url;
		m_expected_next_url.clear();
		m_parent->m_entries[m_parent->m_queue[index]].flags |= PLAYED;
//		TRACE << "Got expected next URL, index=" << index << "\n";
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

	/** Care here as the target (if UPnPAV) might be already
	 * playing something that's not ours.
	 */
	if (m_parent->m_current_index < m_parent->Count())
	    Fire(&QueueObserver::OnTimeCode, m_parent->m_current_index, s);
    }
};


        /* Queue itself */


Queue::Queue(URLPlayer *player)
    : m_current_index(0), m_shuffled(false), m_impl(new Impl(this, player))
{
}

Queue::~Queue()
{
    delete m_impl;
}

void Queue::Add(mediadb::Database *db, unsigned int id)
{
    Entry dbp;
    dbp.db = db;
    dbp.id = id;
    dbp.flags = 0;
    {
	util::RecursiveMutex::Lock lock(m_mutex);
	m_entries.push_back(dbp);
	m_queue.push_back((unsigned int)m_entries.size()-1);
    }

    if (m_current_index == m_entries.size()-2)
	SetNextURL();
}

/** Insert at a given position in the queue.
 *
 * Used when displaying the list in shuffled order.
 */
void Queue::QueueInsert(mediadb::Database *db, unsigned int id, 
			unsigned int where)
{
    Entry dbp;
    dbp.db = db;
    dbp.id = id;
    dbp.flags = 0;

    bool set = false;
    bool set_next = false;

    {
	util::RecursiveMutex::Lock lock(m_mutex);
	m_entries.push_back(dbp);
	m_queue.insert(m_queue.begin() + where, 
		       (unsigned int)m_entries.size()-1);

	if (where > 0 && m_current_index == where-1)
	    set_next = true;
	else if (m_current_index >= where)
	    ++m_current_index;
	if (m_entries.size() == 1)
	    set = true;
    }

    if (set)
	SetURL();
    if (set_next)
	SetNextURL();
}

/** Insert at a given position in the Entries array.
 *
 * Used when displaying the list in unshuffled order.
 */
void Queue::EntryInsert(mediadb::Database *db, unsigned int id, 
			unsigned int where)
{
    Entry dbp;
    dbp.db = db;
    dbp.id = id;
    dbp.flags = 0;

    bool set = false;
    bool set_next = false;

    {
	util::RecursiveMutex::Lock lock(m_mutex);
	m_entries.insert(m_entries.begin() + where, dbp);

	for (unsigned int i=0; i<m_queue.size(); ++i)
	{
	    if (m_queue[i] >= where)
		m_queue[i] += 1;
	}

	// Where to add it to the queue? If in unshuffled mode, add it
	// in the position corresponding to the entry. If shuffled, add it
	// at a random point in "the future".
	//
	// @bug if populating an empty list, first/last never get shuffled
	size_t pos;
	if (m_shuffled)
	{
//	    TRACE << "mqs=" << m_queue.size() << " mci=" << m_current_index
//		  << "\n";

	    unsigned int first_available = m_current_index;
	    if (m_impl->m_state == PLAY)
		++first_available;

	    if (first_available == m_queue.size())
	    {
		pos = m_queue.size();
	    }
	    else
	    {
		pos = first_available
		    + (rand() % (m_queue.size() - first_available + 1));
//		TRACE << "pos=" << pos << "\n";
		assert(pos <= m_queue.size());
	    }
	}
	else
	{
	    pos = where;
	}
//	TRACE << "pos=" << pos << "\n";

	m_queue.insert(m_queue.begin() + pos, where);

	if (pos > 0 && m_current_index == pos-1)
	    set_next = true;
	else if (pos == m_current_index)
	    set = true;
	else if (m_current_index > pos)
	    ++m_current_index;
    }

    if (set)
	SetURL();
    if (set_next)
	SetNextURL();
}

struct NullDatabase
{
    bool operator()(const Queue::Entry& e) { return !e.db; }
};

void Queue::QueueRemove(unsigned int from, unsigned int to)
{
    bool set = false;
    bool set_next = false;

    {
	util::RecursiveMutex::Lock lock(m_mutex);

	// Mark the relevant entries for deletion
	for (unsigned int i=from; i<to; ++i)
	    m_entries[m_queue[i]].db = NULL;

	// Prepare a map of old-index -> new-index
	std::map<unsigned int, unsigned int> themap;
	for (unsigned int i=0, j=0; i<m_entries.size(); ++i)
	{
	    if (m_entries[i].db)
		themap[i] = j++;
	}

	// Do the deletion of entries
	m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(),
				       NullDatabase()),
			m_entries.end());

	// Do the deletion of queue elements
	m_queue.erase(m_queue.begin()+from,
		      m_queue.begin()+to);

	// Convert all the old-indexes to new-indexes using the map
	for (unsigned int i=0; i<m_queue.size(); ++i)
	    m_queue[i] = themap[m_queue[i]];

	if (m_current_index >= to)
	{
	    m_current_index -= (to-from);
	}
	else if (m_current_index >= from)
	{
	    // In removed section
//	    TRACE << "mci vanished\n";
	    set = true;
	    set_next = true;
	    if (from == m_entries.size())
		m_current_index = 0;
	    else
		m_current_index = from;
	}
	else if (from>0 && m_current_index == from-1)
	    set_next = true;
    }

    if (set)
	SetURL();
    if (set_next)
	SetNextURL();
}

void Queue::EntryRemove(unsigned int from, unsigned int to)
{
    bool set = false;
    bool set_next = false;

    {
	util::RecursiveMutex::Lock lock(m_mutex);

	unsigned int next_entry = 0;
	if (m_current_index + 1 < m_queue.size())
	    next_entry = m_queue[m_current_index+1];
	set_next = (next_entry >= from && next_entry < to);

//	TRACE << "Erasing from " << from << " to " << to << " size "
//	      << m_entries.size() << "\n";

	m_entries.erase(m_entries.begin()+from,
			m_entries.begin()+to);

//	TRACE << "mci was " << m_current_index << "\n";

	unsigned int new_current_index = 0;
	for (unsigned int i=0, j=0; i<m_queue.size(); ++i)
	{
//	    TRACE << i << " -> " << j << "\n";
	    if (m_current_index == i)
		new_current_index = j;

	    if (m_queue[i] >= to)
		m_queue[j++] = from + (m_queue[i]-to);
	    else if (m_queue[i] < from)
		m_queue[j++] = m_queue[i];
	    else
	    {
		/* otherwise, it's vanished */
		if (m_current_index == i)
		{
//		    TRACE << "mci vanished\n";
		    set = true;
		}
	    }
	}
	m_current_index = new_current_index;
//	TRACE << "mci now " << m_current_index << "\n";

	m_queue.resize(m_entries.size());

	if (m_current_index == m_queue.size())
	    m_current_index = 0; // Clipped off tail of queue
    }

    if (set)
	SetURL();
    if (set_next)
	SetNextURL();
}

void Queue::SetPlayState(output::PlayState state)
{
    switch (state)
    {
    case PLAY:
    {
	if (m_entries.empty())
	    return;

	SetURL();
	SetNextURL();
//	TRACE << "calling Play\n";
	m_impl->m_player->SetPlayState(output::PLAY);
//	TRACE << "play done\n";
	break;
    }
    case PAUSE:
    case STOP:
//	TRACE << "calling SetPlayState\n";
	m_impl->m_player->SetPlayState(state);
//	TRACE << "setplaystate done\n";
	break;
	
    default:
	break;
    }
}

void Queue::SetShuffle(bool whether)
{
    if (whether == m_shuffled)
	return;
    m_shuffled = whether;

    if (m_entries.size() <= 1)
	return;

    util::RecursiveMutex::Lock lock(m_mutex);

    if (whether)
    {
	// Bipartite shuffle -- keep all the songs we've played
	// "behind" us in the shuffled order, all the songs we haven't
	// played yet "ahead".

	unsigned int current_entry = m_queue[m_current_index];
	m_queue.clear();
	m_queue.reserve(m_entries.size());

	for (unsigned int i=0; i<m_entries.size(); ++i)
	{
//	    TRACE << "Entry " << i << " flags " << m_entries[i].flags
//		  << " current=" << current_entry << "\n";
	    if (i != current_entry && (m_entries[i].flags & PLAYED))
		m_queue.push_back(i);
	}
//	TRACE << "Shuffling " << m_queue.size() << " played tracks\n";
	std::random_shuffle(m_queue.begin(), m_queue.end());

	m_current_index = (unsigned int)m_queue.size();
	m_queue.push_back(current_entry);

//	TRACE << "Leaving alone index " << current_entry << "\n";

	for (unsigned int i=0; i<m_entries.size(); ++i)
	{
	    if (i != current_entry && (m_entries[i].flags & PLAYED) == 0)
		m_queue.push_back(i);
	}
//	TRACE << "Shuffling " << (m_queue.size() - m_current_index - 1)
//	      << " unplayed tracks\n";
	std::random_shuffle(m_queue.begin() + m_current_index + 1,
			    m_queue.end());
    }
    else
    {
	unsigned int current_entry = m_queue[m_current_index];
	m_queue.clear();
	m_queue.reserve(m_entries.size());
	for (unsigned int i=0; i<m_entries.size(); ++i)
	    m_queue.push_back(i);
	m_current_index = current_entry;
    }
    SetNextURL();
}

void Queue::Seek(unsigned int index, unsigned int /*ms*/)
{
//    TRACE << "Seeking to index " << index << "\n";
    {
	util::RecursiveMutex::Lock lock(m_mutex);
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
    Entry dbp;
    {
	util::RecursiveMutex::Lock lock(m_mutex);
	if (m_current_index >= m_entries.size())
	    return;
	dbp = m_entries[m_queue[m_current_index]];
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
	util::RecursiveMutex::Lock lock(m_mutex);
	m_impl->m_expected_url = url.c_str();
    }
    m_impl->m_player->SetURL(url, metadata);
}

void Queue::SetNextURL()
{
    Entry dbp;
    {
	util::RecursiveMutex::Lock lock(m_mutex);
	if ((m_current_index+1) >= m_entries.size())
	    return;
	dbp = m_entries[m_queue[m_current_index+1]];
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
	util::RecursiveMutex::Lock lock(m_mutex);
	m_impl->m_expected_next_url = url.c_str();
    }
    m_impl->m_player->SetNextURL(url, metadata);
}

URLPlayer *Queue::GetPlayer() const
{
    return m_impl->m_player;
}

} // namespace output

#ifdef TEST

# include <string.h>
# include "libmediadb/fake_database.h"
# include "test_url_player.h"

class TestObserver: public output::QueueObserver
{
    output::PlayState m_play_state;
    unsigned int m_index;

public:
    TestObserver() : m_play_state(output::PAUSE), m_index(42) {}

    output::PlayState GetPlayState() const { return m_play_state; }
    unsigned int GetIndex() const { return m_index; }

    // Being a QueueObserver
    void OnPlayState(output::PlayState play_state)
    {
	m_play_state = play_state;
    }

    void OnTimeCode(unsigned int index, unsigned int)
    {
	m_index = index;
    }
};

/** Checks that each Entry appears in the Queue exactly once.
 */
static void CheckQueue(const output::Queue *queue)
{
    std::set<unsigned int> theset;
    unsigned int entries = (unsigned int)(queue->entries_end() - queue->entries_begin());

    for (output::Queue::queue_iterator i = queue->queue_begin();
	 i != queue->queue_end();
	 ++i)
    {
	unsigned int entry = *i;
	if (theset.find(entry) != theset.end())
	    TRACE << "Duplicate entry " << entry << " in queue\n";
	assert(theset.find(entry) == theset.end());
	if (entry >= entries)
	    TRACE << "Oversize queue entry " << entry << "/" << entries
		  << "\n";
	assert(entry < entries);
	theset.insert(entry);
    }

    if (theset.size() != entries)
	TRACE << "Queue mismatch, contains only " << theset.size()
	      << "/" << entries << " entries\n";
    assert(theset.size() == entries);
}

static void TestQueueing1(mediadb::Database *db)
{
    output::TestURLPlayer player;
    output::Queue queue(&player);

    /* Tests queue-order (shuffled order, playback order) */

    CheckQueue(&queue);
    assert(queue.Count() == 0);
    queue.Add(db, 1);
    CheckQueue(&queue);
    assert(queue.Count() == 1);  // [1]
    queue.Add(db, 2);
    CheckQueue(&queue);
    assert(queue.Count() == 2);  // [1,2]
    queue.QueueInsert(db, 3, 1);
    CheckQueue(&queue);
    assert(queue.Count() == 3);  // [1,3,2]
    assert(queue.EntryAt(queue.QueueAt(0)).id == 1);
    assert(queue.EntryAt(queue.QueueAt(1)).id == 3);
    assert(queue.EntryAt(queue.QueueAt(2)).id == 2);
    queue.QueueInsert(db, 4, 0);
    CheckQueue(&queue);
    assert(queue.Count() == 4);  // [4,1,3,2]
    assert(queue.EntryAt(queue.QueueAt(0)).id == 4);
    assert(queue.EntryAt(queue.QueueAt(1)).id == 1);
    assert(queue.EntryAt(queue.QueueAt(2)).id == 3);
    assert(queue.EntryAt(queue.QueueAt(3)).id == 2);
    queue.QueueInsert(db, 5, 4);
    CheckQueue(&queue);
    assert(queue.Count() == 5);  // [4,1,3,2,5]
    assert(queue.EntryAt(queue.QueueAt(0)).id == 4);
    assert(queue.EntryAt(queue.QueueAt(1)).id == 1);
    assert(queue.EntryAt(queue.QueueAt(2)).id == 3);
    assert(queue.EntryAt(queue.QueueAt(3)).id == 2);
    assert(queue.EntryAt(queue.QueueAt(4)).id == 5);
    queue.QueueRemove(1,3); 
    CheckQueue(&queue);
    assert(queue.Count() == 3);  // [4,2,5]
    assert(queue.EntryAt(queue.QueueAt(0)).id == 4);
    assert(queue.EntryAt(queue.QueueAt(1)).id == 2);
    assert(queue.EntryAt(queue.QueueAt(2)).id == 5);

    /* Test randomness of shuffle (a bit) */
    srand(42);
    queue.EntryRemove(0,3);
    assert(queue.Count() == 0);

    unsigned int count[4][4];

    memset(count, '\0', sizeof(count));
    queue.SetShuffle(true);

    for (unsigned int i=0; i<100; ++i)
    {
	for (unsigned int j=0; j<4; ++j)
	    queue.EntryInsert(db, j+1, j);

	for (unsigned int j=0; j<4; ++j)
	    count[queue.QueueAt(j)][j] ++;

	queue.EntryRemove(0,4);
    }

    for (unsigned int j=0; j<4; ++j)
    {
//	TRACE << "item " << j << ": "
//	      << count[j][0] << " " << count[j][1] << " "
//	      << count[j][2] << " " << count[j][3] << "\n";
    }
}

static void TestQueueing2(mediadb::Database *db)
{
    output::TestURLPlayer player;
    output::Queue queue(&player);

    /* Tests entry-order (unshuffled order) */

    CheckQueue(&queue);
    assert(queue.Count() == 0);
    queue.Add(db, 1);
    CheckQueue(&queue);
    assert(queue.Count() == 1);  // [1]
    queue.Add(db, 2);
    CheckQueue(&queue);
    assert(queue.Count() == 2);  // [1,2]
    queue.EntryInsert(db, 3, 1);
    CheckQueue(&queue);
    assert(queue.Count() == 3);  // [1,3,2]
    assert(queue.EntryAt(0).id == 1);
    assert(queue.EntryAt(1).id == 3);
    assert(queue.EntryAt(2).id == 2);
    queue.EntryInsert(db, 4, 0);
    CheckQueue(&queue);
    assert(queue.Count() == 4);  // [4,1,3,2]
    assert(queue.EntryAt(0).id == 4);
    assert(queue.EntryAt(1).id == 1);
    assert(queue.EntryAt(2).id == 3);
    assert(queue.EntryAt(3).id == 2);
    queue.EntryInsert(db, 5, 4);
    CheckQueue(&queue);
    assert(queue.Count() == 5);  // [4,1,3,2,5]
    assert(queue.EntryAt(0).id == 4);
    assert(queue.EntryAt(1).id == 1);
    assert(queue.EntryAt(2).id == 3);
    assert(queue.EntryAt(3).id == 2);
    assert(queue.EntryAt(4).id == 5);
    queue.EntryRemove(1,3); 
    CheckQueue(&queue);
    assert(queue.Count() == 3);  // [4,2,5]
    assert(queue.EntryAt(0).id == 4);
    assert(queue.EntryAt(1).id == 2);
    assert(queue.EntryAt(2).id == 5);
}

static void TestPlaying(mediadb::Database *db)
{
    output::TestURLPlayer player;
    output::Queue queue(&player);
    TestObserver obs;
    queue.AddObserver(&obs);

    for (unsigned int i=1; i<=10; ++i)
	queue.Add(db, i);

    CheckQueue(&queue);

    assert(queue.Count() == 10);
    queue.SetPlayState(output::PLAY);
    assert(player.GetPlayState() == output::PLAY);
    assert(player.GetURL() == "test:1");
    assert(player.GetNextURL() == "test:2");
    assert(obs.GetPlayState() == output::PLAY);
    assert(obs.GetIndex() == 0);
    player.FinishTrack();
    assert(player.GetPlayState() == output::PLAY);
    assert(player.GetURL() == "test:2");
    assert(player.GetNextURL() == "test:3");
    assert(queue.GetCurrentIndex() == 1);
    assert(obs.GetPlayState() == output::PLAY);
    assert(obs.GetIndex() == 1);
    player.FinishTrack();
    assert(player.GetPlayState() == output::PLAY);
    assert(player.GetURL() == "test:3");
    assert(player.GetNextURL() == "test:4");
    assert(queue.GetCurrentIndex() == 2);
    assert(obs.GetPlayState() == output::PLAY);
    assert(obs.GetIndex() == 2);

    /* Bipartite, reversable shuffle
     *
     * We've now played test:1, test:2, and part of test:3.
     * Shuffling should now result in [1,2] 3 [4,5,6,7,8,9,10] where the square
     * brackets represent "an arbitrary shuffle of".
     */
    CheckQueue(&queue);
    queue.SetShuffle(true);
    CheckQueue(&queue);
    assert(queue.Count() == 10);
    assert(queue.GetCurrentIndex() == 2);
    assert(player.GetURL() == "test:3");
    player.FinishTrack();
    assert(player.GetPlayState() == output::PLAY);
    // We don't know what the next track is, but it can't be 1, 2, or 3
    unsigned int trackX = atoi(player.GetURL().c_str() + 5);
    assert(player.GetURL() != "test:1");
    assert(player.GetURL() != "test:2");
    assert(player.GetURL() != "test:3");
    assert(player.GetNextURL() != "test:1");
    assert(player.GetNextURL() != "test:2");
    assert(player.GetNextURL() != "test:3");
    unsigned int next_track = atoi(player.GetNextURL().c_str() + 5);
    assert(queue.GetCurrentIndex() == 3);
    assert(obs.GetPlayState() == output::PLAY);
    assert(obs.GetIndex() == 3);
    player.FinishTrack();
    assert(player.GetPlayState() == output::PLAY);
    // We don't know what the next track is, but it can't be 1, 2, 3, or X
    unsigned int trackY = atoi(player.GetURL().c_str() + 5);
    assert(player.GetURL() != "test:1");
    assert(player.GetURL() != "test:2");
    assert(player.GetURL() != "test:3");
    assert(trackY != trackX);
    assert(trackY == next_track);
    assert(player.GetNextURL() != "test:1");
    assert(player.GetNextURL() != "test:2");
    assert(player.GetNextURL() != "test:3");
    assert(queue.GetCurrentIndex() == 4);
    assert(obs.GetPlayState() == output::PLAY);
    assert(obs.GetIndex() == 4);

    /* Unshuffling should result in the current index changing to Y's natural
     * position
     */
    CheckQueue(&queue);
    queue.SetShuffle(false);
    CheckQueue(&queue);
    assert(queue.Count() == 10);
    assert(queue.GetCurrentIndex() == trackY-1);

    /* Now shuffling again should result in
     * [1,2,3,X] Y [4,5,6,7,8,9,10 - X - Y]
     */
    CheckQueue(&queue);
    queue.SetShuffle(true);
    CheckQueue(&queue);
    assert(queue.Count() == 10);
    assert(queue.GetCurrentIndex() == 4);
    assert(obs.GetPlayState() == output::PLAY);
    assert(obs.GetIndex() == 4);

    /* Note that CheckQueue effectively checks that all the queue entries are
     * unique, so we don't need to check that here.
     */
    for (unsigned int i=0; i<4; ++i)
    {
	unsigned int id = queue.EntryAt(queue.QueueAt(i)).id;
	assert(id == 1 || id == 2 || id == 3 || id == trackX);
    }
    assert(queue.EntryAt(queue.QueueAt(4)).id == trackY);
    for (unsigned int i=5; i<10; ++i)
    {
	unsigned int id = queue.EntryAt(queue.QueueAt(i)).id;
	assert(id >= 4 && id <= 10 && id != trackX && id != trackY);
    }

    /* Test that removal of non-playing items doesn't interrupt playback */
    queue.EntryRemove(1,2);
    CheckQueue(&queue);         // [1,3,X] Y [4,5,6,7,8,9,10 - X - Y]
    assert(queue.Count() == 9);
    assert(queue.GetCurrentIndex() == 3);
    assert(obs.GetPlayState() == output::PLAY);
    //assert(obs.GetIndex() == 3);

    /* Pick a track to remove that isn't X or Y */
    unsigned int trackZ = 6;
    if (trackZ == trackX || trackZ == trackY)
	++trackZ;
    if (trackZ == trackX || trackZ == trackY)
	++trackZ;
//    TRACE << "Removing entry-" << trackZ << "\n";
    queue.EntryRemove(trackZ -2, trackZ+1 -2);
    CheckQueue(&queue);         // [1,3,X] Y [4,5,6,7,8,9,10 - X - Y - Z]
    assert(queue.Count() == 8);
    assert(queue.GetCurrentIndex() == 3);
    assert(obs.GetPlayState() == output::PLAY);

//    TRACE << "Removing queue-1\n";
    queue.QueueRemove(1,2);
    CheckQueue(&queue);         // [1,3,X-P] Y [4,5,6,7,8,9,10 - X - Y - Z]
    assert(queue.Count() == 7);
    assert(queue.GetCurrentIndex() == 2);
    assert(obs.GetPlayState() == output::PLAY);

//    TRACE << "Removing queue-3\n";
    queue.QueueRemove(3,4);
    CheckQueue(&queue);         // [1,3,X-P] Y [4,5,6,7,8,9,10 - X - Y - Z - Q]
    assert(queue.Count() == 6);
    assert(queue.GetCurrentIndex() == 2);
    assert(obs.GetPlayState() == output::PLAY);

    /* Test removal of playing track */
    queue.QueueRemove(2, 3);
    CheckQueue(&queue);         // [1,3,X-P] R [4,5,6,7,8,9,10 -X -Y -Z -Q -R]
    assert(queue.Count() == 5);
    assert(queue.GetCurrentIndex() == 2);
    assert(obs.GetPlayState() == output::PLAY);
    unsigned int trackR = queue.EntryAt(queue.QueueAt(2)).id;
    assert(trackR >= 4 && trackR <= 10);
    assert(trackR != trackX && trackR != trackY && trackR != trackZ);
}

int main()
{
    mediadb::FakeDatabase db;
    db::RecordsetPtr rs = db.CreateRecordset();
    for (unsigned int i=1; i<=10; ++i)
    {
	rs->AddRecord();
	rs->SetInteger(mediadb::ID, i);
	rs->SetInteger(mediadb::TITLE, i);
	rs->Commit();
    }

    TestQueueing1(&db);
    TestQueueing2(&db);
    TestPlaying(&db);
    return 0;
}

#endif
