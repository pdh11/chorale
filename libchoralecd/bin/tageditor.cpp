#include <QApplication>
#include <QProgressDialog>
#include "libdbsteam/db.h"
#include "libdblocal/db.h"
#include "libdblocal/file_scanner.h"
#include "libutil/worker_thread_pool.h"
#include "libutil/http_client.h"
#include "libutil/errors.h"
#include "libmediadb/schema.h"
#include "libmediadb/registry.h"
#include "libchoralecd/tag_editor_widget.h"
#include "libchoralecd/explorer_window.h"
#include "libchoralecd/events.h"
#include <getopt.h>
#include <stdio.h>

#if HAVE_TAGLIB
#define LICENCE "  This program is free software under the GNU Lesser General Public Licence.\n\n"
#else
#define LICENCE "  This program is placed in the public domain by its authors.\n\n"
#endif

namespace tageditor {

namespace {

class ScanProgressDialog: public QProgressDialog, 
		    public db::local::FileScanner::Observer
{
    mediadb::Database *m_db;
    mediadb::Registry *m_registry;

public:
    ScanProgressDialog(mediadb::Database *db, mediadb::Registry *registry)
	: QProgressDialog("Scanning files...", QString(), 0, 0),
	  m_db(db),
	  m_registry(registry)
    {
	setWindowTitle("tageditor Scanning files...");
    }

    void customEvent(QEvent *ce)
    {
	if ((int)ce->type() == choraleqt::StringEvent::EventType())
	{
	    choraleqt::StringEvent *se = (choraleqt::StringEvent*)ce;
	    if (!se->m_data.empty())
	    {
		setLabelText(QString::fromUtf8(se->m_data.c_str()));
	    }
	    else
	    {
		// Finished!

		choraleqt::ExplorerWindow *ew =
		    new choraleqt::ExplorerWindow(m_db,
						  m_registry,
						  choraleqt::TagEditorWidget::SHOW_PATH);
		ew->SetTagMode();
		ew->show();
		hide();
	    }
	}
    }

    // Being a db::local::FileScanner::Observer
    unsigned int OnFile(const std::string& s)
    {
	// Called on wrong thread
	QApplication::postEvent(this, new choraleqt::StringEvent(s));
	if (wasCanceled())
	    return ECANCELED;
	return 0;
    }

    void OnFinished(unsigned int error)
    {
	// Called on wrong thread
	if (error)
	{
	    fprintf(stderr, "Scanning failed: %u\n", error);
	    exit(1);
	}

	QApplication::postEvent(this, new choraleqt::StringEvent(""));
    }
};

static void Usage(FILE *f)
{
    fprintf(f,

"Usage: tageditor [options] <root-directory> [<flac-root-directory>]\n"
"Media metadata (tag) editing.\n"
"\n"
"The options are:\n"
" -t, --threads=N    Use max N threads to scan/rewrite files (default 32)\n"
" -h, --help         These hastily-scratched notes\n"
	    "\n"
	    LICENCE
"From " PACKAGE_STRING " (" PACKAGE_WEBSITE ") built on " __DATE__ ".\n"
	);
}

int Main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    static const struct option options[] =
    {
	{ "help", no_argument, NULL, 'h' },
	{ "threads", required_argument, NULL, 't' },
	{ NULL, 0, NULL, 0 }
    };

    unsigned int nthreads = 32;

    int option_index;
    int option;
    while ((option = getopt_long(argc, argv, "ht:", options, &option_index))
	   != -1)
    {
	switch (option)
	{
	case 'h':
	    Usage(stdout);
	    return 0;
	case 't':
	    nthreads = (unsigned int)strtoul(optarg, NULL, 10);
	    break;
	default:
	    Usage(stderr);
	    return 1;
	}
    }

    const char *media_root;
    const char *flac_root = "";

    switch ((int)(argc-optind)) // How many more arguments?
    {
    case 2:
	media_root = argv[optind];
	flac_root  = argv[optind+1];
	break;
    case 1:
	media_root = argv[optind];
	break;
    default:
	Usage(stderr);
	return 1;
    }

    util::http::Client http_client;
    db::steam::Database sdb(mediadb::FIELD_COUNT);
    sdb.SetFieldInfo(mediadb::ID, 
		     db::steam::FIELD_INT|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::PATH,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::ARTIST,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::ALBUM,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::GENRE,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);

    util::WorkerThreadPool wtp(util::WorkerThreadPool::LOW, nthreads);
    db::local::Database db(&sdb, &http_client, &wtp);

    db::local::FileScanner ifs(media_root, flac_root, &sdb, &db, &wtp);

    mediadb::Registry registry;

    ScanProgressDialog spd(&db, &registry);
    spd.show();
    ifs.AddObserver(&spd);

    unsigned int rc = ifs.StartScan();
    assert(rc == 0);
    if (rc)
    {
	fprintf(stderr, "Scanning failed: %u\n", rc);
	exit(1);
    }

    return app.exec();
}

} // anon namespace

} // namespace tageditor

int main(int argc, char *argv[])
{
    return tageditor::Main(argc, argv);
}
