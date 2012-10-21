#ifndef OUTPUT_QUEUE_H
#define OUTPUT_QUEUE_H 1

#include "playstate.h"
#include <boost/thread/mutex.hpp>
#include <vector>

namespace mediadb { class Database; }

/** Classes for playback of media items.
 */
namespace output {

class URLPlayer;

/** Output observer, called back on various audio threads
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

class Queue
{
public:
    struct Pair
    {
	mediadb::Database *db;
	unsigned int id;
    };

private:
    /** The mutex protects m_current_index and m_queue.
     *
     * Because the UI thread is the only one to modify m_queue, it
     * (and only it) can *read* the queue without taking a
     * lock. Modifying the queue, or changing m_current_index from any
     * thread, requires the lock.
     */
    boost::mutex m_mutex;
    unsigned m_current_index;
    std::vector<Pair> m_queue;
    std::string m_name;

    class Impl;
    Impl *m_impl;
    friend class Impl;

    void SetURL();
    void SetNextURL();
    
public:
    explicit Queue(URLPlayer *player);
    ~Queue();

    void SetName(const std::string& name) { m_name = name; }
    const std::string& GetName() const { return m_name; }

    void Seek(unsigned int index, unsigned int ms);

    void SetPlayState(output::PlayState);

    void AddObserver(QueueObserver*);
    
    typedef std::vector<Pair>::const_iterator const_iterator;

    const_iterator begin() { return m_queue.begin(); }
    const_iterator end()   { return m_queue.end(); }

    const Pair& At(unsigned int index) { return m_queue[index]; }

    void Add(mediadb::Database *db, unsigned int id);

    unsigned int GetCurrentIndex() const { return m_current_index; }
};

}; // namespace output

#endif
