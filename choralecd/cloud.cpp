#include "features.h"
#include "cloud_window.h"
#include "cloud_style.h"
#include "libutil/trace.h"
#include "libutil/hal.h"
#include "libutil/dbus.h"
#include "libutil/bind.h"
#include "libutil/poll.h"
#include "libutil/poll_thread.h"
#include "libutil/http_client.h"
#include "libutil/http_server.h"
#include "libmediadb/registry.h"
#include "liboutput/gstreamer.h"
#include "liboutput/upnpav.h"
#include "liboutput/queue.h"
#include "libupnp/ssdp.h"
#include "poller.h"
#include "cloud_upnp_databases.h"
#include <QtGui>

#include "imagery/network.xpm"
#include "imagery/cd.xpm"
#include "imagery/empeg.xpm"
#include "imagery/folder.xpm"
#include "imagery/output.xpm"
#include "imagery/noart100.xpm"
#include "imagery/file.xpm"

namespace cloud {

class QueueListModel: public QAbstractListModel
{
    output::Queue *m_queue;
    std::vector<std::string> m_names;

public:
    explicit QueueListModel(output::Queue *queue);

    // Being a QAbstractListModel
    int rowCount(const QModelIndex&) const;
    QVariant data(const QModelIndex&, int role) const;
};

QueueListModel::QueueListModel(output::Queue *queue)
    : m_queue(queue)
{
}

int QueueListModel::rowCount(const QModelIndex&) const
{
    return (int)(m_queue->end() - m_queue->begin());
}

QVariant QueueListModel::data(const QModelIndex& i, int) const
{
    return QVariant(QString::fromUtf8(m_names[i.row()].c_str()));
}

class OutputWidget: public QWidget
{
    QListView *m_list_view;
    QPixmap m_art;
    QueueListModel m_model;

    enum { LIST_OFFSET = 150 };

public:
    OutputWidget(Window *parent, output::Queue *queue);

    void paintEvent(QPaintEvent*);
    void resizeEvent(QResizeEvent*);
};

OutputWidget::OutputWidget(Window *parent, output::Queue *queue)
    : QWidget(parent),
      m_list_view(new QListView(this)),
      m_art(ShadeImage(parent->Foreground(), parent->Background(),
		       noart100_xpm)),
      m_model(queue)
{
    hide();
    m_list_view->setModel(&m_model);
    m_list_view->move(0, LIST_OFFSET);
}

void OutputWidget::resizeEvent(QResizeEvent *re)
{
    m_list_view->resize(re->size().width(), re->size().height() - LIST_OFFSET);
}

void OutputWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.drawPixmap(20, 20, m_art);
}

class Output
{
    Window *m_parent;
    output::URLPlayer *m_player; 
    output::Queue m_queue;
    OutputWidget *m_widget;

public:
    Output(Window *parent,
	   output::URLPlayer *player, 
	   const std::string &queue_name);
    ~Output();

    unsigned int OnSelect();
};

Output::Output(Window *parent, output::URLPlayer *player,
	       const std::string& queue_name)
    : m_parent(parent),
      m_player(player),
      m_queue(m_player),
      m_widget(new OutputWidget(parent, &m_queue))
{
    m_queue.SetName(queue_name);
}

Output::~Output()
{
    delete m_player;
}

unsigned int Output::OnSelect()
{
    m_parent->SetMainWidget(m_widget);
    return 0;
}

class LocalOutputs: public util::hal::DeviceObserver
{
    Window *m_parent;
    util::hal::Context *m_hal;
    mediadb::Registry *m_registry;
    QPixmap m_pixmap;
    std::set<Output*> m_outputs;

public:
    LocalOutputs(Window*, util::hal::Context*, mediadb::Registry*);
    ~LocalOutputs();

    // Being a util::hal::DeviceObserver
    void OnDevice(util::hal::DevicePtr dev);
};

LocalOutputs::LocalOutputs(Window *parent, util::hal::Context *hal,
			   mediadb::Registry *registry)
    : m_parent(parent),
      m_hal(hal),
      m_registry(registry),
      m_pixmap(ShadeImage(parent->Foreground(), parent->Background(),
			  output_xpm))
{
    if (m_hal)
    {
	m_hal->GetMatchingDevices("alsa.type", "playback", this);
    }
    else
    {
	Output *out = new Output(parent,
				 new output::gstreamer::URLPlayer,
				 "output");
	m_outputs.insert(out);
	MenuEntry me;
	me.pixmap = m_pixmap;
	me.text = "Playback";
	me.onselect = util::Bind<Output, &Output::OnSelect>(out);
	parent->AppendMenuEntry(me);
    }
}

LocalOutputs::~LocalOutputs()
{
    while (!m_outputs.empty())
    {
	Output *out = *m_outputs.begin();
	m_outputs.erase(m_outputs.begin());
	delete out;
    }
}

void LocalOutputs::OnDevice(util::hal::DevicePtr dev)
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

    Output *out = new Output(m_parent,
			     new output::gstreamer::URLPlayer(card, device),
			     "output");
    m_outputs.insert(out);
    MenuEntry me;
    me.pixmap = m_pixmap;
    me.text = devnode.c_str();
    me.onselect = util::Bind<Output, &Output::OnSelect>(out);
    m_parent->AppendMenuEntry(me);
}

int Main(int argc, char *argv[])
{
//    QColor fg(127,255,127);
//    QColor bg(Qt::black);

    QColor fg(6, 74, 140);
    QColor bg(Qt::white);

    QApplication app(argc, argv);
    cloud::Style *style = new cloud::Style(fg, bg);
    QApplication::setStyle(style);

    cloud::Window mainwindow(fg,bg);

    choraleqt::Poller fg_poller;
    util::PollThread bg_poller;
    util::WorkerThreadPool disk_pool(util::WorkerThreadPool::NORMAL, 32);

    util::hal::Context *halp = NULL;
#ifdef HAVE_HAL
    util::dbus::Connection dbusc(&fg_poller);
    unsigned int res = dbusc.Connect(util::dbus::Connection::SYSTEM);
    if (res)
    {
	TRACE << "Can't connect to D-Bus\n";
    }
    util::hal::Context halc(&dbusc);
    res = halc.Init();
    if (res == 0)
	halp = &halc;
#endif

    mediadb::Registry registry;

    QPixmap output_pixmap((const char**)output_xpm);
#ifdef HAVE_LIBOUTPUT
    cloud::LocalOutputs owf(&mainwindow, halp, &registry);
#endif

    util::http::Client http_client;
    util::http::Server http_server(&bg_poller, &disk_pool);
    http_server.Init();

    upnp::ssdp::Responder uclient(&fg_poller);

    QPixmap network_pixmap(ShadeImage(fg, bg, network_xpm));

    cloud::UpnpDatabases udb(&mainwindow, &network_pixmap, &uclient, &registry,
			     &http_client, &http_server);

    mainwindow.show();
    return app.exec();
}

} // namespace cloud

int main(int argc, char *argv[])
{
    return cloud::Main(argc, argv);
}
