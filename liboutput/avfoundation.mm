#include "config.h"
#include "avfoundation.h"
#include "libutil/observable.h"
#include "libutil/errors.h"

#import <Foundation/NSObject.h>
#import <Foundation/NSError.h>
#import <Foundation/NSString.h>
#import <AVFoundation/AVAudioPlayer.h>

namespace output { namespace avfoundation { class Observer; } }

/** A delegate (Objective-C word for observer) which gets
 * notifications back from the AVAudioPlayer.
 */

@interface OutputDelegate : NSObject
{
    output::avfoundation::Observer *m_observer;
}
- (id)initWithObserver: (output::avfoundation::Observer*)obs;
- (void)audioPlayerBeginInterruption:(AVAudioPlayer *)player;
- (void)audioPlayerEndInterruption:(AVAudioPlayer *)player;
- (void)audioPlayerDidFinishPlaying:(AVAudioPlayer *)player successfully:(BOOL)flag;
- (void)audioPlayerDecodeErrorDidOccur:(AVAudioPlayer *)player error:(NSError *)error;
- (void)dealloc;

@end

namespace output {

namespace avfoundation {

class Observer
{
public:
    virtual ~Observer() {}
    virtual void OnBeginInterruption() = 0;
    virtual void OnEndInterruption() = 0;

    virtual void OnFinishedPlaying(id player, bool successfully) = 0;
    virtual void OnDecodeError(id player, NSError *error) = 0;
};


        /* URLPlayer::Impl */


class URLPlayer::Impl: public util::Observable<URLObserver>, public Observer
{
    id m_audio_player;
    id m_next_audio_player;
    std::string m_next_url;
    id m_output_delegate;
    bool m_playing;

public:
    Impl();
    ~Impl();
    unsigned int SetPlayState(output::PlayState state);
    unsigned int SetURL(const std::string&);
    unsigned int SetNextURL(const std::string&);
    unsigned int Seek(unsigned int ms);

    void OnBeginInterruption();
    void OnEndInterruption();
    void OnFinishedPlaying(id player, bool successfully);
    void OnDecodeError(id player, NSError *error);
};

URLPlayer::Impl::Impl()
    : m_audio_player(nil),
      m_next_audio_player(nil),
      m_output_delegate( [ [ OutputDelegate alloc ] initWithObserver: this ] ),
      m_playing(false)
{
}

URLPlayer::Impl::~Impl()
{
    if (m_next_audio_player)
    {
	[ m_next_audio_player stop ];
	[ m_next_audio_player setDelegate: nil ];
	[ m_next_audio_player release ];
    }
    if (m_audio_player)
    {
	[ m_audio_player stop ];
	[ m_audio_player setDelegate: nil ];
	[ m_audio_player release ];
    }
    [ m_output_delegate release ];
}

unsigned int URLPlayer::Impl::SetURL(const std::string& url)
{
    if (m_audio_player)
    {
	[ m_next_audio_player stop ];
	[ m_audio_player setDelegate: nil ];
	[ m_audio_player release ];
    }

    m_audio_player = [ AVAudioPlayer alloc ];
    [ m_audio_player 
	initWithContentsOfURL: [ NSString stringWithUTF8String: url.c_str() ]
	];

    [ m_audio_player setDelegate: m_output_delegate ];

    Fire(&URLObserver::OnURL, url);
    
    if (m_playing)
    {
	BOOL b = [ m_audio_player play ];
	m_playing = (b == YES);
	if (!m_playing)
	{
	    Fire(&URLObserver::OnPlayState, STOP);
	    return EINVAL;
	}
    }

    return 0;
}

unsigned int URLPlayer::Impl::SetNextURL(const std::string& url)
{
    if (m_next_audio_player)
    {
	[ m_next_audio_player stop ];
	[ m_next_audio_player setDelegate: nil ];
	[ m_next_audio_player release ];
    }

    m_next_audio_player = [ [ AVAudioPlayer alloc ]
			      initWithContentsOfURL: [ NSString stringWithUTF8String: url.c_str() ] ];

    m_next_url = url;

    [ m_next_audio_player setDelegate: m_output_delegate ];

    return 0;
}

unsigned int URLPlayer::Impl::SetPlayState(output::PlayState state)
{
    switch (state)
    {
    case PLAY:
	if (m_audio_player)
	{
	    [ m_audio_player play ];
	    m_playing = true;
	}
	break;
    case PAUSE:
	if (m_audio_player)
	{
	    [ m_audio_player pause ];
	}
	break;
    case STOP:
	if (m_audio_player)
	{
	    [ m_audio_player stop ];
	    [ m_audio_player setCurrentTime: 0.0 ];
	    m_playing = false;
	}
	break;
    }

    return 0;
}

unsigned int URLPlayer::Impl::Seek(unsigned int ms)
{
    if (m_audio_player)
	[ m_audio_player setCurrentTime: (double)(ms*0.001) ];
    return 0;
}

void URLPlayer::Impl::OnBeginInterruption()
{
}
 
void URLPlayer::Impl::OnEndInterruption()
{
}

void URLPlayer::Impl::OnFinishedPlaying(id player, bool)
{
    if (player == m_audio_player)
    {
	[ m_audio_player release ];
	if (m_next_audio_player)
	{
	    m_audio_player = m_next_audio_player;
	    m_next_audio_player = nil;
	    [ m_audio_player play ];
	    Fire(&URLObserver::OnURL, m_next_url);
	}
    }
}

void URLPlayer::Impl::OnDecodeError(id player, NSError*)
{
    if (player == m_audio_player)
    {
	[ m_audio_player release ];
	if (m_next_audio_player)
	{
	    m_audio_player = m_next_audio_player;
	    m_next_audio_player = nil;
	    [ m_audio_player play ];
	    Fire(&URLObserver::OnURL, m_next_url);
	}
    }
}


        /* URLPlayer itself */


URLPlayer::URLPlayer()
    : m_impl(new Impl)
{
}

URLPlayer::~URLPlayer()
{
    delete m_impl;
}

void URLPlayer::AddObserver(URLObserver *obs)
{
    m_impl->AddObserver(obs);
}

void URLPlayer::RemoveObserver(URLObserver *obs)
{
    m_impl->RemoveObserver(obs);
}

unsigned int URLPlayer::Seek(unsigned int ms)
{
    return m_impl->Seek(ms);
}

unsigned int URLPlayer::SetURL(const std::string& url, const std::string&)
{
    return m_impl->SetURL(url);
}

unsigned int URLPlayer::SetNextURL(const std::string& url, const std::string&)
{
    return m_impl->SetNextURL(url);
}

unsigned int URLPlayer::SetPlayState(output::PlayState state)
{
    return m_impl->SetPlayState(state);
}

} // namespace avframework
} // namespace output


/* The ObjC-to-C++ delegate */


@implementation OutputDelegate

- (id)initWithObserver: (output::avfoundation::Observer*)obs
{
    m_observer = obs;
    return self;
}

- (void)audioPlayerDidFinishPlaying:(AVAudioPlayer *)player successfully:(BOOL)flag
{
    m_observer->OnFinishedPlaying(player, flag);
}

- (void)audioPlayerDecodeErrorDidOccur:(AVAudioPlayer *)player error:(NSError *)error
{
    m_observer->OnDecodeError(player, error);
}

- (void)audioPlayerBeginInterruption:(AVAudioPlayer *)player
{
    m_observer->OnBeginInterruption();
}

- (void)audioPlayerEndInterruption:(AVAudioPlayer *)player
{
    m_observer->OnEndInterruption();
}

- (void)dealloc
{
    [ super dealloc ];
}

@end
