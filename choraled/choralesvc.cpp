/** Win32 service (daemon) for chorale
 */
#include "config.h"

#ifdef WIN32

#include <windows.h>
#include <stdio.h>
#include <getopt.h>
#include <pthread.h>
#include <shlobj.h>
#include <stdarg.h>
#include "cd.h"
#include "database.h"
#include "nfs.h"
#include "web.h"
#include "libutil/worker_thread_pool.h"
#include "libutil/poll.h"
#include "libutil/http_client.h"
#include "libutil/http_server.h"
#include "libutil/trace.h"
#include "libutil/file.h"
#include "libutil/utf8.h"
#include "libreceiver/ssdp.h"
#include "libreceiverd/content_factory.h"
#include "libupnp/server.h"
#include "libupnp/ssdp.h"
#include "libupnpd/media_server.h"
#include "libupnpd/optical_drive.h"
#include "main.h"

class ChoraleService
{
    static SERVICE_STATUS_HANDLE sm_status_handle;
    static SERVICE_STATUS sm_status;

    static bool sm_foreground;

    class Impl;

    static Impl *sm_impl;

public:

    static void Install(const choraled::Settings&);
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
public:
    Impl();
    ~Impl();

    void Run();
};

ChoraleService::Impl::Impl()
{    
}

ChoraleService::Impl::~Impl()
{
    choraled::s_exiting = true;
    if (choraled::s_poller)
	choraled::s_poller->Wake();
}

static std::wstring GetRegistryEntry(const wchar_t *key, const wchar_t *value)
{
    HKEY hkey;
    LONG rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE, key, 0, 
			    KEY_READ|KEY_QUERY_VALUE, &hkey);
    if (rc != ERROR_SUCCESS)
    {
	TRACE << "Can't open registry key " << key << ": " << rc << "\n";
	return std::wstring();
    }

    DWORD type;
    wchar_t buf[256];
    DWORD size = sizeof(buf);
    rc = RegQueryValueExW(hkey, value, NULL, &type, (BYTE*)&buf, &size);
    RegCloseKey(hkey);

    if (rc != ERROR_SUCCESS)
    {
	TRACE << "Can't read registry value " << key << ": " << rc << "\n";
	return std::wstring();
    }
    if (type != REG_SZ)
    {
	TRACE << "Registry value " << key << " not a string (" << type
	      << ")\n";
	return std::wstring();
    }
//    TRACE << "RQVE returned " << size << " bytes\n";
    buf[size] = 0;
    return std::wstring(buf);
}

static LONG SetRegistryEntry(const wchar_t *key, const wchar_t *value,
			     const std::wstring& contents)
{
    HKEY hkey;
    DWORD disposition;
    LONG rc = RegCreateKeyExW(HKEY_LOCAL_MACHINE, key, 0, NULL,
			      REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
			      &hkey, &disposition);
    if (rc != ERROR_SUCCESS)
    {
	TRACE << "Can't create registry key " << key << ": " << rc << "\n";
	return rc;
    }
		
    rc = RegSetValueExW(hkey, value, 0, REG_SZ,
			(const BYTE*)contents.c_str(), 
			(contents.length() + 1) * sizeof(wchar_t));
    RegCloseKey(hkey);

    if (rc != ERROR_SUCCESS)
    {
	TRACE << "Can't create registry value " << value << ": " << rc << "\n";
    }    
    
    return rc;
}

class Win32Complaints: public choraled::Complaints
{
public:
    void Complain(int loglevel, const char *format, ...)
    {
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	/// @todo Event logging (requires message file/windmc/windres)
#if 0
	HANDLE h = RegisterEventSorce(NULL, PACKAGE_NAME);
	char buf[512];
	va_list args;

	va_args(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);

	WORD wType = EVENTLOG_INFORMATION_TYPE;
	if (loglevel == LOG_WARNING)
	    wType = EVENTLOG_WARNING_TYPE;
	else if (loglevel == LOG_ERR)
	    wType = EVENTLOG_ERROR_TYPE;

	const char *ptr = buf;
	ReportEvent(h, wType, 0, 0x60000000, NULL, 1, 0, &ptr, 1);

	DeregisterEventSource(h);
#endif
    }
};

static const wchar_t REG_ROOT[] = L"SOFTWARE\\chorale.sf.net\\Chorale\\1.0";

#define RIO_REG_ROOT L"SOFTWARE\\Diamond Multimedia\\Audio Receiver Manager\\1.0"

void ChoraleService::Impl::Run()
{
    pthread_win32_process_attach_np();

    std::string mp3root = 
	util::UTF16ToUTF8(GetRegistryEntry(REG_ROOT, L"mp3root"));
    std::string flacroot = 
	util::UTF16ToUTF8(GetRegistryEntry(REG_ROOT, L"flacroot"));
    std::wstring assimilate_receiver = GetRegistryEntry(REG_ROOT, 
						       L"assimilate_receiver");

    choraled::Settings settings;
    memset(&settings, '\0', sizeof(settings));

    settings.flags = choraled::CD;

    if (!mp3root.empty())
	settings.flags |= choraled::LOCAL_DB | choraled::MEDIA_SERVER;

    if (!assimilate_receiver.empty())
	settings.flags |= choraled::ASSIMILATE_RECEIVER | choraled::MEDIA_SERVER;
    else if (!mp3root.empty())
	settings.flags |= choraled::RECEIVER;

    util::utf16_t common_app_data[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL,
				CSIDL_COMMON_APPDATA|CSIDL_FLAG_CREATE,
				NULL, 0, common_app_data)))
    {
	fprintf(stderr,
		"Chorale not installed properly -- can't find shared app data\n");
	return;
    }
    std::string dbfile(util::UTF16ToUTF8(common_app_data) + "/chorale.db");

    std::string layout;

    if (settings.flags & choraled::RECEIVER)
	layout = util::UTF16ToUTF8(GetRegistryEntry(RIO_REG_ROOT "\\HTTP",
						    L"LAYOUT"));

    std::string webroot;
    const char *rback = strrchr(layout.c_str(), '\\');
    if (rback)
	webroot = std::string(layout.c_str(), rback);
    else
    {
	util::utf16_t widename[512];
	if (::GetModuleFileNameW(NULL, widename, sizeof(widename)/2))
	{
	    webroot = util::UTF16ToUTF8(widename);
	    TRACE << "choralesvc installed as " << webroot << "\n";
	    webroot = util::GetDirName(webroot.c_str()) + "\\web";
	}
	else
	    webroot = "C:\\My Music";
    }

    webroot = util::Canonicalise(webroot);

    TRACE << "Webroot is " << webroot << "\n";

    std::string arf;

    if (settings.flags & choraled::RECEIVER)
    {
	arf = util::UTF16ToUTF8(GetRegistryEntry(RIO_REG_ROOT "\\NFS", 
						 L"IMAGE FILE"));
    
	TRACE << "Arf is '" << arf << "'\n";
    }

    settings.database_file = dbfile.c_str();
    settings.web_root = webroot.c_str();
    settings.receiver_software_server = NULL;
    settings.receiver_arf_file = arf.c_str();
    settings.media_root = mp3root.c_str();
    settings.flac_root = flacroot.c_str();
    settings.web_port = (unsigned short)((settings.flags & choraled::RECEIVER) ? 12078 : 0);
    settings.nthreads = 32;

    Win32Complaints complaints;

    choraled::Main(&settings, &complaints);

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

void ChoraleService::Install(const choraled::Settings& settings)
{
    const char *mp3root = settings.media_root;
    const char *flacroot = settings.flac_root;
    bool assimilate_receiver
	= (settings.flags & choraled::ASSIMILATE_RECEIVER) != 0;
    
    LONG rc = SetRegistryEntry(REG_ROOT, L"mp3root",
			       mp3root ? util::UTF8ToUTF16(mp3root)
			               : std::wstring());
    if (rc == ERROR_SUCCESS)
	rc = SetRegistryEntry(REG_ROOT, L"flacroot",
			      flacroot ? util::UTF8ToUTF16(flacroot)
			               : std::wstring());
    if (rc == ERROR_SUCCESS)
	rc = SetRegistryEntry(REG_ROOT, L"assimilate_receiver",
			      assimilate_receiver ? L"yes" : L"");
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

    util::utf16_t my_path[MAX_PATH];
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

	if (err == ERROR_SERVICE_MARKED_FOR_DELETE)
	{
	    fprintf(stderr, "The service is marked for deletion but can't disappear entirely yet.\n");
	    fprintf(stderr, "For instance, if Control Panel/Services is open, that prevents\nservices from disappearing until it is closed and reopened.\n");
	}
	else
	{
	    fprintf(stderr, "The service could not be deleted (error number is %d)\n", err);
	    fprintf(stderr, "Uninstallation failed\n");
	    return;
	}
    }
    else
    {
	printf("The service has been stopped and uninstalled\n");
    }

    CloseServiceHandle(svc);
    CloseServiceHandle(mgr);
}


        /* Command-line handling */


#if (HAVE_LIBCDIOP && !HAVE_PARANOIA)
#define LICENCE "This program is free software under the GNU General Public Licence.\n\n"
#elif (HAVE_TAGLIB || HAVE_PARANOIA)
#define LICENCE "This program is free software under the GNU Lesser General Public Licence.\n\n"
#else
#define LICENCE "This program is placed in the public domain by its authors.\n\n"
#endif

static void Usage(FILE *f)
{
    fprintf(f,
"Usage: choralesvc -i [-a] <root-directory> [<flac-root-directory>]\n"
"       choralesvc -d|-u|-h\n"
"Serve local media resources (files, inputs, outputs), to network clients.\n"
"\n"
"The options are:\n"
#ifdef WITH_DEBUG
" -d, --debug     Run service in foreground (for debugging)\n"
#endif
" -i, --install   Install service\n"
" -u, --uninstall Uninstall service\n"
"     --assimilate-receiver  Gateway existing Receiver server to UPnP\n"
" -h, --help      These hastily-scratched notes\n"
"\n"
"Once the service is installed, it is automatically started; you can then\n"
"use Control Panel / Services to stop or restart it.\n"
"\n"
	    LICENCE
"From " PACKAGE_STRING " built on " __DATE__ ".\n"
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

/* We use main() rather than wmain() because there isn't a wide
 * version of getopt. This means that the command-line arguments are
 * char*, but IN THE SYSTEM ANSI CODEPAGE. All char* strings
 * everywhere outside this file in Chorale are UTF-8.
 */
int main(int argc, char *argv[])
{
    static const struct option options[] =
    {
	{ "debug",     no_argument, NULL, 'd' },
	{ "install",   no_argument, NULL, 'i' },
	{ "uninstall", no_argument, NULL, 'u' },
	{ "help",      no_argument, NULL, 'h' },
	{ "assimilate-receiver", no_argument, NULL, 11 },
	{ NULL, 0, NULL, 0 }
    };

    choraled::Settings settings;
    memset(&settings, '\0', sizeof(settings));

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

	case 11:
	    settings.flags |= choraled::ASSIMILATE_RECEIVER;
	    break;

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

    std::string flac_root, mp3_root;

    if (which == 'i')
    {
	switch (argc-optind) // Number of remaining arguments
	{
	case 2:
	    flac_root = util::LocalEncodingToUTF8(argv[optind+1]);
	    settings.flac_root = flac_root.c_str();
	    /* fall through */
	case 1:
	    mp3_root = util::LocalEncodingToUTF8(argv[optind]);
	    settings.media_root = mp3_root.c_str();
	    /* fall through */
	case 0:
	    break;
	default:
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
	ChoraleService::Install(settings);
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
