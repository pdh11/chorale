/* choralecd.cpp
 *
 * This source code has been released by its author or authors into
 * the public domain, and all copyright is disclaimed.
 *
 * Note that *binaries* compiled from this source link against Qt, and
 * are thus subject to the licence under which you obtained Qt:
 * typically, the GPL.
 */
#include "config.h"
#include <qapplication.h>
#include <qpixmap.h>
#include "main_window.h"
#include "settings.h"
#include "libimport/cd_drives.h"
#include "libutil/worker_thread_pool.h"
#include "poller.h"
#include "libutil/trace.h"
#include "cd_widget.h"
#include "db_widget.h"
#include "output_widget.h"
#include "libdbreceiver/db.h"
#include "libreceiver/ssdp.h"
#include "libutil/cpus.h"
#include "liboutput/queue.h"
#include "liboutput/urlplayer.h"
#include "libmediadb/registry.h"

#include "cd.xpm"
#include "folder.xpm"
#include "network.xpm"
#include "output.xpm"

#if defined(HAVE_CURL)
#define HAVE_LIBDBRECEIVER 1
#endif
#if defined(HAVE_UPNP) && defined(HAVE_LIBUUID) && defined(HAVE_OPENSSL)
#define HAVE_LIBDBUPNP 1
#endif
#if defined(HAVE_UPNP)
#define HAVE_LIBOUTPUT_UPNP 1
#endif
#if defined(HAVE_GSTREAMER)
#define HAVE_LIBOUTPUT 1
#endif

int main(int argc, char *argv[])
{
    QApplication app( argc, argv );

    // Start this off really early, as it fork()s internally
    output::GSTPlayer player;

    import::CDDrives cds;
    Settings settings;
    util::WorkerThreadPool cpu_pool;
    util::WorkerThreadPool disk_pool(util::CountCPUs()*2);
    util::TaskQueue *cpu_queue = cpu_pool.GetTaskQueue();
    util::TaskQueue *disk_queue = disk_pool.GetTaskQueue();

    std::auto_ptr<choraleqt::MainWindow> mainwin(
	new choraleqt::MainWindow(&settings, cpu_queue, disk_queue) );

    app.setMainWidget(mainwin.get());

//    QPixmap folder_pixmap((const char**)folder_xpm);
//    choraleqt::LocalDBWidgetFactory ldbwf(&folder_pixmap, &settings);
//    mainwin->AddWidgetFactory(&ldbwf);

    QPixmap cd_pixmap((const char**)cd_xpm);
    choraleqt::CDWidgetFactory cwf(&cd_pixmap, &cds, &settings, cpu_queue,
				   disk_queue);
    mainwin->AddWidgetFactory(&cwf);

    mediadb::Registry registry;

#ifdef HAVE_LIBOUTPUT
    output::Queue queue(&player);
    QPixmap output_pixmap((const char**)output_xpm);
    choraleqt::OutputWidgetFactory owf(&output_pixmap, &queue, &registry);
    mainwin->AddWidgetFactory(&owf);
#endif

    choraleqt::Poller poller;

#ifdef HAVE_LIBOUTPUT_UPNP
    util::ssdp::Client uclient(&poller);
    choraleqt::UpnpOutputWidgetFactory uowf(&output_pixmap, &registry);
    mainwin->AddWidgetFactory(&uowf);
    TRACE << "Calling uc.init\n";
    uclient.Init(util::ssdp::s_uuid_avtransport, &uowf);
    TRACE << "uc.init done\n";
#endif

    QPixmap network_pixmap((const char**)network_xpm);
#ifdef HAVE_LIBDBRECEIVER
    choraleqt::ReceiverDBWidgetFactory rdbwf(&network_pixmap, &registry);
    mainwin->AddWidgetFactory(&rdbwf);

    receiver::ssdp::Client client;
    client.Init(&poller, receiver::ssdp::s_uuid_musicserver, &rdbwf);
#endif

#ifdef HAVE_LIBDBUPNP
    choraleqt::UpnpDBWidgetFactory udbwf(&network_pixmap, &registry);
    mainwin->AddWidgetFactory(&udbwf);
    uclient.Init(util::ssdp::s_uuid_contentdirectory, &udbwf);
#endif

    mainwin->show();
    int rc = app.exec();

    return rc;
}

/** @mainpage
 *
 * Chorale mainly consists of a bunch of implementations of the
 * abstract class mediadb::Database, which does what it says on the
 * tin: it represents a database of media files. There are derived
 * classes which encapsulate a collection of local files, a remote
 * UPnP A/V MediaServer, or a remote Rio Receiver server.
 *
 * There are also a bunch of things that can use a mediadb::Database
 * once you've got one: you can serve it out again over a different
 * protocol, or queue up tracks from it (on any audio device, local or
 * remote) and listen to them. Effectively, because of careful choice
 * of abstractions, Chorale acts a giant crossbar switch between
 * sources of media and consumers of media:
 *
 * \dot
digraph G {
  db2 -> sv2 [color=gray];
  db3 -> sv2 [color=gray];
  db1 -> q;
  db2 -> q;
  db3 -> q;
  db1 -> sv1;
  db3 -> sv1;
  q -> op1;
  q -> op2;
  db1 [label="UPnP A/V client"];
  db2 [label="Receiver client"];
  db3 [label="Local files"];
  q [label="Playback queue"];
  sv1 [label="Receiver server"];
  sv2 [label="UPnP A/V server (NYI)", color=gray, fontcolor=gray];
  op1 [label="Local playback"];
  op2 [label="UPnP A/V renderer"];
};
\enddot
 *
 * @section notable Notable things in the code
 *
 * I'd like to draw your attention to:
 *
 * @li Non-recursive make. There's a famous white-paper, "Recursive @p
 * make Considered Harmful", which did a good job of convincing people
 * that traditional automake-style Makefiles were a bad idea, but was
 * short on detail of what to do instead. Chorale uses autoconf but
 * not automake; the makefiles all include each other rather than
 * recursively invoking each other. This has the effect that one
 * single @p make invocation gets to see the entire dependency graph
 * of the system, and so can make better decisions. For instance,
 * using the @p -j option to @p make keeps multi-core or multi-CPU
 * machines much more effectively-used, as @p make can start new jobs
 * from any part of the project at any time.
 *
 * @li Unit tests and coverage tests. The @p tests Makefile target runs
 * all the unit tests relevant to the directory where it's invoked (for
 * the top-level directory, that's @e all unit tests). Code coverage of
 * the unit tests is also displayed each time. The unit tests have proper
 * dependencies, and are only re-tested when something has changed.
 *
 * @li Missing configure dependencies. The configure script checks for
 * @e all required libraries, then prints a list of @e all the ones it
 * can't find. This is so much less annoying than the standard
 * practice of bailing out as soon as one is missing; it saves having
 * to do several rounds of re-configuring.
 *
 * @li Autogenerated UPnP APIs. A UPnP XML service description (an
 * SCPD) is very much like an IDL. So, rather than manually write XML
 * building or parsing, all the boilerplate code, for both client and
 * server, is generated directly from the XML file (straight from
 * upnp.org) using XSLT.
 *
 * @li Inter-library dependencies. Coupling between modules is a
 * source of undesirable complexity in software systems. One tool that
 * is used in Chorale to keep an eye on that, is an @e
 * automatically-generated graph of library dependencies (<tt>make
 * libdeps.png</tt> at the top level). It represents the concrete
 * architecture of the system, to be compared with the abstract
 * architecture in the above graph.
 */
