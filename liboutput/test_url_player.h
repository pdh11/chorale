#ifndef LIBOUTPUT_TEST_URL_PLAYER_H
#define LIBOUTPUT_TEST_URL_PLAYER_H 1

#include "urlplayer.h"
#include "libutil/observable.h"

namespace output {

class TestURLPlayer: public URLPlayer, public util::Observable<URLObserver>
{
    std::string m_url;
    std::string m_next_url;
    PlayState m_play_state;

public:
    TestURLPlayer() : m_play_state(STOP) {}

    std::string GetURL() const { return m_url; }
    std::string GetNextURL() const { return m_next_url; }
    PlayState GetPlayState() const { return m_play_state; }
    void FinishTrack()
    {
	m_url = m_next_url;
	m_next_url.clear();
	if (m_url.empty())
	{
	    m_play_state = STOP;
	    Fire(&URLObserver::OnPlayState, STOP);
	}
	else
	{
	    Fire(&URLObserver::OnURL, m_url);
	}
    }

    // Being a URLPlayer
    unsigned int SetURL(const std::string& url, const std::string&)
    {
	m_url = url;
	Fire(&URLObserver::OnURL, m_url);
	return 0;
    }

    unsigned int SetNextURL(const std::string& url, const std::string&)
    {
	m_next_url = url;
	return 0;
    }

    unsigned int SetPlayState(output::PlayState play_state)
    {
	m_play_state = play_state;
	Fire(&URLObserver::OnPlayState, play_state);
	return 0;
    }

    unsigned int Seek(unsigned int) { return 0; }

    void AddObserver(URLObserver *observer)
    {
	util::Observable<URLObserver>::AddObserver(observer);
    }

    void RemoveObserver(URLObserver *observer)
    {
	util::Observable<URLObserver>::RemoveObserver(observer);
    }
};

} // namespace output

#endif
