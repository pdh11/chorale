#include "config.h"
#include "gstreamer.h"

#if HAVE_GSTREAMER

#include <gst/gst.h>
#include "libutil/trace.h"
#include "libutil/observable.h"
#include "libutil/mutex.h"
#include "libutil/bind.h"
#include <boost/format.hpp>

namespace output {

namespace gstreamer {

class GSTInitHelper
{
    static unsigned int sm_count;
public:
    GSTInitHelper()
    {
	GError *gerror = NULL;
	if (!sm_count)
	{
	    TRACE << "Calling gst_init\n";
	    gst_init_check(NULL, NULL, &gerror);
	}
	++sm_count;
    }
    ~GSTInitHelper()
    {
	--sm_count;
	if (!sm_count)
	{
	    TRACE << "No more users, calling gst_deinit\n";
	    gst_deinit();
	}
    }
};

unsigned int GSTInitHelper::sm_count = 0;

class URLPlayer::Impl: public util::Observable<URLObserver>
{
    int m_card;
    int m_device;

    GSTInitHelper m_init;
    GMainContext *m_context;
    GMainLoop *m_loop;
    GstElement *m_play;
    GstBus *m_bus;

    util::Mutex m_mutex;
    util::Condition m_cstarted;
    std::string m_url;
    std::string m_next_url;
    GstState m_last_state;
    unsigned int m_last_timecode_sec;
    bool m_async_state_change;
    bool m_got_early_warning;

    volatile bool m_started;
    volatile bool m_exiting;

    util::Thread m_thread;

    unsigned int Run();

    static gboolean StaticBusCallback(GstBus*, GstMessage*, gpointer);
    gboolean OnBusCallback(GstBus*, GstMessage*);

    static gboolean StaticAlarmCallback(gpointer);
    gboolean OnAlarm();

    static gboolean StaticStartup(gpointer);
    gboolean OnStartup();

    static void StaticAboutToFinishCallback(GstElement*, gpointer);
    bool OnAboutToFinish(GstElement*);

public:
    Impl(int card, int device);
    ~Impl();

    unsigned int SetURL(const std::string&);
    unsigned int SetNextURL(const std::string&);
    unsigned int SetPlayState(output::PlayState);
    unsigned int Seek(unsigned int ms);
};

URLPlayer::Impl::Impl(int card, int device)
    : m_card(card),
      m_device(device),
      m_context(NULL),
      m_loop(NULL),
      m_play(NULL),
      m_bus(NULL),
      m_last_state(GST_STATE_NULL),
      m_last_timecode_sec(UINT_MAX),
      m_async_state_change(false),
      m_got_early_warning(false),
      m_started(false),
      m_exiting(false),
      m_thread(util::Bind(this).To<&Impl::Run>())
{
    if (!g_thread_supported())
    {
	TRACE << "Initing g threads\n";
	g_thread_init(NULL);
    }
    else
	TRACE << "no need\n";

    /* For some reason, gstreamer deadlocks loading the gnomevfs
     * element if we call set_element_state on playbin from the wrong
     * thread. (With stock gstreamer-0.10.15, g-p-b 0.10.15, g-p-g
     * 0.10.6, g-p-u 0.10.6, and even with the Ubuntu 7.10 versions.)
     *
     * This problem goes away if we force-load the gnomevfs element first.
     */    
    GstElement *gnomevfs = gst_element_factory_make("gnomevfssrc", "temp");
    gst_object_unref(GST_OBJECT(gnomevfs));
}

URLPlayer::Impl::~Impl()
{
    m_exiting = true;
    TRACE << "URLPlayer::~Impl\n";
    if (m_play)
	gst_element_set_state(m_play, GST_STATE_NULL);
    if (m_loop)
	g_main_loop_quit(m_loop);
    TRACE << "URLPlayer::~Impl joining\n";
    m_thread.Join();
    TRACE << "URLPlayer::~Impl joined\n";
}

gboolean URLPlayer::Impl::StaticStartup(gpointer data)
{
    Impl *impl = (Impl*)data;
    return impl->OnStartup();
}

/* GStreamer crashes if we call gst_element_factory_make from two threads
 * simultaneously.
 */
static util::Mutex gstreamer_unnecessarily_not_re_entrant;

gboolean URLPlayer::Impl::OnStartup()
{
    {
	util::Mutex::Lock lock(gstreamer_unnecessarily_not_re_entrant);
    
	/** If it exists, playbin2 is newer and better than playbin */
//	m_play = gst_element_factory_make("playbin2", "play");
	if (!m_play)
	{
	    m_play = gst_element_factory_make("playbin", "play");
	    if (!m_play)
	    {
		TRACE << "Oh-oh, no playbin\n";
		return false;
	    }
	}

	GstElement *alsasink = NULL;
	std::string alsadevice;

	if (m_card >= 0 && m_device >= 0)
	{
	    alsadevice = (boost::format("plughw:%d,%d") % m_card % m_device).str();
	    
	    alsasink = gst_element_factory_make("alsasink", "audio-sink");
	    g_object_set(m_play, "audio-sink", alsasink, NULL);
	    g_object_set(alsasink, "device", alsadevice.c_str(), NULL);
	}
	// Otherwise it uses the "default" ALSA output

	gst_element_set_state(m_play, GST_STATE_NULL);
	m_bus = gst_pipeline_get_bus(GST_PIPELINE(m_play));
//    TRACE << "Apparently bus=" << m_bus << " (this=" << this << ")\n";

	GSource *src = gst_bus_create_watch(m_bus);

	// Hurrah for filthy GLib non-typesafety
	g_source_set_callback(src, (GSourceFunc)&StaticBusCallback, this, 
			      NULL);
	g_source_attach(src, m_context);

	gulong rc = g_signal_connect(m_play, "about-to-finish",
				     G_CALLBACK(&StaticAboutToFinishCallback),
				     this);

	TRACE << "g_signal_connect returned " << rc << "\n";
    }

    {
	util::Mutex::Lock lock(m_mutex);
	m_started = true;
	m_cstarted.NotifyAll();
    }

    return false; // Don't call me again
}

unsigned int URLPlayer::Impl::Run()
{
    TRACE << "starting\n";

    GSource *timersource;

    m_context = g_main_context_new();
    g_main_context_acquire(m_context);

    m_loop = g_main_loop_new(m_context, false);

    GSource *idlesource = g_timeout_source_new(100);
    g_source_set_callback(idlesource, &StaticStartup, this, NULL);
    g_source_attach(idlesource, m_context);

    timersource = g_timeout_source_new(500);
    g_source_set_callback(timersource, &StaticAlarmCallback, this, NULL);
    g_source_attach(timersource, m_context);
	
    TRACE << "running\n";

    /* now run */
    g_main_loop_run(m_loop);

    TRACE << "ran, exiting\n";
    
    /* also clean up */
    {
	util::Mutex::Lock lock(gstreamer_unnecessarily_not_re_entrant);
    
	if (m_play)
	    gst_element_set_state(m_play, GST_STATE_NULL);
	if (m_bus)
	    gst_object_unref(GST_OBJECT(m_bus));
	if (m_play)
	    gst_object_unref(GST_OBJECT(m_play));
//    if (alsasink)
//	gst_object_unref(GST_OBJECT(alsasink));
	g_main_loop_unref(m_loop);
	m_loop = NULL;
	g_source_destroy(timersource);
//    g_source_destroy(src);
	g_main_context_unref(m_context);

    }

    TRACE << "now falling off thread\n";
    return 0;
}

unsigned int URLPlayer::Impl::SetURL(const std::string& url)
{
    {
	util::Mutex::Lock lock(m_mutex);
	m_url = url.c_str(); // Deep copy for thread-safety
	m_next_url.clear();

	if (!m_started)
	{
	    TRACE << "Waiting for start\n";
	    m_cstarted.Wait(lock, 60);
	}
    }

    if (!m_play)
	return ENOENT;

    TRACE << "Setting ready\n";
    bool playing = m_last_state == GST_STATE_PLAYING;
    if (playing)
	gst_element_set_state(m_play, GST_STATE_READY);
    TRACE << "Calling g_object_set(" << m_url << ") on " << m_play << "\n";
    g_object_set(G_OBJECT(m_play), "uri", m_url.c_str(), NULL);
    TRACE << "g_object_set returned\n";
    if (playing)
	gst_element_set_state(m_play, GST_STATE_PLAYING);
    Fire(&URLObserver::OnURL, m_url);

    return 0;
}

unsigned int URLPlayer::Impl::SetNextURL(const std::string& url)
{
    util::Mutex::Lock lock(m_mutex);
    m_next_url = url.c_str(); // Deep copy for thread-safety
    return 0;
}

unsigned int URLPlayer::Impl::SetPlayState(output::PlayState state)
{
    {
	util::Mutex::Lock lock(m_mutex);
	if (!m_started)
	{
	    TRACE << "Waiting for start again\n";
	    m_cstarted.Wait(lock, 60);
	}
    }

    if (!m_play)
	return ENOENT;

    switch (state)
    {
    case PLAY:
    {
//	TRACE << "Calling set_state\n";
//	GstStateChangeReturn sr = 

	gst_element_set_state(m_play, GST_STATE_PLAYING);

//	TRACE << "set_state returned " << sr << "\n";
	break;
    }
    case PAUSE:
	gst_element_set_state(m_play, GST_STATE_PAUSED);
	break;

    case STOP:
	gst_element_set_state(m_play, GST_STATE_NULL);
	break;
    }

    return 0;
}

unsigned int URLPlayer::Impl::Seek(unsigned int ms)
{
    gint64 ns = ((gint64)ms) * 1000000;
    gst_element_seek(m_play, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
		     GST_SEEK_TYPE_SET, ns, GST_SEEK_TYPE_NONE, 0);
    return 0;
}

gboolean URLPlayer::Impl::StaticBusCallback(GstBus *bus, GstMessage *message,
					    gpointer data)
{
//    TRACE << "sbc(" << bus << "," << message << "," << data << ")\n";
    Impl *impl = (Impl*)data;
    return impl->OnBusCallback(bus, message);
}

void URLPlayer::Impl::StaticAboutToFinishCallback(GstElement *playbin,
						  gpointer data)
{
    Impl *impl = (Impl*)data;
    impl->m_got_early_warning = true;
    impl->OnAboutToFinish(playbin);
}

/** Returns true if carrying on
 */
bool URLPlayer::Impl::OnAboutToFinish(GstElement*)
{
    TRACE << "Wooo about to finish\n";

    std::string url;
    {
	TRACE << "Taking lock\n";
	util::Mutex::Lock lock(m_mutex);
	if (!m_next_url.empty())
	{
	    url = m_url = m_next_url;
	    m_next_url.clear();
	}
	TRACE << "Releasing lock\n";
    }

    if (url.empty())
    {
	TRACE << "Done (stopping)\n";
	return false;
    }

    TRACE << "Calling ready\n";
    gst_element_set_state(m_play, GST_STATE_READY);
    TRACE << "Calling objectset\n";
    g_object_set(G_OBJECT(m_play), "uri", url.c_str(), NULL);
    TRACE << "Calling play\n";
    gst_element_set_state(m_play, GST_STATE_PLAYING);
    TRACE << "Calling fire\n";
    Fire(&URLObserver::OnURL, m_url);
    TRACE << "Done (continuing)\n";
    return true;
}

#if 0
static void DumpTag(const GstTagList*, const gchar *tag, gpointer)
{
    TRACE << "  " << tag << "\n";
}
#endif

gboolean URLPlayer::Impl::OnBusCallback(GstBus*, GstMessage* message)
{
    switch (GST_MESSAGE_TYPE(message)) 
    {
    case GST_MESSAGE_WARNING:
    {
	GError *err;
	gchar *debug;
	
	gst_message_parse_warning(message, &err, &debug);
	TRACE << "URLPlayer Warning: " << (const char*)err->message << "(" << (const char*)debug << ")\n";
	g_error_free(err);
	g_free(debug);
	break;
    }

    case GST_MESSAGE_ERROR:
    {
	GError *err;
	gchar *debug;
	
	gst_message_parse_error (message, &err, &debug);
	TRACE << "URLPlayer Error: " << err->message << "\n";
	g_error_free(err);
	g_free(debug);
	gst_element_set_state(m_play, GST_STATE_NULL);
	break;
    }

    case GST_MESSAGE_EOS:
	TRACE << "*** eos\n";

	if (!m_exiting) 
	{
	    bool still_playing = false;

	    if (!m_got_early_warning)
		still_playing = OnAboutToFinish(NULL);
	    
	    if (!still_playing)
	    {
		TRACE << "Calling setstate(ready)\n";
		gst_element_set_state(m_play, GST_STATE_READY);
		TRACE << "Done\n";
		m_last_state = GST_STATE_READY;
	    }
	}
	break;

    case GST_MESSAGE_STATE_CHANGED:
    {
	if (GST_MESSAGE_SRC(message) == GST_OBJECT(m_play) && !m_exiting)
	{
	    GstState newstate;
	    gst_message_parse_state_changed(message, NULL, &newstate, NULL);
	    if (newstate != m_last_state)
	    {
		TRACE << "new state " << newstate << "\n";
		switch (newstate)
		{
		case GST_STATE_PLAYING:
		    Fire(&URLObserver::OnPlayState, PLAY);
		    break;
		case GST_STATE_PAUSED:
		    Fire(&URLObserver::OnPlayState, PAUSE);
		    break;
		default:
		    Fire(&URLObserver::OnPlayState, STOP);
		    break;
		}
		m_last_state = newstate;
	    }
	}
	break;
    }

    case GST_MESSAGE_TAG:
    {
	GstTagList *tl;
	gst_message_parse_tag(message, &tl);

//	gst_tag_list_foreach(tl, &DumpTag, NULL);

	gst_tag_list_free(tl);
	break;
    }

    case GST_MESSAGE_BUFFERING:
    {
	gint percent;
	gst_message_parse_buffering(message, &percent);
//	TRACE << "buffering " << percent << "%\n";
	break;
    }

    case GST_MESSAGE_ASYNC_DONE:
	m_async_state_change = false;
	break;

    default:
	TRACE << "Got unhandled GST message "
	      << GST_MESSAGE_TYPE_NAME(message) << "\n";
	/* unhandled message */
	break;
    }

    /* we want to be notified again the next time there is a message
     * on the bus, so returning TRUE (FALSE means we want to stop watching
     * for messages on the bus and our callback should not be called again)
     */
    return true;
}

gboolean URLPlayer::Impl::StaticAlarmCallback(gpointer data)
{
    Impl *impl = (Impl*)data;
    return impl->OnAlarm();
}

gboolean URLPlayer::Impl::OnAlarm()
{
    if (!m_started)
	return true;

    GstFormat fmt = GST_FORMAT_TIME;
    gint64 ns;

    if (gst_element_query_position(m_play, &fmt, &ns))
    {
	unsigned int timecode = (unsigned int)(ns / 1000000000); // ns to s
	if (timecode != m_last_timecode_sec)
	{
//	    TRACE << "tc " << timecode << "s (" << ns << "ns)\n";
	    Fire(&URLObserver::OnTimeCode, timecode);
	    m_last_timecode_sec = timecode;
	}
    }

    return true;
}


/* URLPlayer */


URLPlayer::URLPlayer()
    : m_impl(NULL)
{
}

unsigned int URLPlayer::Init(int card, int device)
{
    assert(!m_impl);
    m_impl = new Impl(card, device);
    return 0;
}

URLPlayer::~URLPlayer()
{
    delete m_impl;
}

unsigned int URLPlayer::SetURL(const std::string& url, const std::string&)
{
    if (!m_impl)
	return EINVAL;

    return m_impl->SetURL(url);
}

unsigned int URLPlayer::SetNextURL(const std::string& url, const std::string&)
{
    if (!m_impl)
	return EINVAL;

    return m_impl->SetNextURL(url);
}

unsigned int URLPlayer::SetPlayState(PlayState st)
{
    if (!m_impl)
	return EINVAL;

    return m_impl->SetPlayState(st);
}

unsigned int URLPlayer::Seek(unsigned int ms)
{
    if (!m_impl)
	return EINVAL;

    return m_impl->Seek(ms);
}

void URLPlayer::AddObserver(URLObserver *obs)
{
    if (m_impl)
	m_impl->AddObserver(obs);
}

void URLPlayer::RemoveObserver(URLObserver *obs)
{
    if (m_impl)
	m_impl->RemoveObserver(obs);
}

} // namespace gstreamer

} // namespace output

#endif // HAVE_GSTREAMER
