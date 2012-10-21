#include "config.h"
#include "gstreamer.h"

#ifdef HAVE_GSTREAMER

#include <gst/gst.h>
#include "libutil/trace.h"
#include "libutil/observable.h"
#include <boost/thread/thread.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

namespace output {

namespace gstreamer {

class URLPlayer::Impl: public util::Observable<URLObserver>
{
    GMainLoop *m_loop;
    GstElement *m_play;
    GstBus *m_bus;

    boost::mutex m_mutex;
    boost::condition m_cstarted;
    std::string m_url;
    std::string m_next_url;
    GstState m_last_state;
    unsigned int m_last_timecode_sec;
    bool m_async_state_change;

    volatile bool m_started;
    volatile bool m_exiting;

    boost::thread m_thread;

    void Run();

    static gboolean StaticBusCallback(GstBus*, GstMessage*, gpointer);
    gboolean OnBusCallback(GstBus*, GstMessage*);

    static gboolean StaticAlarmCallback(gpointer);
    gboolean OnAlarm();

public:
    Impl();
    ~Impl();

    unsigned int SetURL(const std::string&);
    unsigned int SetNextURL(const std::string&);
    unsigned int SetPlayState(output::PlayState);
};

URLPlayer::Impl::Impl()
    : m_loop(NULL),
      m_play(NULL),
      m_bus(NULL),
      m_last_state(GST_STATE_NULL),
      m_last_timecode_sec(UINT_MAX),
      m_async_state_change(false),
      m_started(false),
      m_exiting(false),
      m_thread(boost::bind(&Impl::Run, this))
{
}

URLPlayer::Impl::~Impl()
{
    m_exiting = true;
    TRACE << "URLPlayer::~Impl\n";
    gst_element_set_state(m_play, GST_STATE_NULL);
    if (m_loop)
	g_main_loop_quit(m_loop);
    TRACE << "URLPlayer::~Impl joining\n";
    m_thread.join();
    TRACE << "URLPlayer::~Impl joined\n";
}

void URLPlayer::Impl::Run()
{
    TRACE << "starting\n";

    gst_init(NULL, NULL);

    GMainContext *context = g_main_context_new();
    GSource *timersource = g_timeout_source_new(500);
    g_source_set_callback(timersource, &StaticAlarmCallback, this, NULL);
    g_source_attach(timersource, context);

    m_loop = g_main_loop_new(context, false);

    /* For some reason, gstreamer deadlocks loading the gnomevfs
     * element if we call set_element_state on playbin from the wrong
     * thread. (With stock gstreamer-0.10.15, g-p-b 0.10.15, g-p-g
     * 0.10.6, g-p-u 0.10.6, and even with the Ubuntu 7.10 versions.)
     *
     * This problem goes away if we force-load the gnomevfs element first.
     */    
    GstElement *gnomevfs = gst_element_factory_make("gnomevfssrc", "temp");
    gst_object_unref(GST_OBJECT(gnomevfs));
    
    m_play = gst_element_factory_make("playbin", "play");
    if (!m_play)
    {
	TRACE << "Oh-oh, no playbin\n";
    }

    gst_element_set_state(m_play, GST_STATE_READY);

    m_bus = gst_pipeline_get_bus(GST_PIPELINE(m_play));
//    TRACE << "Apparently bus=" << m_bus << " (this=" << this << ")\n";
    GSource *src = gst_bus_create_watch(m_bus);

    // Hurrah for filthy GLib non-typesafety
    g_source_set_callback(src, (GSourceFunc)&StaticBusCallback, this, NULL);
    g_source_attach(src, context);

    TRACE << "running\n";

    {
	boost::mutex::scoped_lock lock(m_mutex);
	m_started = true;
	m_cstarted.notify_all();
    }

    /* now run */
    g_main_loop_run(m_loop);

    TRACE << "ran, exiting\n";
    
    /* also clean up */
    gst_element_set_state(m_play, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(m_bus));
    gst_object_unref(GST_OBJECT(m_play));
    g_main_loop_unref(m_loop);
    m_loop = NULL;
    g_source_destroy(timersource);
    g_source_destroy(src);
    g_main_context_unref(context);
    gst_deinit();

    TRACE << "now falling off thread\n";
}

unsigned int URLPlayer::Impl::SetURL(const std::string& url)
{
    {
	boost::mutex::scoped_lock lock(m_mutex);
	m_url = url.c_str(); // Deep copy for thread-safety
	m_next_url.clear();

	if (!m_started)
	{
	    TRACE << "Waiting for start\n";
	    m_cstarted.wait(lock);
	}
    }
//    TRACE << "Calling g_object_set(" << m_url << ") on " << m_play << "\n";
    g_object_set(G_OBJECT(m_play), "uri", m_url.c_str(), NULL);
//    TRACE << "g_object_set returned\n";
    Fire(&URLObserver::OnURL, m_url);

    return 0;
}

unsigned int URLPlayer::Impl::SetNextURL(const std::string& url)
{
    boost::mutex::scoped_lock lock(m_mutex);
    m_next_url = url.c_str(); // Deep copy for thread-safety
    return 0;
}

unsigned int URLPlayer::Impl::SetPlayState(output::PlayState state)
{
    {
	boost::mutex::scoped_lock lock(m_mutex);
	if (!m_started)
	{
	    TRACE << "Waiting for start again\n";
	    m_cstarted.wait(lock);
	}
    }
    switch (state)
    {
    case PLAY:
    {
//	TRACE << "Calling set_state\n";
	GstStateChangeReturn sr = gst_element_set_state(m_play, 
							GST_STATE_PLAYING);
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

gboolean URLPlayer::Impl::StaticBusCallback(GstBus *bus, GstMessage *message,
					    gpointer data)
{
//    TRACE << "sbc(" << bus << "," << message << "," << data << ")\n";
    Impl *impl = (Impl*)data;
    return impl->OnBusCallback(bus, message);
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
    case GST_MESSAGE_ERROR:
    {
	GError *err;
	gchar *debug;
	
	gst_message_parse_error (message, &err, &debug);
	TRACE << "URLPlayer Error: " << err->message << "\n";
	g_error_free (err);
	g_free (debug);
	gst_element_set_state(m_play, GST_STATE_NULL);
	break;
    }

    case GST_MESSAGE_EOS:
	TRACE << "*** eos\n";

	if (!m_exiting)
	{
	    std::string url;
	    {
//		TRACE << "Taking lock\n";
		boost::mutex::scoped_lock lock(m_mutex);
		if (!m_next_url.empty())
		{
		    url = m_url = m_next_url;
		    m_next_url.clear();
		}
//		TRACE << "Releasing lock\n";
	    }

	    if (!url.empty())
	    {
//		TRACE << "Calling setstate\n";
		gst_element_set_state(m_play, GST_STATE_READY);
//		TRACE << "Calling objectset\n";
		g_object_set(G_OBJECT(m_play), "uri", url.c_str(), NULL);
//		TRACE << "Calling fire\n";
		Fire(&URLObserver::OnURL, m_url);
//		TRACE << "Calling setps\n";
		SetPlayState(PLAY);
	    }
	    else
	    {
		gst_element_set_state(m_play, GST_STATE_READY);
		Fire(&URLObserver::OnPlayState, STOP);
		m_last_state = GST_STATE_READY;
	    }
//	    TRACE << "Done\n";
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
    GstFormat fmt = GST_FORMAT_TIME;
    gint64 ns;

    if (gst_element_query_position(m_play, &fmt, &ns))
    {
	unsigned int timecode = ns / 1000000000; // ns to s
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
{
    if (!g_thread_supported())
    {
	TRACE << "Initing g threads\n";
	g_thread_init(NULL);
    }
    else
	TRACE << "no need\n";

    m_impl = new Impl;
}

URLPlayer::~URLPlayer()
{
    delete m_impl;
}

unsigned int URLPlayer::SetURL(const std::string& url, const std::string&)
{
    return m_impl->SetURL(url);
}

unsigned int URLPlayer::SetNextURL(const std::string& url, const std::string&)
{
    return m_impl->SetNextURL(url);
}

unsigned int URLPlayer::SetPlayState(PlayState st)
{
    return m_impl->SetPlayState(st);
}

void URLPlayer::AddObserver(URLObserver *obs)
{
    m_impl->AddObserver(obs);
}

void URLPlayer::RemoveObserver(URLObserver *obs)
{
    m_impl->RemoveObserver(obs);
}

} // namespace gstreamer

} // namespace output

#endif // HAVE_GSTREAMER
