/* output_widget.cpp
 *
 * This source code has been released by its author or authors into
 * the public domain, and all copyright is disclaimed.
 *
 * Note that *binaries* compiled from this source link against Qt, and
 * are thus subject to the licence under which you obtained Qt:
 * typically, the GPL.
 */
#include "features.h"
#include "config.h"
#include "output_widget.h"
#include <qlayout.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qlabel.h>
#include "setlist_window.h"
#include "tagtable.h"
#include "libutil/hal.h"
#include "libutil/ssdp.h"
#include "libutil/trace.h"
#include "liboutput/gstreamer.h"
#include "liboutput/upnpav.h"
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/construct.hpp>
#include <algorithm>

namespace choraleqt {

OutputWidget::OutputWidget(QWidget *parent, const std::string &host,
			   QPixmap pm, output::Queue *queue,
			   mediadb::Registry *registry, 
			   const std::string& tooltip)
    : ResourceWidget(parent, host, pm, "Set-list", "Player", tooltip),
      m_queue(queue),
      m_registry(registry),
      m_setlist(NULL)
{
    EnableBottom(false);
}

OutputWidget::~OutputWidget()
{
    delete m_setlist;
}

void OutputWidget::OnTopButton()
{
    if (!m_setlist)
	m_setlist = new SetlistWindow(m_queue, m_registry);
    m_setlist->raise();
    m_setlist->show();
}

void OutputWidget::OnBottomButton()
{
}


        /* OutputWidgetFactory */


OutputWidgetFactory::OutputWidgetFactory(QPixmap *pixmap, 
					 util::hal::Context *halp,
					 mediadb::Registry *registry)
    : m_pixmap(pixmap),
      m_hal(halp),
      m_registry(registry)
{
}

OutputWidgetFactory::~OutputWidgetFactory()
{
    std::for_each(m_players.begin(), m_players.end(), 
		  boost::lambda::delete_ptr());
}

void OutputWidgetFactory::CreateWidgets(QWidget *parent)
{
    if (m_hal)
    {
	m_parent = parent;
	m_hal->GetMatchingDevices("alsa.type", "playback", this);
    }
    else
    {
	output::URLPlayer *player = new output::gstreamer::URLPlayer;
	m_players.push_back(player);
	output::Queue *queue = new output::Queue(player);
	queue->SetName("output");
	(void) new OutputWidget(parent, "localhost", *m_pixmap, queue, 
				m_registry, std::string());
    }
}

void OutputWidgetFactory::OnDevice(util::hal::DevicePtr dev)
{
    unsigned card = dev->GetInt("alsa.card");
    unsigned device = dev->GetInt("alsa.device");
    std::string devnode = dev->GetString("alsa.device_file");
    std::string card_id = dev->GetString("alsa.card_id");
    std::string device_id = dev->GetString("alsa.device_id");
    if (!device_id.empty())
    {
	if (card_id.empty())
	    card_id = device_id;
	else
	    card_id = device_id + " on " + card_id;
    }

    output::URLPlayer *player = new output::gstreamer::URLPlayer(card, device);
    m_players.push_back(player);
    output::Queue *queue = new output::Queue(player);
    queue->SetName(devnode);
    (void) new OutputWidget(m_parent, devnode.c_str(), *m_pixmap, queue, 
			    m_registry, card_id);
}


        /* UpnpOutputWidgetFactory */


#ifdef HAVE_LIBOUTPUT_UPNP
UpnpOutputWidgetFactory::UpnpOutputWidgetFactory(QPixmap *pixmap,
						 mediadb::Registry *registry,
						 util::PollerInterface *poller)
    : m_pixmap(pixmap),
      m_registry(registry),
      m_poller(poller)
{
}

void UpnpOutputWidgetFactory::CreateWidgets(QWidget *parent)
{
    m_parent = parent;
}

void UpnpOutputWidgetFactory::OnService(const std::string& url,
					const std::string& udn)
{
    output::upnpav::URLPlayer *player = new output::upnpav::URLPlayer;
    player->Init(url, udn, m_poller);

    std::string fn = player->GetFriendlyName();
    TRACE << "fn '" << fn << "'\n";

    output::Queue *queue = new output::Queue(player);
    queue->SetName(fn);

    (void) new OutputWidget(m_parent, fn, *m_pixmap, queue, m_registry,
			    std::string());
}
#endif

} // namespace choraleqt
