PACKAGE="chorale"
PACKAGE_VERSION="0.21"
PACKAGE_WEBSITE="https://github.com/pdh11/chorale"

import subprocess
import os
SetOption('num_jobs', os.cpu_count())

env = Environment()
if not env.GetOption('clean'):
    PREFIX = ARGUMENTS.get("PREFIX", "/usr/local")
    print("Compiling Chorale to install in PREFIX="+PREFIX)
    conf = env.Configure(config_h = "config.h")
    conf.CheckLib('wrap')
    conf.CheckLib('boost_regex')
    conf.CheckLib('boost_system')
    conf.CheckLib('boost_thread')
    conf.CheckLib('pthread')
    for header in [
            "time.h",
            "poll.h",
            "sched.h",
            "net/if.h",
            "stdint.h",
            "sys/disk.h",
            "sys/time.h",
            "sys/poll.h",
            "sys/types.h",
            "sys/socket.h",
            "netinet/ip.h",
            "sys/utsname.h",
            "linux/cdrom.h",
            "sys/resource.h",
    ]:
        conf.CheckHeader(header)
    conf.CheckHeader("boost/spirit/include/classic.hpp", language="C++")
    for f in [
            "pipe",
            "fsync",
            "mkdir",
            "gettid",
            "eventfd",
            "pread64",
            "mkstemp",
            "scandir",
            "gmtime_r",
            "pwrite64",
            "realpath",
            "vasprintf",
            "ftruncate",
            "localtime_r",
            "setpriority",
            "getaddrinfo",
            "inotify_init",
            "gettimeofday",
            "posix_fadvise",
            "posix_fallocate",
            "gethostbyname_r",
            "sync_file_range",
            "sched_setscheduler",
    ]:
        conf.CheckFunc(f)
    for t in [
            "IP_PKTINFO",
    ]:
        conf.CheckDeclaration(t, "#include <netinet/ip.h>")
    for (t, header) in [
            ("boost::mutex", "<boost/thread/mutex.hpp>"),
            ("boost::thread", "<boost/thread/thread.hpp>"),
            ("boost::condition", "<boost/thread/condition.hpp>"),
            ("boost::recursive_mutex", "<boost/thread/recursive_mutex.hpp>"),
            ("boost::mutex::scoped_lock", "<boost/thread/mutex.hpp>"),
            ("boost::recursive_mutex::scoped_lock", "<boost/thread/recursive_mutex.hpp>"),
    ]:
        n = conf.CheckTypeSize(t, "#include "+header, language="C++")
    for e in [
            "EINVAL",
            "EISCONN",
            "EALREADY",
            "ECANCELED",
            "EOPNOTSUPP",
            "EWOULDBLOCK",
            "EINPROGRESS",
            ]:
        conf.Define("HAVE_"+e, 1)
    # Changed in glibc 2.10, we don't care about earlier any more
    conf.Define("SCANDIR_COMPARATOR_ARG_T", "const struct dirent**")
    conf.Define("HAVE_CONDITION_TIMED_WAIT_INTERVAL", 1)
    conf.Define("HAVE_LAME", 1)
    conf.Define("HAVE_TAGLIB", 1)
    conf.Define("HAVE_LIBFLAC", 1)
    conf.Define("HAVE_PARANOIA", 1)
    conf.Define("HAVE_DECL_PARANOIA_CB_CACHEERR", 1)
    conf.Define("HAVE_IP_PKTINFO", "HAVE_DECL_IP_PKTINFO")
    conf.Define("PACKAGE_NAME", '"'+PACKAGE+'"')
    conf.Define("PACKAGE_VERSION", '"'+PACKAGE_VERSION+'"')
    conf.Define("PACKAGE_STRING", 'PACKAGE_NAME " " PACKAGE_VERSION');
    conf.Define("PACKAGE_WEBSITE", '"'+PACKAGE_WEBSITE+'"')
    conf.Define("LOCALSTATEDIR", '"'+PREFIX+'/var"')
    conf.Define("CHORALE_DATADIR", '"'+PREFIX+'/share"')
    conf.Define("SRCROOT", '"' + Dir(".").path + '"')
    conf.CheckLib(["cdda_paranoia"])
    conf.CheckLib(["cdda_interface"])
    env = conf.Finish()
    env.MergeFlags("!pkg-config --libs --cflags taglib")
    env.MergeFlags("!pkg-config --libs flac")
    env.Append(LIBS="-lmp3lame")
    env.Append(CPPPATH="#")

debug = ARGUMENTS.get('DEBUG', 1)
profile = ARGUMENTS.get('PROFILE', 0)
if debug:
    suffix = "debug"
    env.Append(CCFLAGS=["-g", "-O2", "-DDEBUG=1"])
elif profile:
    suffix = "profile"
    env.Append(CCFLAGS=["-fprofile-arcs", "-ftest-coverage", "-DDEBUG=0",
                        "-DNDEBUG", "-O2"])
else:
    suffix = "release"
    env.Append(CCFLAGS=["-DDEBUG=0", "-O2", "-Os", "-DNDEBUG"])

if ARGUMENTS.get('VERBOSE') != "1":
    env["ARCOMSTR"] = "Library ${TARGET.file}"
    env["RANLIBCOMSTR"] = "Ranlib ${TARGET.file}"
    env["LINKCOMSTR"] = "Linking ${TARGET.file}"
    env["CCCOMSTR"] = "Compiling ${SOURCE}"
    env["CXXCOMSTR"] = "Compiling ${SOURCE}"

testenv = env.Clone()
testenv.Append(CCFLAGS="-DTEST")
testenv["CXXCOMSTR"] = "Compiling ${SOURCE} as test"

def XSLT(service, stylesheet, output):
    src = "libupnp/" + service + ".xml"
    env.Command(output, [src, stylesheet],
                ["xsltproc --stringparam class " + service + " -o " + output + " " + stylesheet + " " + src]
                )

for i in [
	"AVTransport.xml",
        "ConnectionManager.xml",
	"ContentDirectory.xml",
	"ContentSync.xml",
	"DeviceSecurity.xml",
	"MSMediaReceiverRegistrar.xml",
	"OpticalDrive.xml",
	"RemoteUIClient.xml",
	"RemoteUIServer.xml",
	"RenderingControl.xml",
	"ScheduledRecording.xml",
	"TestService.xml",
]:
    service = os.path.splitext(i)[0]
    XSLT(service, "libupnp/scpd-header.xsl", "libupnp/" + service + ".h")
    XSLT(service, "libupnp/scpd-source.xsl", "libupnp/" + service + ".cpp")
    XSLT(service, "libupnp/scpd-clientheader.xsl", "libupnp/" + service + "_client.h")
    XSLT(service, "libupnp/scpd-clientsource.xsl", "libupnp/" + service + "_client.cpp")
    XSLT(service, "libupnp/scpd-serversource.xsl", "libupnp/" + service + "_server.cpp")
    # Server headers are different for some reason
    src = "libupnp/"+i
    out = "libupnp/"+service+"_server.h"
    env.Command(out, [src, "libupnp/scpd.xsl"],
                ["xsltproc --stringparam class " + service + " --stringparam mode serverheader -o " + out + " libupnp/scpd.xsl libupnp/"+ i]
                )


libs = []

def ChoraleLib(name):
    global libs
    srcs = Glob("lib"+ name + "/*.cpp") + Glob("lib" + name + "/*.c")
    objs = []
    for j in srcs:
        obj = "#obj/"+suffix+"/"+os.path.splitext(str(j))[0]+".o"
        env.StaticObject(obj, j)
        objs.append(obj)

    lib0 = env.StaticLibrary(target="#obj/"+suffix+"/lib"+name+"/lib"+name+"-untested.a", source=objs)

    tests = []
    tested = []
    for j in srcs:
        testbin = "obj/"+suffix+"/"+os.path.splitext(str(j))[0]+".test"
        testobj = testbin + ".o"
        testenv.StaticObject(target = testobj, source = j)
        testflag = testbin + ".tested"
        testoutput = testbin + ".output"
        test = testenv.Program(target=testbin, source=[testobj, lib0],
                               LIBS=libs + env["LIBS"])
        tests.append(test)
        tested.append(env.Command(testflag, test,
                                  [ "@echo Testing "+str(j),
                                    "@" + str(testbin) + " > "+testoutput + " || { cat "+testoutput+" ; exit 1; }",
                                    '@date >> ' + testflag ]))

    lib = env.StaticLibrary(target="#obj/"+suffix+"/lib"+name+".a", source=objs)
    env.Depends(lib, tested)
    libs = [lib] + libs

def ChoraleBin(name):
    srcs = Glob(name + "/*.cpp")
    objs = []
    for j in srcs:
        obj = "#obj/"+suffix+"/"+os.path.splitext(str(j))[0]+".o"
        env.StaticObject(obj, j)
        objs.append(obj)

    bin = Program(target = "obj/"+suffix+"/bin/"+name,
                  source = objs,
                  LIBS = libs + env["LIBS"])

for i in [
        "util",

        "db",

        "dbsteam",
        "mediadb",
        "upnp",

        "dbupnp",
        "import",
        "receiver",
        "dbmerge",

        "dbreceiver",
        "dblocal",

        "upnpd",
        "receiverd",
]:
    ChoraleLib(i)

for i in ["choraled"]:
    ChoraleBin(i)
