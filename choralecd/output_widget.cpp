/* output_widget.cpp
 *
 * This source code has been released by its author or authors into
 * the public domain, and all copyright is disclaimed.
 *
 * Note that *binaries* compiled from this source link against Qt, and
 * are thus subject to the licence under which you obtained Qt:
 * typically, the GPL.
 */
#include "config.h"
#include "output_widget.h"
#include <qlayout.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qlabel.h>
#include "setlist_window.h"
#include "libutil/trace.h"
#include "libutil/bind.h"
#include "libutil/http_fetcher.h"
#include "libutil/counted_pointer.h"
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


#if HAVE_HAL
OutputWidgetFactory::OutputWidgetFactory(QPixmap *pixmap, 
					 util::hal::Context *halp,
					 mediadb::Registry *registry)
    : m_pixmap(pixmap),
      m_parent(NULL),
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
	output::gstreamer::URLPlayer *player =
	    new output::gstreamer::URLPlayer;
	player->Init();
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

    output::gstreamer::URLPlayer *player = new output::gstreamer::URLPlayer;
    player->Init(card, device);
    m_players.push_back(player);
    output::Queue *queue = new output::Queue(player);
    queue->SetName(devnode);
    (void) new OutputWidget(m_parent, devnode.c_str(), *m_pixmap, queue, 
			    m_registry, card_id);
}
#endif


        /* UpnpOutputWidget */

UpnpOutputWidget::UpnpOutputWidget(QWidget *parent, const std::string& name, 
				   QPixmap pm, 
				   output::upnpav::URLPlayer *player,
				   output::Queue *queue, 
				   mediadb::Registry *database_registry,
				   const std::string& tooltip)
    : OutputWidget(parent, name, pm, queue, database_registry, tooltip),
      m_player(player)
{
    EnableTop(false);
}

unsigned int UpnpOutputWidget::OnInitialised(unsigned int rc)
{
    if (!rc)
    {
	std::string friendly_name = m_player->GetFriendlyName();
	GetQueue()->SetName(friendly_name);
	SetLabel(friendly_name);
    }
    EnableTop(rc == 0);
    return 0;
}


        /* UpnpOutputWidgetFactory */


UpnpOutputWidgetFactory::UpnpOutputWidgetFactory(QPixmap *pixmap,
						 mediadb::Registry *dbregistry,
						 output::Registry *opregistry,
						 util::Scheduler *poller,
						 util::http::Client *client,
						 util::http::Server *server)
    : m_pixmap(pixmap),
      m_parent(NULL),
      m_db_registry(dbregistry),
      m_output_registry(opregistry),
      m_poller(poller),
      m_client(client),
      m_server(server)
{
}

void UpnpOutputWidgetFactory::CreateWidgets(QWidget *parent)
{
    m_parent = parent;
}

void UpnpOutputWidgetFactory::OnService(const std::string& url,
					const std::string& udn)
{
    std::string output_name = "upnpav:" + udn;

    output::upnpav::URLPlayer *player =
	new output::upnpav::URLPlayer(m_client, m_server, m_poller);

    output::Queue *queue = new output::Queue(player);

    // Use the hostname as a stand-in until we know the friendly-name
    std::string hostpart, pathpart;
    util::http::ParseURL(url, &hostpart, &pathpart);
    std::string host;
    unsigned short port;
    util::http::ParseHost(hostpart, 80, &host, &port);

    UpnpOutputWidget *widget = new UpnpOutputWidget(m_parent, host,
						    *m_pixmap, player,
						    queue, m_db_registry,
						    url);
    
    player->Init(url, udn,
		 util::Bind(widget).To<unsigned int,
		                       &UpnpOutputWidget::OnInitialised>());
}

} // namespace choraleqt
