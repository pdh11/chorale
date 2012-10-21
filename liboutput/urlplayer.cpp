#include "urlplayer.h"
#include <gst/gst.h>
#include "libutil/trace.h"
#include "libutil/observable.h"
#include <boost/thread/thread.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

namespace output {

class GSTPlayer::Impl: public util::Observable<URLObserver>
{
    GMainLoop *m_loop;
    GstElement *m_play;

    boost::mutex m_mutex;
    std::string m_url;
    std::string m_next_url;
    GstState m_last_state;
    unsigned int m_last_timecode_sec;
    bool m_async_state_change;

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

GSTPlayer::Impl::Impl()
    : m_last_state(GST_STATE_NULL),
      m_last_timecode_sec(UINT_MAX),
      m_async_state_change(false),
      m_exiting(false),
      m_thread(boost::bind(&Impl::Run, this))
{
}

GSTPlayer::Impl::~Impl()
{
    m_exiting = true;
    TRACE << "GSTPlayer::~Impl\n";
    gst_element_set_state(m_play, GST_STATE_NULL);
    if (m_loop)
	g_main_loop_quit(m_loop);
    TRACE << "GSTPlayer::~Impl joining\n";
    m_thread.join();
    TRACE << "GSTPlayer::~Impl joined\n";
}

void GSTPlayer::Impl::Run()
{
    TRACE << "starting\n";

    int argc = 0;
    char **argv = { NULL };
    gst_init(&argc, &argv);

    m_loop = g_main_loop_new(NULL, false);
    
    m_play = gst_element_factory_make("playbin", "play");

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(m_play));
    gst_bus_add_watch(bus, &StaticBusCallback, this);
    gst_object_unref(bus);

    gst_element_set_state(m_play, GST_STATE_READY);

    TRACE << "running\n";

    /* now run */
    g_timeout_add(500, &StaticAlarmCallback, this);
    g_main_loop_run(m_loop);

    TRACE << "ran, exiting\n";
    
    /* also clean up */
    gst_element_set_state(m_play, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(m_play));
    g_main_loop_unref(m_loop);
    m_loop = NULL;
    gst_deinit();

    TRACE << "now falling off thread\n";
}

unsigned int GSTPlayer::Impl::SetURL(const std::string& url)
{
    {
	boost::mutex::scoped_lock lock(m_mutex);
	m_url = url.c_str(); // Deep copy for thread-safety
	m_next_url.clear();
    }
    g_object_set(G_OBJECT(m_play), "uri", m_url.c_str(), NULL);

    return 0;
}

unsigned int GSTPlayer::Impl::SetNextURL(const std::string& url)
{
    boost::mutex::scoped_lock lock(m_mutex);
    m_next_url = url.c_str(); // Deep copy for thread-safety
    return 0;
}

unsigned int GSTPlayer::Impl::SetPlayState(output::PlayState state)
{
    switch (state)
    {
    case PLAY:
    {
	GstStateChangeReturn sr = gst_element_set_state(m_play, 
							GST_STATE_PLAYING);
	TRACE << "set_state returned " << sr << "\n";
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

gboolean GSTPlayer::Impl::StaticBusCallback(GstBus *bus, GstMessage *message,
					    gpointer data)
{
    Impl *impl = (Impl*)data;
    return impl->OnBusCallback(bus, message);
}

#if 0
static void DumpTag(const GstTagList*, const gchar *tag, gpointer)
{
    TRACE << "  " << tag << "\n";
}
#endif

gboolean GSTPlayer::Impl::OnBusCallback(GstBus*, GstMessage *message)
{
    switch (GST_MESSAGE_TYPE(message)) 
    {
    case GST_MESSAGE_ERROR:
    {
	GError *err;
	gchar *debug;
	
	gst_message_parse_error (message, &err, &debug);
	TRACE << "GSTPlayer Error: " << err->message << "\n";
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
		boost::mutex::scoped_lock lock(m_mutex);
		if (!m_next_url.empty())
		{
		    url = m_url = m_next_url;
		    m_next_url.clear();
		}
	    }

	    if (!url.empty())
	    {
		gst_element_set_state(m_play, GST_STATE_READY);
		g_object_set(G_OBJECT(m_play), "uri", url.c_str(), NULL);
		Fire(&URLObserver::OnURL, m_url);
		SetPlayState(PLAY);
	    }
	    else
	    {
		gst_element_set_state(m_play, GST_STATE_READY);
		Fire(&URLObserver::OnPlayState, STOP);
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

gboolean GSTPlayer::Impl::StaticAlarmCallback(gpointer data)
{
    Impl *impl = (Impl*)data;
    return impl->OnAlarm();
}

gboolean GSTPlayer::Impl::OnAlarm()
{
    /* For some reason, gstreamer deadlocks if the FIRST time we try and play,
     * we're on the wrong thread. (With gstreamer-0.10.15, g-p-b 0.10.15,
     * g-p-g 0.10.6, g-p-u 0.10.6.)
     *
     * This problem goes away if we try and play a fake file first on the
     * GStreamer thread.
     */
    static bool doneit = false;
    if (!doneit)
    {
	doneit = true;
	SetURL("http://127.0.0.1/fake.mp3");

	GstStateChangeReturn sr = gst_element_set_state(m_play, GST_STATE_PAUSED);
	TRACE << "set_state1 returned " << sr << "\n";
    }

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


/* GSTPlayer */


GSTPlayer::GSTPlayer()
    : m_impl(new Impl)
{
}

GSTPlayer::~GSTPlayer()
{
    delete m_impl;
}

unsigned int GSTPlayer::SetURL(const std::string& url, const std::string&)
{
    return m_impl->SetURL(url);
}

unsigned int GSTPlayer::SetNextURL(const std::string& url, const std::string&)
{
    return m_impl->SetNextURL(url);
}

unsigned int GSTPlayer::SetPlayState(PlayState st)
{
    return m_impl->SetPlayState(st);
}

void GSTPlayer::AddObserver(URLObserver *obs)
{
    m_impl->AddObserver(obs);
}

void GSTPlayer::RemoveObserver(URLObserver *obs)
{
    m_impl->RemoveObserver(obs);
}

}; // namespace output
