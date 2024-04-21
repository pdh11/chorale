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
    conf.Define("HAVE_HAL", 0)
    conf.Define("HAVE_LAME", 1)
    conf.Define("HAVE_CAIRO", 0)
    conf.Define("HAVE_TAGLIB", 1)
    conf.Define("HAVE_LIBCDDB", 1)
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
    env.MergeFlags("!pkg-config --libs --cflags libcddb")
    env.MergeFlags("!pkg-config --libs flac")
    env.MergeFlags("!pkg-config --libs --cflags Qt5Gui Qt5Core Qt5Widgets")
    env.Append(LIBS="-lmp3lame")
    env.Append(CPPPATH="#")

debug = ARGUMENTS.get('DEBUG', 1)
profile = ARGUMENTS.get('PROFILE', 0)
if debug:
    suffix = "debug"
    env.Append(CCFLAGS=["-g", "-O2", "-DDEBUG=1", "-fPIC"])
elif profile:
    suffix = "profile"
    env.Append(CCFLAGS=["-fprofile-arcs", "-ftest-coverage", "-DDEBUG=0",
                        "-DNDEBUG", "-O2", "-fPIC"])
else:
    suffix = "release"
    env.Append(CCFLAGS=["-DDEBUG=0", "-O2", "-Os", "-DNDEBUG", "-fPIC"])
env.Append(CPPPATH="#obj/"+suffix)

if ARGUMENTS.get('VERBOSE') != "1":
    env["ARCOMSTR"]     = "Library        ${TARGET.file}"
    env["RANLIBCOMSTR"] = "Ranlib         ${TARGET.file}"
    env["LINKCOMSTR"]   = "Linking        ${TARGET.file}"
    env["CCCOMSTR"]     = "Compiling      ${SOURCE}"
    env["CXXCOMSTR"]    = "Compiling      ${SOURCE}"

testenv = env.Clone()
testenv.Append(CCFLAGS="-DTEST")
testenv["CXXCOMSTR"] = "Compiling test ${SOURCE}"

def XSLT(service, stylesheet, output):
    src = "libupnp/" + service + ".xml"
    env.Command(output, [src, stylesheet],
                Action(["@xsltproc --stringparam class " + service + " -o " + output + " " + stylesheet + " " + src],
                "Creating       "+output))

# Autogenerated header and source from XML files
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
    outdir = "obj/" + suffix + "/libupnp/"
    XSLT(service, "libupnp/scpd-header.xsl", outdir + service + ".h")
    XSLT(service, "libupnp/scpd-source.xsl", outdir + service + ".cpp")
    XSLT(service, "libupnp/scpd-clientheader.xsl", outdir + service + "_client.h")
    XSLT(service, "libupnp/scpd-clientsource.xsl", outdir + service + "_client.cpp")
    XSLT(service, "libupnp/scpd-serversource.xsl", outdir + service + "_server.cpp")
    # Server headers are different for some reason
    src = "libupnp/"+i
    out = outdir+service+"_server.h"
    env.Command(out, [src, "libupnp/scpd.xsl"],
                Action(["@xsltproc --stringparam class " + service + " --stringparam mode serverheader -o " + out + " libupnp/scpd.xsl libupnp/"+ i],
                       "Creating       " + out))

# Autogenerated source files from Qt meta-object-compiler
for i in [
        "choralecd/browse_widget.h",
        "choralecd/cd_progress.h",
        "choralecd/cd_widget.h",
        "choralecd/cd_window.h",
        "choralecd/cloud_database_widget.h",
        "choralecd/cloud_style.h",
        "choralecd/cloud_window.h",
        "choralecd/db_widget.h",
        "choralecd/explorer_window.h",
        "choralecd/main_window.h",
        "choralecd/output_widget.h",
        "choralecd/resource_widget.h",
        "choralecd/setlist_model.h",
        "choralecd/setlist_window.h",
        "choralecd/settings_entry.h",
        "choralecd/settings_window.h",
        "choralecd/tag_editor_widget.h",
        "choralecd/tree_model.h",
        "libuiqt/scheduler.h",
        ]:
    out = "obj/" + suffix + "/" + os.path.splitext(i)[0] + ".moc.cpp"
    env.Command(out, i, Action(["moc " + i + " -o " + out],
                               "Mocking        "+i))

# Autogenerated source files from SVG
for (i, area, w, h) in [
        ("icon16", "60:300:150:390", 16, 16),
        ("icon32", "60:300:150:390", 32, 32),
        ("icon48", "60:300:150:390", 48, 48),
        ("icon60", "60:300:150:390", 60, 60),
        ("320x480", "170:150:260:280", 320, 480),
        ("icon16s", "170:300:260:390", 16, 16),
        ("icon32s", "170:300:260:390", 32, 32),
        ("icon48s", "170:300:260:390", 48, 48),
        ("cd", "295:500:395:600", 32, 32),
        ("cddrive", "775:500:875:600", 32, 32),
        ("portcullis", "775:380:875:480", 32, 32),
        ("empeg", "535:500:635:600", 32, 32),
        ("folder", "415:500:515:600", 32, 32),
        ("network", "425:390:505:470", 32, 32),
        ("output", "295:380:395:480", 32, 32),
        ("vibez", "175:500:275:600", 32, 32),
        ("portable", "55:500:155:600", 32, 32),
        ("tv", "655:380:755:480", 32, 32),
        ("noart32", "535:380:635:480", 32, 32),
        ("noart48", "535:380:635:480", 48, 48),
        ("noart100", "535:380:635:480", 100, 100),
        ("resize", "655:500:755:600", 32, 32),
        ("close", "490:680:540:730", 16, 16),
        ("minimise", "430:680:480:730", 16, 16),
        ("dir", "10:680:60:730", 12, 12),
        ("ffwd", "370:680:420:730", 16, 16),
        ("file", "70:680:120:730", 12, 12),
        ("query", "70:740:120:790", 12, 12),
        ("pause", "130:680:180:730", 16, 16),
        ("image", "130:740:180:790", 12, 12),
        ("play", "190:680:240:730", 16, 16),
        ("film", "190:740:240:790", 12, 12),
        ("rew", "310:680:360:730", 16, 16),
        ("stop", "250:680:300:730", 16, 16),
        ("settings", "550:680:600:730", 16, 16),
        ("shuffle", "610:680:660:730", 16, 16),
        ("repeatall", "670:680:720:730", 16, 16),
        ("norepeat", "730:680:780:730", 16, 16),
        ("repeatone", "790:680:840:730", 16, 16),
]:
    png = "obj/" + suffix + "/imagery/" + i + ".png"
    env.Command(png, "imagery/chorale.svg",
                Action([
                    "@inkscape -z --export-filename="+png+" --export-area="+area+" -w "+str(w)+" -h "+str(h) + " ${SOURCE} > /dev/null 2>&1"],
                       "Creating       "+png))
    xpm = "obj/" + suffix + "/imagery/" + i + ".xpm"
    xpm0 = xpm + ".0.xpm"
    env.Command(xpm, png,
                Action([
                    "@convert -transparent white ${SOURCE} "+xpm0,
                    '@sed -e "s/static.*\[]/static const char *const '+i+'_xpm[]/" < '+xpm0 + " > " + xpm],
                       "Creating       "+xpm))

libs = []

def ChoraleLib(name):
    global libs
    srcs = Glob("lib"+ name + "/*.cpp") + Glob("lib" + name + "/*.c")
    objs = []
    for j in srcs:
        obj = "#obj/"+suffix+"/"+os.path.splitext(str(j))[0]+".o"
        env.StaticObject(obj, j)
        objs.append(obj)

    # Autogenerated ones
    auto_srcs = Glob("obj/" + suffix + "/lib" + name + "/*.cpp")
    for j in auto_srcs:
        obj = os.path.splitext(str(j))[0] + ".o"
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
        tested.append(
            env.Command(
                testflag,
                test,
                Action([
                    "@" + str(testbin) + " > "+testoutput + " || { cat "+testoutput+" ; exit 1; }",
                    '@date >> ' + testflag ],
                       "Testing        "+str(testbin))
            ))

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

    # Autogenerated ones
    auto_srcs = Glob("obj/" + suffix + "/" + name + "/*.cpp")
    for j in auto_srcs:
        obj = os.path.splitext(str(j))[0] + ".o"
        env.StaticObject(obj, j)
        objs.append(obj)

    bin = env.Program(target = "obj/"+suffix+"/bin/"+name,
                      source = objs,
                      LIBS = libs + env["LIBS"])

# Libraries listed in dependency order
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
        "ui",
        "output",

        "dbreceiver",
        "dblocal",

        "uiqt",
        "upnpd",
        "receiverd",
        "mediatree",
]:
    ChoraleLib(i)

for i in [
        "choraled",
        "choralecd",
]:
    ChoraleBin(i)

# SCons TODO list
#
# - fix remaining imagery (gif and ico)
# - lcov on profile build
# - all the metrics (topten etc.)
# - build binaries again
# - installer
# - release packaging
