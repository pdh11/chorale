#ifndef OUTPUT_QUEUE_H
#define OUTPUT_QUEUE_H 1

#include "playstate.h"
#include "libutil/mutex.h"
#include <vector>
#include <string>

namespace mediadb { class Database; }
namespace mediadb { class Registry; }

/** Classes for playback of media items.
 */
namespace output {

class URLPlayer;

/** Observer interface for Queue, called back on various audio threads
 */
class QueueObserver
{
public:
    virtual ~QueueObserver() {}

    virtual void OnPlayState(output::PlayState) {}
    virtual void OnTimeCode(unsigned int /*index*/, unsigned int /*sec*/) {}
    virtual void OnError(unsigned int /*index*/, unsigned int /*error*/) {}
    virtual void OnEnd() {}
};

/** A queue of URLs waiting to play back on an audio output device.
 *
 * The queue can be shuffled, and then subsequently unshuffled,
 * returning to its original order. (A further shuffle will produce a
 * new, different shuffled order.)
 *
 * The URLs on the queue can be iterated in two different orders: the
 * original order (entries_begin/entries_end), and the shuffled order
 * (queue_begin/queue_end). If shuffle isn't active, these will be the
 * same.
 *
 * In other words, the difference between (in Winamp terms) "random"
 * and "shuffle" is a UI difference, not a Queue difference. "Winamp
 * random" means engage shuffle, but keep displaying the tracks in
 * their original order, so playback appears to leap about in the
 * list. "Winamp shuffle" means engage shuffle, displaying the tracks
 * in the shuffled order, so playback appears to follow the list
 * sequentially.
 */
class Queue
{
public:
    struct Entry
    {
	mediadb::Database *db;
	unsigned int id;
	unsigned int flags;
    };

    /** Values for Entry.flags
     */
    enum {
	PLAYED = 0x1
    };

private:
    /** The mutex protects m_current_index, m_entries, and m_queue.
     *
     * Because the UI thread is the only one to modify m_queue, it
     * (and only it) can *read* the queue without taking a
     * lock. Modifying the queue, or changing m_current_index from any
     * thread, requires the lock.
     */
    util::RecursiveMutex m_mutex;
    unsigned m_current_index; ///< Index into m_queue
    std::vector<unsigned int> m_queue; ///< Offsets into m_entries
    std::vector<Entry> m_entries;
    std::string m_name;
    bool m_shuffled;

    class Impl;
    Impl *m_impl;
    friend class Impl;

    void SetURL();
    void SetNextURL();
    
public:
    explicit Queue(URLPlayer *player);
    ~Queue();


    // Transport controls


    void SetPlayState(output::PlayState);
    void Seek(unsigned int index, unsigned int ms);
    void SetShuffle(bool whether);


    // Item access (const)


    /** Get the number of items in the queue */
    unsigned int Count() const { return (unsigned int)m_entries.size(); }
    unsigned int GetCurrentIndex() const { return m_current_index; }

    /// Iterator for original (unshuffled) order, cf queue_iterator
    typedef std::vector<Entry>::const_iterator entries_iterator;

    /// Begin-iterator for original (unshuffled) order
    entries_iterator entries_begin() const { return m_entries.begin(); }

    /// End-iterator for original (unshuffled) order
    entries_iterator entries_end()   const { return m_entries.end(); }

    /// Get entry by original (unshuffled) order
    const Entry& EntryAt(unsigned int index) const { return m_entries[index]; }

    /// Iterator for playback (shuffled) order, cf entries_iterator
    typedef std::vector<unsigned int>::const_iterator queue_iterator;

    /// Begin-iterator for playback (shuffled) order
    queue_iterator queue_begin() const { return m_queue.begin(); }

    /// End-iterator for playback (shuffled) order
    queue_iterator queue_end()   const { return m_queue.end(); }

    /// Get entry by playback (shuffled) order
    unsigned int QueueAt(unsigned int index) const
    { return m_queue[index]; }


    // Adding and removing


    /** Add an item to the end of the queue and the entries.
     */
    void Add(mediadb::Database *db, unsigned int id);

    /** The item becomes the new item in position "where" in the queue.
     *
     * That is, where=0 means insert at top, where=size means insert
     * at end. This operation changes m_current_index if it's >=where.
     */
    void QueueInsert(mediadb::Database *db, unsigned int id,
		     unsigned int where);

    /** The item becomes the new item in position "where" in the
     * entries array.
     *
     * That is, where=0 means insert at top, where=size means insert
     * at end. The item is added to the queue in the corresponding
     * position (if unshuffled) or at a random point in the future (if
     * shuffled).
     */
    void EntryInsert(mediadb::Database *db, unsigned int id,
		     unsigned int where);

    /** Remove items from, from+1, ... , from+to-1 (as numbered in the
     * queue).
     */
    void QueueRemove(unsigned int from, unsigned int to);

    /** Remove items from, from+1, ... , from+to-1 (as numbered in the
     * entries array).
     */
    void EntryRemove(unsigned int from, unsigned int to);


    // Housekeeping


    void SetName(const std::string& name) { m_name = name; }
    const std::string& GetName() const { return m_name; }
    void AddObserver(QueueObserver*);
    URLPlayer *GetPlayer() const;
};

} // namespace output

#endif
