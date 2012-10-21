/** Win32 service (daemon) for chorale
 */
#include "config.h"

#ifdef WIN32

#include <windows.h>
#include <stdio.h>
#include <getopt.h>
#include <pthread.h>
#include <shlobj.h>
#include <shlwapi.h>
#include "cd.h"
#include "database.h"
#include "nfs.h"
#include "web.h"
#include "libutil/worker_thread_pool.h"
#include "libutil/poll.h"
#include "libutil/http_client.h"
#include "libutil/http_server.h"
#include "libutil/trace.h"
#include "libutil/utf8.h"
#include "libreceiver/ssdp.h"
#include "libreceiverd/content_factory.h"
#include "libupnp/server.h"
#include "libupnp/ssdp.h"
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

    static void Install(const char *mp3root, const char *flacroot);
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

    util::WorkerThreadPool m_wtp_low;
    util::WorkerThreadPool m_wtp_normal;
    Database m_db;

public:
    Impl();
    ~Impl();

    void Run();
};

ChoraleService::Impl::Impl()
    : m_exiting(false),
      m_wtp_low(util::WorkerThreadPool::LOW),
      m_wtp_normal(util::WorkerThreadPool::NORMAL)
{    
}

ChoraleService::Impl::~Impl()
{
    m_exiting = true;
    m_poller.Wake();
}

static std::string GetRegistryEntry(const char *key, const char *value)
{
    HKEY hkey;
    LONG rc = RegOpenKeyExA(HKEY_LOCAL_MACHINE, key, 0, 
			    KEY_READ|KEY_QUERY_VALUE, &hkey);
    if (rc != ERROR_SUCCESS)
    {
	TRACE << "Can't open registry key " << key << ": " << rc << "\n";
	return "";
    }

    DWORD type;
    char buf[256];
    DWORD size = sizeof(buf);
    rc = RegQueryValueExA(hkey, value, NULL,
			       &type, (BYTE*)&buf, &size);
    RegCloseKey(hkey);

    if (rc != ERROR_SUCCESS)
    {
	TRACE << "Can't read registry value " << key << ": " << rc << "\n";
	return "";
    }
    if (type != REG_SZ)
    {
	TRACE << "Registry value " << key << " not a string (" << type
	      << ")\n";
	return "";
    }
    TRACE << "RQVE returned " << size << " bytes\n";
    buf[size] = 0;
    return std::string(buf);
}

static LONG SetRegistryEntry(const char *key, const char *value,
			     const std::string& contents)
{
    HKEY hkey;
    DWORD disposition;
    LONG rc = RegCreateKeyExA(HKEY_LOCAL_MACHINE, key, 0, NULL,
			      REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
			      &hkey, &disposition);
    if (rc != ERROR_SUCCESS)
    {
	TRACE << "Can't create registry key " << key << ": " << rc << "\n";
	return rc;
    }
		
    rc = RegSetValueExA(hkey, value, 0, REG_SZ,
			(const BYTE*)contents.c_str(), contents.length() + 1);
    RegCloseKey(hkey);

    if (rc != ERROR_SUCCESS)
    {
	TRACE << "Can't create registry value " << value << ": " << rc << "\n";
    }    
    
    return rc;
}

#define REG_ROOT "SOFTWARE\\chorale.sf.net\\Chorale\\1.0"

#define RIO_REG_ROOT "SOFTWARE\\Diamond Multimedia\\Audio Receiver Manager\\1.0"

const char *mediaroot = "C:\\My Music";

void ChoraleService::Impl::Run()
{
    pthread_win32_process_attach_np();

    std::string mp3root = GetRegistryEntry(REG_ROOT, "mp3root");
    std::string flacroot = GetRegistryEntry(REG_ROOT, "flacroot");

    if (mp3root.empty())
    {
	fprintf(stderr,
		"Chorale not installed properly -- no music root set\n");
	return;
    }

    char common_app_data[MAX_PATH];
    if (FAILED(SHGetFolderPathA(NULL,
				CSIDL_COMMON_APPDATA|CSIDL_FLAG_CREATE,
				NULL, 0, common_app_data)))
    {
	fprintf(stderr,
		"Chorale not installed properly -- can't find shared app data\n");
	return;
    }
    PathAppend(common_app_data, "chorale.db");
    std::string dbfile(common_app_data);

    m_db.Init(mp3root.c_str(), flacroot.c_str(), &m_wtp_low, dbfile);

    std::string layout = GetRegistryEntry(RIO_REG_ROOT "\\HTTP", "LAYOUT");

    std::string webroot;
    const char *rback = strrchr(layout.c_str(), '\\');
    if (rback)
	webroot = std::string(layout.c_str(), rback);
    else
	webroot = "C:\\My Music";

    TRACE << "Webroot is " << webroot << "\n";

    std::string arf = GetRegistryEntry(RIO_REG_ROOT "\\NFS", "IMAGE FILE");
    
    TRACE << "Arf is '" << arf << "'\n";

    NFSService nfsd;
    if (!arf.empty())
    {
	unsigned int rc = nfsd.Init(&m_poller, arf.c_str());
	if (rc != 0)
	{
	    TRACE << "Can't initialise NFS: " << rc << "\n";
	    arf.clear();
	}
    }

    util::http::Server ws(&m_poller, &m_wtp_normal);
    
    receiverd::ContentFactory rcf(m_db.Get());
    util::http::FileContentFactory fcf(webroot+"/upnp", "/upnp");
    util::http::FileContentFactory fcf2(webroot+"/layout", "/layout");

    if (1)
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
    ws.Init(12078);
    TRACE << "Webserver got port " << ws.GetPort() << "\n";

    receiver::ssdp::Server pseudo_ssdp;

    if (1)
    {
	pseudo_ssdp.Init(&m_poller);
	pseudo_ssdp.RegisterService(receiver::ssdp::s_uuid_musicserver, 
				    ws.GetPort());
	if (!arf.empty())
	    pseudo_ssdp.RegisterService(receiver::ssdp::s_uuid_softwareserver,
					nfsd.GetPort());
	TRACE << "Receiver service started\n";
    }

    char hostname[256];
    hostname[0] = '\0';
    ::gethostname(hostname, sizeof(hostname)-1);
    hostname[255] = '\0';
    char *dot = strchr(hostname, '.');
    if (dot && dot > hostname)
	*dot = '\0';

    upnp::ssdp::Responder ssdp(&m_poller);
    util::http::Client wc;
    upnp::Server upnpserver(&m_poller, &wc, &ws, &ssdp);
    upnpd::MediaServer mediaserver(m_db.Get(), &upnpserver);

    if (0)
    {
	mediaserver.SetFriendlyName(std::string(mediaroot)
				    + " on " + hostname);
    }

# ifdef HAVE_CD
    CDService cds(NULL);
    if (1)
	cds.Init(&ws, hostname, &upnpserver);
# endif

    upnpserver.Init();

    TRACE << "Polling\n";

    while (!m_exiting)
	m_poller.Poll(util::Poller::INFINITE_MS);

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

void ChoraleService::Install(const char *mp3root, const char *flacroot)
{
    /* At this point mp3root is guaranteed to be non-NULL (by the argc
     * checks), but flacroot might be NULL if no secondary root was
     * given.
     */
    LONG rc = SetRegistryEntry(REG_ROOT, "mp3root", mp3root);
    if (rc == ERROR_SUCCESS)
	rc = SetRegistryEntry(REG_ROOT, "flacroot", flacroot ? flacroot : "");
    if (rc != ERROR_SUCCESS)
    {
	fprintf(stderr, "Unable to set registry entries (code %u)\n\n",
		(unsigned int)rc);
	fprintf(stderr, "You need to be logged-in with Administrator privileges to install this service.\n");
        fprintf(stderr, "Installation failed\n");
	exit(1);
    }

    if (IsInstalled())
    {
        fprintf(stderr, 
		"The " PACKAGE_NAME " service is already installed.\n"
		"You can use Control Panel / Administrative Tools / Services to start and stop it.\n");
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
"Usage: choralesvc -i <root-directory> [<flac-root-directory>]\n"
"       choralesvc -d|-u|-h\n"
"Serve local media resources (files, inputs, outputs), to network clients.\n"
"\n"
"The options are:\n"
#ifdef WITH_DEBUG
" -d, --debug     Run service in foreground (for debugging)\n"
#endif
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

/** GCC 4.3.0 miscompiles the wmemcpy in mingw-runtime-3.14, causing all hell
 * to break loose. Use this, much simpler, wmemcpy instead.
 */
wchar_t *wmemcpy(wchar_t *dest, const wchar_t *src, size_t n)
{
    return (wchar_t*)memcpy(dest, src, n*2);
}

/** GCC 4.3.0 miscompiles the wmemcpy in mingw-runtime-3.14, causing
 * all hell to break loose. Just in case wmemmove is similarly
 * affected, use this, much simpler, one instead.
 */
wchar_t *wmemmove(wchar_t *dest, const wchar_t *src, size_t n)
{
    return (wchar_t*)memmove(dest, src, n*2);
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

    int which = 0;

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
	case 'u':
	case 'd':
	    if (which)
	    {
		Usage(stderr);
		return 1;
	    }
	    which = option;
	    break;
	}
    }

    if (which == 'i')
    {
	// Install mode -- 1 or 2 arguments
	if (argc > optind+2 || argc <= optind)
	{
	    Usage(stderr);
	    return 1;
	}
    }
    else
    {
	// Not install mode -- must be 0 arguments
	if (argc > optind)
	{
	    Usage(stderr);
	    return 1;
	}
    }

    if (which == 'd')
	ChoraleService::RunForeground();
    else if (which == 'i')
	ChoraleService::Install(argv[optind], argv[optind+1]);
    else if (which == 'u')
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
