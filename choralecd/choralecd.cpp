/* choralecd.cpp
 *
 * This source code has been released by its author or authors into
 * the public domain, and all copyright is disclaimed.
 *
 * Note that *binaries* compiled from this source link against Qt, and
 * are thus subject to the licence under which you obtained Qt:
 * typically, the GPL.
 */
#include "features.h"
#include <qapplication.h>
#include <qpixmap.h>
#include "main_window.h"
#include "settings.h"
#include "config.h"
#include "libutil/dbus.h"
#include "libutil/http_client.h"
#include "libutil/http_server.h"
#include "libutil/scheduler.h"
#include "libutil/trace.h"
#include "libutil/worker_thread_pool.h"
#include "libuiqt/scheduler.h"
#include "libmediadb/registry.h"
#include "libmediadb/db.h"
#include "liboutput/registry.h"
#include "cd_widget.h"
#include "db_widget.h"
#include "output_widget.h"
#include <memory>
#include <iostream>

#include "imagery/cddrive.xpm"
#include "imagery/empeg.xpm"
//# include "imagery/folder.xpm"
#include "imagery/network.xpm"
#include "imagery/output.xpm"

namespace choraleqt {

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

    util::hal::Context *halp = NULL;
#if HAVE_HAL
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

#if HAVE_CD
    import::CDDrives cds(halp);
#endif

    std::auto_ptr<choraleqt::MainWindow> mainwin(
	new choraleqt::MainWindow(&settings, &cpu_pool, &disk_pool) );

    QPixmap cd_pixmap((const char**)cddrive_xpm);
#if HAVE_CD
    choraleqt::CDWidgetFactory cwf(&cd_pixmap, &cds, &settings, &cpu_pool,
				   &disk_pool);
    mainwin->AddWidgetFactory(&cwf);
#endif

    mediadb::Registry db_registry;
    output::Registry output_registry;

    QPixmap output_pixmap((const char**)output_xpm);
#if HAVE_LIBOUTPUT
    choraleqt::OutputWidgetFactory owf(&output_pixmap, halp, &db_registry);
    mainwin->AddWidgetFactory(&owf);
#endif

    util::http::Client http_client;
    util::http::Server http_server(&bg_poller, &disk_pool);
    http_server.Init();

    upnp::ssdp::Responder uclient(&fg_poller, NULL);
    choraleqt::UpnpOutputWidgetFactory uowf(&output_pixmap, &db_registry,
					    &output_registry,
					    &fg_poller, &http_client,
					    &http_server);
    mainwin->AddWidgetFactory(&uowf);
//    TRACE << "Calling uc.init\n";
    uclient.Search(upnp::s_service_type_av_transport, &uowf);
//    TRACE << "uc.init done\n";

    QPixmap network_pixmap((const char**)network_xpm);
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

    QPixmap empeg_pixmap((const char**)empeg_xpm);
    choraleqt::EmpegDBWidgetFactory edbwf(&empeg_pixmap, &db_registry,
					  &http_server);
    mainwin->AddWidgetFactory(&edbwf);

//    empeg::Discovery edisc;
//    edisc.Init(&fg_poller, &edbwf);

    mainwin->show();
    int rc = app.exec();

    bg_poller.Shutdown();

    return rc;
}

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
 * automatically-generated graph of library dependencies (<tt>make
 * libdeps.png</tt> at the top level). It represents the concrete
 * architecture of the system, to be compared with the abstract
 * architecture in the above graph.
 *
 * @li Porting and Portability. Portable code can end up being a maze
 * of <tt>ifdef</tt>'s; the aim in Chorale has been to reduce uses of
 * <tt>ifdef</tt> to @e either just one or two lines of code, @e or
 * entire files (so that either way it's easy for the reader to
 * determine what's going on). For instance, in libutil/file.h, the
 * APIs with two implementations are kept in two different C++
 * namespaces, util::posix and util::win32, one of which is
 * namespace-aliased to util::fileapi, with using-declarations to draw
 * them from util::fileapi into the common util namespace where
 * everyone uses them. That way, @e only the one-line namespace-alias
 * need be inside <tt>ifdef</tt>. (An earlier design had "using
 * namespace win32", but the present design (a) ensures that there's a
 * single place with a complete list of all the portable APIs, and (b)
 * discourages writers of portable code from using the non-portable
 * APIs in namespaces win32 and posix.)
 *
 * @li Optimisation and -fwhole-program. When configured with
 * --enable-final, the entire Chorale daemon is compiled as a single
 * C++ translation unit (by @e automatically figuring out which
 * Chorale library objects are used, and then #include'ing all their
 * sources instead).  This reduces the size of the final binary by
 * about 20% on both ia32 and amd64, at the expense of vast memory
 * usage during compilation.
 *
 * @li Unicode is the One True God, and UTF-8 is His prophet. Native
 * Win32 applications use UTF-16 to interface to the filesystem
 * (there's an alternative non-Unicode "ANSI" API, but that leaves
 * some files with Unicode names inaccessible). In order to avoid
 * having two parallel implementations of large swathes of
 * functionality, Chorale includes a translation layer that lets all
 * the rest of the code assume that filenames are always UTF-8
 * everywhere.
 */
