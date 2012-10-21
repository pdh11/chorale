/** Win32 service (daemon) for chorale
 */
#include "config.h"

#ifdef WIN32

#include <windows.h>
#include <stdio.h>
#include <getopt.h>
#include "cd.h"
#include "database.h"
#include "web.h"
#include "libutil/worker_thread_pool.h"
#include "libutil/poll.h"
#include "libutil/web_server.h"
#include "libutil/trace.h"
#include "libreceiver/ssdp.h"
#include "libreceiverd/content_factory.h"
#include "libupnp/server.h"
#include "libupnpd/media_server.h"
#include "libupnpd/optical_drive.h"

class ChoraleService
{
    static SERVICE_STATUS_HANDLE sm_status_handle;
    static SERVICE_STATUS sm_status;

    static bool sm_foreground;

    class Impl;

    static Impl *sm_impl;

public:

    static void Install();
    static void Uninstall();

    static bool IsInstalled();

    /** Run the service in the foreground.
     */
    static void RunForeground();

    /** Run the service as a service.
     */
    static void RunAsService();

    /** Called asynchronously when something happens in Control Panel
     * which we need to know about.
     */
    static void WINAPI ServiceHandler(DWORD ctrl);

    /** Entry point from Windows services manager
     */
    static void WINAPI ServiceMain(DWORD, LPWSTR*);
};

class ChoraleService::Impl
{
    volatile bool m_exiting;

    util::Poller m_poller;
    util::PollWaker m_waker;

    util::WorkerThreadPool m_wtp;
    Database m_db;

public:
    Impl();
    ~Impl();

    void Run();
};

ChoraleService::Impl::Impl()
    : m_exiting(false),
      m_waker(&m_poller),
      m_wtp(4)
{    
}

ChoraleService::Impl::~Impl()
{
    m_exiting = true;
    m_waker.Wake();
}

void ChoraleService::Impl::Run()
{
    const char *mediaroot = "C:\\My Music";
    const char *webroot = "C:\\web";

    pthread_win32_process_attach_np();

//    m_db.Init(mediaroot, "", m_wtp.GetTaskQueue(), "c:\\music.db");

    util::WebServer ws;

    receiverd::ContentFactory rcf(m_db.Get());
    util::FileContentFactory fcf(std::string(webroot)+"/upnp",
				 "/upnp");
    util::FileContentFactory fcf2(std::string(webroot)+"/layout",
				 "/layout");

    if (0)
    {
	ws.AddContentFactory("/tags", &rcf);
	ws.AddContentFactory("/query", &rcf);
	ws.AddContentFactory("/content", &rcf);
	ws.AddContentFactory("/results", &rcf);
	ws.AddContentFactory("/list", &rcf);
	ws.AddContentFactory("/layout", &fcf2);
    }
    ws.AddContentFactory("/upnp", &fcf);

    RootContentFactory rootcf(m_db.Get());

    ws.AddContentFactory("/", &rootcf);
    ws.Init(&m_poller, webroot, 0);
    TRACE << "Webserver got port " << ws.GetPort() << "\n";

    receiver::ssdp::Server ssdp;

    if (0)
    {
	ssdp.Init(&m_poller);
	ssdp.RegisterService(receiver::ssdp::s_uuid_musicserver, ws.GetPort());
//	if (do_nfs)
//	    ssdp.RegisterService(receiver::ssdp::s_uuid_softwareserver, 111,
//				 software_server);
	TRACE << "Receiver service started\n";
    }

    char hostname[256];
    hostname[0] = '\0';
    ::gethostname(hostname, sizeof(hostname)-1);
    hostname[255] = '\0';
    char *dot = strchr(hostname, '.');
    if (dot && dot > hostname)
	*dot = '\0';

#ifdef HAVE_UPNP
    upnpd::MediaServer *mediaserver = NULL;
    upnp::Device *root_device = NULL;

    if (0)
    {
	mediaserver = new upnpd::MediaServer(m_db.Get(), ws.GetPort(), 
					     mediaroot);
	mediaserver->SetFriendlyName(std::string(mediaroot)
				     + " on " + hostname);
	root_device = mediaserver;
    }

# ifdef HAVE_CD
    CDService cds(NULL);
    if (1)
	cds.Init(&ws, hostname, &root_device);
# endif

    upnp::Server svr;
    if (root_device)
    {
	svr.Init(root_device, ws.GetPort());
    }
#endif

    while (!m_exiting)
	m_poller.Poll(util::Poller::INFINITE_MS);

#ifdef HAVE_UPNP
    delete mediaserver;
#endif

    pthread_win32_process_detach_np();
}


        /* Starting and stopping the service */


SERVICE_STATUS_HANDLE ChoraleService::sm_status_handle;

SERVICE_STATUS ChoraleService::sm_status =
{
    SERVICE_WIN32_OWN_PROCESS,
    SERVICE_START_PENDING,
    SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN,
    NO_ERROR,
    0,
    0,
    0
};

bool ChoraleService::sm_foreground = false;

ChoraleService::Impl *ChoraleService::sm_impl = NULL;

void WINAPI ChoraleService::ServiceHandler(DWORD ctrl)
{
    if (ctrl == SERVICE_CONTROL_STOP
	|| ctrl == SERVICE_CONTROL_SHUTDOWN)
    {
        sm_status.dwCurrentState = SERVICE_STOP_PENDING;
        sm_status.dwWaitHint = 1000;
        SetServiceStatus(sm_status_handle, &sm_status);

	delete sm_impl;
	sm_impl = NULL;
            
	sm_status.dwWaitHint = 0;
        sm_status.dwCurrentState = SERVICE_STOPPED;
    }
    SetServiceStatus(sm_status_handle, &sm_status);
}

void WINAPI ChoraleService::ServiceMain(DWORD, LPWSTR*)
{
    if (!sm_foreground)
    {
	sm_status_handle = RegisterServiceCtrlHandlerW(L"" PACKAGE_NAME,
						       &ChoraleService::ServiceHandler);
        SetServiceStatus(sm_status_handle, &sm_status);
    }

    sm_impl = new Impl();

    sm_status.dwCurrentState = SERVICE_RUNNING;
    if (!sm_foreground)
        SetServiceStatus(sm_status_handle, &sm_status);

    sm_impl->Run();
}

void ChoraleService::RunAsService()
{
    static WCHAR servicename[] = L"" PACKAGE_NAME;
    static SERVICE_TABLE_ENTRYW ste[] =
     {
	 { servicename, &ChoraleService::ServiceMain },
	 { NULL, NULL }
     };

    /* Demands that its arguments aren't const, for some reason */
    StartServiceCtrlDispatcherW(ste);
}

void ChoraleService::RunForeground()
{
    sm_foreground = true;
    printf("Starting Chorale server service\n");
    ServiceMain(0, NULL);
}


        /* Installation and uninstallation */


bool ChoraleService::IsInstalled()
{
    SC_HANDLE mgr = OpenSCManager(NULL, NULL, GENERIC_READ);

    if (!mgr)
        return false;

    SC_HANDLE svc = OpenServiceW(mgr, L"" PACKAGE_NAME, GENERIC_READ);

    bool result = (svc != NULL);
    
    if (svc)
        CloseServiceHandle(svc);

    CloseServiceHandle(mgr);

    return result;
}

void ChoraleService::Install()
{
    if (IsInstalled())
    {
        fprintf(stderr, 
		"The " PACKAGE_NAME " service is already installed.\n"
		"You can use Control Panel / Services to start and stop it.\n");
        return;
    }

    SC_HANDLE mgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);

    if (!mgr)
    {
        fprintf(stderr, "Could not access the Service Manager database.\n");
        if (GetLastError() == ERROR_ACCESS_DENIED)
        {
            fprintf(stderr, 
		    "You need to be logged-in with Administrator privileges to install this service.\n");
        }
        fprintf(stderr, "Installation failed\n");
        return;
    }

    WCHAR my_path[MAX_PATH];
    GetModuleFileNameW(NULL, my_path, MAX_PATH);

    SC_HANDLE svc = CreateServiceW(mgr, L"" PACKAGE_NAME, L"" PACKAGE_NAME, 
				   SERVICE_ALL_ACCESS,
				   SERVICE_WIN32_OWN_PROCESS, 
				   SERVICE_AUTO_START,
				   SERVICE_ERROR_NORMAL,
				   my_path,
				   NULL, NULL, NULL, NULL, NULL);

    if (!svc)
    {
        int err = GetLastError();

        CloseServiceHandle(mgr);
        fprintf(stderr,
		"The service could not be created (error number is %d)\n", 
		err);
        fprintf(stderr, "Installation failed\n");
        return;
    }

    if (!StartService(svc, 0, NULL))
    {
        int err = GetLastError();
        CloseServiceHandle(svc);
        CloseServiceHandle(mgr);
        fprintf(stderr,
		"The service could not be started (error number is %d)\n",
		err);
        fprintf(stderr, "Installation failed\n");
        return;
    }

    CloseServiceHandle(svc);
    CloseServiceHandle(mgr);

    printf("The service has been installed and started.\n");
}

void ChoraleService::Uninstall()
{
    if (!IsInstalled())
    {
        fprintf(stderr,
		"The " PACKAGE_NAME " service isn't currently installed.\n" );
        return;
    }

    SC_HANDLE mgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);

    if (!mgr)
    {
        fprintf(stderr, "Could not access the Service Manager database.\n");
        if (GetLastError() == ERROR_ACCESS_DENIED)
        {
            fprintf(stderr, 
		    "You need to be logged-in with Administrator privileges to remove this service.\n");
        }
        fprintf(stderr, "Uninstallation failed\n");
        return;
    }

    SC_HANDLE svc = OpenServiceW(mgr, L"" PACKAGE_NAME, SERVICE_ALL_ACCESS);

    if (!svc)
    {
        int err = GetLastError();

        CloseServiceHandle( mgr );
        fprintf(stderr, "The service could not be opened (error number is %d)\n", err);
        fprintf(stderr, "Uninstallation failed\n");
        return;
    }

    SERVICE_STATUS ss;
    if (!ControlService(svc, SERVICE_CONTROL_STOP, &ss))
    {
        int err = GetLastError();

        if (err != ERROR_SERVICE_NOT_ACTIVE)
        {
            CloseServiceHandle(svc);
            CloseServiceHandle(mgr);
            fprintf(stderr, "The service could not be stopped (error number is %d)\n", err);
            fprintf(stderr, "Uninstallation failed\n");
            return;
        }
    }

    if (!DeleteService(svc))
    {
        int err = GetLastError();
        CloseServiceHandle(svc);
        CloseServiceHandle(mgr);
        fprintf(stderr, "The service could not be deleted (error number is %d)\n", err);
        fprintf(stderr, "Uninstallation failed\n");
        return;
    }

    CloseServiceHandle(svc);
    CloseServiceHandle(mgr);
}


        /* Command-line handling */


static void Usage(FILE *f)
{
    fprintf(f,
	    "Usage: choralesvc -d|-i|-u|-h\n"
"Serve local media resources (files, inputs, outputs), to network clients.\n"
"\n"
"The options are:\n"
" -d, --debug     Run service in foreground (for debugging)\n"
" -i, --install   Install service\n"
" -u, --uninstall Uninstall service\n"
" -h, --help      These hastily-scratched notes\n"
"\n"
"Once the service is installed, it is automatically started; you can then\n"
"use Control Panel / Services to stop or restart it.\n"
"\n"
"From " PACKAGE_STRING " built on " __DATE__ "\n"
	);
}

int main(int argc, char *argv[])
{
    static const struct option options[] =
    {
	{ "debug",     no_argument, NULL, 'd' },
	{ "install",   no_argument, NULL, 'i' },
	{ "uninstall", no_argument, NULL, 'u' },
	{ "help",      no_argument, NULL, 'h' },
	{ NULL, 0, NULL, 0 }
    };

    bool do_install = false;
    bool do_uninstall = false;
    bool do_debug = false;

    int option_index;
    int option;
    while ((option = getopt_long(argc, argv, "diuh", options, &option_index))
	   != -1)
    {
	switch (option)
	{
	case 'h':
	    Usage(stdout);
	    return 0;

	case 'i':
	    do_install = true;
	    break;

	case 'u':
	    do_uninstall = true;
	    break;
	    
	case 'd':
	    do_debug = true;
	    break;
	}
    }

    if (argc > optind)
    {
	Usage(stderr);
	return 1;
    }

    if (do_debug)
	ChoraleService::RunForeground();
    else if (do_install)
	ChoraleService::Install();
    else if (do_uninstall)
	ChoraleService::Uninstall();
    else
    {
	// We're actually being run as a service
	if (ChoraleService::IsInstalled())
	    ChoraleService::RunAsService();
    }
    return 0;
}

#endif // WIN32
