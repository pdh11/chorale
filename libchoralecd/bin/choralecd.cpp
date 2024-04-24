/* choralecd.cpp
 *
 * This source code has been released by its author or authors into
 * the public domain, and all copyright is disclaimed.
 *
 * Note that *binaries* compiled from this source link against Qt, and
 * are thus subject to the licence under which you obtained Qt:
 * typically, the GPL.
 */
#include "libchoralecd/features.h"
#include <qapplication.h>
#include <qpixmap.h>
#include "libchoralecd/main_window.h"
#include "libchoralecd/settings.h"
#include "config.h"
#include "libutil/http_client.h"
#include "libutil/http_server.h"
#include "libutil/scheduler.h"
#include "libutil/trace.h"
#include "libutil/worker_thread_pool.h"
#include "libuiqt/scheduler.h"
#include "libmediadb/registry.h"
#include "libmediadb/db.h"
#include "liboutput/registry.h"
#include "libchoralecd/cd_widget.h"
#include "libchoralecd/db_widget.h"
#include "libchoralecd/output_widget.h"
#include "libimport/test_cd.h"
#include "libchoralecd/cd_window.h"
#include <memory>
#include <iostream>

#include "imagery/cddrive.xpm"
#include "imagery/empeg.xpm"
//# include "imagery/folder.xpm"
#include "imagery/network.xpm"
#include "imagery/output.xpm"

namespace choraleqt {

namespace {

int Main(int argc, char *argv[])
{
    QApplication app( argc, argv );
#ifdef WIN32
    /** The System font comes up blank in Wine for some reason, so we use this
     * font instead.
     */
    app.setFont(QFont("Verdana", 8, QFont::Normal));
#endif

    choraleqt::Settings settings;
    util::WorkerThreadPool cpu_pool(util::WorkerThreadPool::LOW);
    util::WorkerThreadPool disk_pool(util::WorkerThreadPool::NORMAL, 32);

    ui::qt::Scheduler fg_poller;
    util::BackgroundScheduler bg_poller;
    disk_pool.PushTask(util::SchedulerTask::Create(&bg_poller));

#if HAVE_CD
    import::CDDrives cds;
#endif

    std::unique_ptr<choraleqt::MainWindow> mainwin(
	new choraleqt::MainWindow(&settings, &cpu_pool, &disk_pool) );

    QPixmap cd_pixmap(cddrive_xpm);
#if HAVE_CD
    choraleqt::CDWidgetFactory cwf(&cd_pixmap, &cds, &settings, &cpu_pool,
				   &disk_pool);
    mainwin->AddWidgetFactory(&cwf);
#endif

    mediadb::Registry db_registry;
    output::Registry output_registry;

    QPixmap output_pixmap(output_xpm);

    util::http::Client http_client;
    util::http::Server http_server(&bg_poller, &disk_pool);
    http_server.Init();

    upnp::ssdp::Responder uclient(&fg_poller, NULL);
    choraleqt::UpnpOutputWidgetFactory uowf(&output_pixmap, &db_registry,
					    &output_registry,
					    &fg_poller, &http_client,
					    &http_server);
    mainwin->AddWidgetFactory(&uowf);
    uclient.Search(upnp::s_service_type_av_transport, &uowf);
    // At least one renderer (Coherence) responds to version 2 but not
    // version 1, so we must ask for both
//    uclient.Search(upnp::s_service_type_av_transport2, &uowf);

    QPixmap network_pixmap(network_xpm);
    choraleqt::ReceiverDBWidgetFactory rdbwf(&network_pixmap, &db_registry,
					     &http_client);
    mainwin->AddWidgetFactory(&rdbwf);

    receiver::ssdp::Client client;
    client.Init(&fg_poller, receiver::ssdp::s_uuid_musicserver, &rdbwf);

    choraleqt::UpnpDBWidgetFactory udbwf(&network_pixmap, &db_registry,
					 &http_client, &http_server,
					 &fg_poller);
    mainwin->AddWidgetFactory(&udbwf);
    uclient.Search(upnp::s_service_type_content_directory, &udbwf);

#if HAVE_CD
    choraleqt::UpnpCDWidgetFactory ucdwf(&cd_pixmap, &settings, &cpu_pool,
					 &disk_pool, &http_client,
					 &http_server, &fg_poller);
    mainwin->AddWidgetFactory(&ucdwf);
    uclient.Search(upnp::s_service_type_optical_drive, &ucdwf);
#endif

#if 0
    QPixmap empeg_pixmap(empeg_xpm);
    choraleqt::EmpegDBWidgetFactory edbwf(&empeg_pixmap, &db_registry,
					  &http_server);
    mainwin->AddWidgetFactory(&edbwf);
#endif

//    empeg::Discovery edisc;
//    edisc.Init(&fg_poller, &edbwf);

//    import::CDDrivePtr cdd(new import::TestCDDrive);
//    import::AudioCDPtr acd(new import::TestCD);
//    new CDWindow(cdd, acd, import::CDDBLookupPtr(), &settings, &cpu_pool,
//		 &disk_pool);

    mainwin->show();
    int rc = app.exec();

    bg_poller.Shutdown();

    return rc;
}

} // anon namespace

} // namespace choraleqt

int main(int argc, char *argv[])
{
    return choraleqt::Main(argc, argv);
}

/** @mainpage
 *
 * Chorale mainly consists of a bunch of implementations of the
 * abstract class mediadb::Database, which does what it says on the
 * tin: it represents a database of media files. There are derived
 * classes which encapsulate a collection of local files, a networked
 * UPnP A/V MediaServer, a networked Rio Receiver server, or a
 * networked Empeg music player.
 *
 * There are also a bunch of things that can use a mediadb::Database
 * once you've got one: you can serve it out again over a different
 * protocol, synchronise it with a different storage device, or queue
 * up tracks from it (on any audio device, local or remote) and listen
 * to them. You can even merge together a number of different
 * databases, and treat them as a single one. Effectively, because of
 * careful choice of abstractions, Chorale acts as a giant crossbar
 * switch between sources of media and consumers of media:
 *
 * \dot
digraph G {
  db2 -> sv2;
  db4 -> sv2;
  db3 -> sv2;
  db1 -> q;
  db2 -> q;
  db3 -> q;
  db4 -> q;
  db1 -> sv1;
  db3 -> sv1;
  db4 -> sv1;
  db1 -> sync;
  db2 -> sync;
  db3 -> sync;
  db4 -> sync;
  q -> op1;
  q -> op2;
  db1 [label="UPnP A/V client"];
  db2 [label="Receiver client"];
  db3 [label="Local files"];
  db4 [label="Empeg client"];
  q [label="Playback queue"];
  sv1 [label="Receiver server"];
  sv2 [label="UPnP A/V server"];
  sync [label="Sync to device"];
  op1 [label="Local playback"];
  op2 [label="UPnP A/V renderer"];
};
\enddot
 *
 * @section notable Notable things in the code
 *
 * @li Single SConstruct. The entire build is treated as a single graph
 * by scons, allowing it to keep all cores busy until everything is built.
 * The 700-line SConstruct replaced nearly a thousand lines of handwritten
 * configure.ac, and over 2,500 lines of handwritten Makefiles.
 *
 * @li Unit tests and coverage tests. The build system runs all tests for
 * each library before constructing the file ".a" file -- meaning that
 * library users know that, by construction, if they link at all then they
 * have a library that passed all its tests.
 *
 * @li Autogenerated UPnP APIs. A UPnP XML service description (an
 * SCPD) is very much like an IDL. So, rather than manually write XML
 * building or parsing, all the boilerplate code, for both client and
 * server, is generated directly from the XML file (straight from
 * upnp.org) using XSLT. Client code (or unit tests) accesses the same
 * API whether it's being run against a local "loopback"
 * implementation, or over the wire. For each SCPD, say
 * AVTransport2.xml, we generate: a base class upnp::AVTransport2 (and
 * a stubbed-out implementation, all of whose member functions return
 * ENOSYS); a client class upnp::AVTransport2Client, which packages up
 * the arguments and makes a SOAP call; and a server class
 * upnp::AVTransport2Server, which unpackages a SOAP request and calls
 * the actual implementation of the AVTransport service (which is
 * itself derived from upnp::AVTransport2).
 *
 * @li DSEL for specifying XML parsers. Too cunning to describe here;
 *     see @ref xml.
 *
 * @li Inter-library dependencies. Coupling between modules is a
 * source of undesirable complexity in software systems. One tool that
 * is used in Chorale to keep an eye on that, is an @e
 * automatically-generated graph of library dependencies (in
 * obj/metrics). It represents the concrete architecture of the
 * system, to be compared with the abstract architecture in the above
 * graph.
 */
