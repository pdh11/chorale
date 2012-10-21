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
#include "output_widget.h"
#include <qlayout.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qlabel.h>
#include "setlist_window.h"
#include "tagtable.h"
#include "libutil/ssdp.h"
#include "libutil/trace.h"
#include "liboutput/upnp.h"

namespace choraleqt {

OutputWidget::OutputWidget(QWidget *parent, const std::string &host,
			   QPixmap pm, output::Queue *queue,
			   mediadb::Registry *registry)
    : ResourceWidget(parent, host, pm, "Set-list", "Player"),
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


OutputWidgetFactory::OutputWidgetFactory(QPixmap *pixmap, output::Queue *queue,
					 mediadb::Registry *registry)
    : m_pixmap(pixmap),
      m_queue(queue),
      m_registry(registry)
{
}

void OutputWidgetFactory::CreateWidgets(QWidget *parent)
{
    (void) new OutputWidget(parent, "localhost", *m_pixmap, m_queue, 
			    m_registry);
}


        /* UpnpOutputWidgetFactory */


#ifdef HAVE_LIBOUTPUT_UPNP
UpnpOutputWidgetFactory::UpnpOutputWidgetFactory(QPixmap *pixmap,
						 mediadb::Registry *registry)
    : m_pixmap(pixmap),
      m_registry(registry)
{
}

void UpnpOutputWidgetFactory::CreateWidgets(QWidget *parent)
{
    m_parent = parent;
}

void UpnpOutputWidgetFactory::OnService(const std::string& url)
{
    output::upnpav::URLPlayer *player = new output::upnpav::URLPlayer;
    player->Init(url);

    output::Queue *queue = new output::Queue(player);
    queue->SetName(player->GetFriendlyName());

    (void) new OutputWidget(m_parent, player->GetFriendlyName(), *m_pixmap,
			    queue, m_registry);
}
#endif

}; // namespace choraleqt
