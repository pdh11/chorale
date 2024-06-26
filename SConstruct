# SConstruct for Chorale
#
# Options:
#    PREFIX=<dir>  -- set install prefix (default /usr/local)
#    WERROR=1      -- compile everything with -Werror
#    PROFILE=1     -- do profiling build
#    DEBUG=0       -- do release build
#    SINGLE=1      -- force non-parallel build
#    UBSAN=1       -- enable undefined-behaviour sanitiser
#    ASAN=1        -- enable address sanitiser
#    TIDY=1        -- enable clang-tidy linter
#    VALGRIND=1    -- in debug buidls, enable valgrind
#    CC=<cmd>      -- set C compiler (default gcc)
#    CXX=<cmd>     -- set C++ compiler (default g++)
#
# Dependencies:
#    inkscape, xsltproc, graphviz, lcov, pkg-config;
#    libboost-dev, libflac-dev, libgstreamer1.0-dev, libcdparanoia-dev,
#    libtag1-dev, libcddb-dev, libmpg123-dev, liblame-dev, libavformat-dev,
#    libwrap0-dev, qtbase5-dev.
#
PACKAGE = "chorale"
PACKAGE_WEBSITE = "https://github.com/pdh11/chorale"

import subprocess
import os
import pathlib
if not ARGUMENTS.get('SINGLE', 0):
    SetOption('num_jobs', os.cpu_count())

try:
    # In a tarball, there's a version string
    PACKAGE_VERSION = pathlib.Path(".version").read_text().strip()
except FileNotFoundError:
    # In a git checkout, ask git
    raw_version = subprocess.check_output(['git', 'describe', '--tags'])
    PACKAGE_VERSION = raw_version.decode('utf-8').strip()

### CONFIGURY

def CheckCFlag(ctx, flag):
    ctx.Message("Checking whether "+ctx.env["CC"]+" accepts "+flag + "... ")
    old_CFLAGS=ctx.env["CFLAGS"]
    ctx.env.Append(CFLAGS=[flag,"-Werror"])
    ok = ctx.TryCompile("", ".c")
    ctx.env["CFLAGS"] = old_CFLAGS
    if ok:
        ctx.env.Append(CFLAGS=flag)
    ctx.Result(ok)
    return ok

def CheckCXXFlag(ctx, flag):
    ctx.Message("Checking whether "+ctx.env["CXX"]+" accepts "+flag + "... ")
    old_CXXFLAGS=ctx.env["CXXFLAGS"]
    ctx.env.Append(CXXFLAGS=[flag, "-Werror"])
    ok = ctx.TryCompile("", ".cpp")
    ctx.env["CXXFLAGS"] = old_CXXFLAGS
    if ok:
        ctx.env.Append(CXXFLAGS=flag)
    ctx.Result(ok)
    return ok

env = Environment()
if "CC" in ARGUMENTS:
    env["CC"] = ARGUMENTS["CC"]
if "CXX" in ARGUMENTS:
    env["CXX"] = ARGUMENTS["CXX"]
if not env.GetOption('clean'):
    PREFIX = ARGUMENTS.get("PREFIX", "/usr/local")
    print("Compiling Chorale "+PACKAGE_VERSION+" to install in PREFIX="+PREFIX)
    try:
        os.mkdir("obj")
    except FileExistsError:
        pass
    conf = env.Configure(config_h = "config.h",
                         log_file = "obj/config.log",
                         custom_tests={'CheckCXXFlag': CheckCXXFlag,
                                       'CheckCFlag': CheckCFlag})
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
            "unistd.h",
            "pthread.h",
            "sys/disk.h",
            "sys/time.h",
            "sys/poll.h",
            "sys/types.h",
            "sys/socket.h",
            "netinet/ip.h",
            "sys/syslog.h",
            "sys/utsname.h",
            "linux/cdrom.h",
            "linux/unistd.h",
            "sys/resource.h",
            "linux/dvb/dmx.h",
            "linux/dvb/frontend.h",
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
    for l in [
            "LOG_ERR",
            "LOG_NOTICE",
            "LOG_WARNING"]:
        conf.CheckDeclaration(l, "#include <sys/syslog.h>")
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
    conf.CheckLib(["mp3lame"])
    conf.CheckLib(["cdda_paranoia"])
    conf.CheckLib(["cdda_interface"])

    # Set compiler flags -- do this AFTER checks above, which fail with
    # warnings enabled
    if not conf.CheckCFlag("-std=gnu23"):
        if not conf.CheckCFlag("-std=gnu18"):
            if not conf.CheckCFlag("-std=gnu17"):
                if not conf.CheckCFlag("-std=gnu11"):
                    conf.CheckCFlag("-std=gnu1x")
    for flag in [
            "-W",
            "-Wall",
            "-fPIC",
            "-Wextra",
            "-Wundef",
            "-Wshadow",
            "-Wswitch",
            "-Waddress",
            "-Wcoercion",
            "-Wcast-align",
            "-Wconversion",
            "-Wwrite-strings",
            "-Wpointer-arith",
            "-Wbad-function-cast",
            "-Wstrict-prototypes",
            "-Wmissing-prototypes",
            "-pedantic -Wno-long-long",
            "-fdiagnostics-show-option",
            "-Wdeclaration-after-statement",
            "-Wno-unused-command-line-argument",
    ]:
        conf.CheckCFlag(flag)
    if not conf.CheckCXXFlag("-std=gnu++23"):
        if not conf.CheckCXXFlag("-std=gnu++20"):
            if not conf.CheckCXXFlag("-std=gnu++17"):
                if not conf.CheckCXXFlag("-std=gnu++14"):
                    if not conf.CheckCXXFlag("-std=gnu++11"):
                        conf.CheckCXXFlag("-std=gnu++1x")
    for flag in [
            "-W",
            "-Wall",
            "-fPIC",
            "-Wextra",
            "-Wundef",
            "-Wshadow",
            "-pedantic",
            "-Wnoexcept",
            "-Wcast-qual",
            "-Wlogical-op",
            "-Wcast-align",
            #"-Wconversion",
            "-Wc++0x-compat",
            "-Wc++11-compat",
            "-Wc++14-compat",
            "-Wc++17-compat",
            "-Wc++20-compat",
            "-fstrict-enums",
            "-Wno-long-long",
            "-fno-exceptions",
            "-Wwrite-strings",
            "-Wpointer-arith",
            "-Wnon-virtual-dtor",
            "-Wno-system-headers",
            "-Woverloaded-virtual",
            "-Wno-sign-conversion",
            "-Wmissing-declarations",
            "-Wunused-but-set-variable",
            "-fdiagnostics-show-option",
            "-Wno-unused-command-line-argument",
    ]:
        conf.CheckCXXFlag(flag)
    if ARGUMENTS.get("WERROR", 0):
        conf.CheckCFlag("-Werror")
        conf.CheckCXXFlag("-Werror")
    for v in range(20,14,-1):
        clangtidy = conf.CheckProg("clang-tidy-"+str(v))
        if clangtidy:
            break
    if not clangtidy:
        clangtidy = conf.CheckProg("clang-tidy")

    # Changed in glibc 2.10, we don't care about earlier any more
    conf.Define("SCANDIR_COMPARATOR_ARG_T", "const struct dirent**")
    conf.Define("HAVE_CONDITION_TIMED_WAIT_INTERVAL", 1)
    conf.Define("HAVE_LAME", 1)
    conf.Define("HAVE_LAME_GET_LAMETAG_FRAME", 1)
    conf.Define("HAVE_TAGLIB", 1)
    conf.Define("HAVE_MPG123", 1)
    conf.Define("HAVE_TCP_CORK", 1)
    conf.Define("HAVE_LIBCDDB", 1)
    conf.Define("HAVE_LIBFLAC", 1)
    conf.Define("HAVE_PARANOIA", 1)
    conf.Define("HAVE_AVFORMAT", 1)
    conf.Define("HAVE_GSTREAMER", 1)
    conf.Define("HAVE_DECL_PARANOIA_CB_CACHEERR", 1)
    conf.Define("HAVE_IP_PKTINFO", "HAVE_DECL_IP_PKTINFO")
    conf.Define("HAVE_DVB", "(HAVE_LINUX_DVB_DMX_H && HAVE_LINUX_DVB_FRONTEND_H)")
    conf.Define("PACKAGE_NAME", '"'+PACKAGE+'"')
    conf.Define("PACKAGE_WEBSITE", '"'+PACKAGE_WEBSITE+'"')
    conf.Define("LOCALSTATEDIR", '"'+PREFIX+'/var"')
    conf.Define("CHORALE_DATADIR", '"'+PREFIX+'/share"')
    conf.Define("SRCROOT", '"' + Dir(".").path + '"')
    env = conf.Finish()
    # Do version separately so everything doesn't recompile when it changes
    conf = env.Configure(config_h = "version.h", log_file = "obj/version.log")
    conf.Define("PACKAGE_VERSION", '"'+PACKAGE_VERSION+'"')
    conf.Define("PACKAGE_STRING", 'PACKAGE_NAME " " PACKAGE_VERSION');
    conf.Finish()
    env.MergeFlags("!pkg-config --libs taglib libcddb libavformat flac libmpg123 gstreamer-1.0")
    env.MergeFlags("!pkg-config --libs Qt5Gui Qt5Core Qt5Widgets")

    # Convert "-I" to "-isystem" otherwise -Wno-system-headers doesn't work (?)
    flags = env.ParseFlags(
        # Not the flac ones
        "!pkg-config --cflags taglib libcddb libavformat gstreamer-1.0",
        "!pkg-config --cflags Qt5Gui Qt5Core Qt5Widgets")
    for f in flags["CPPPATH"]:
        env.Append(CCFLAGS = ["-isystem", f])
    env.Append(CPPPATH=["."])
    env.Append(LINKFLAGS=["-Wl,--as-needed","-Wl,-Map,${TARGET}.map"])
    if clangtidy:
        env["CLANGTIDY"] = clangtidy

maybe_valgrind = ""
debug = ARGUMENTS.get('DEBUG', 1)
profile = ARGUMENTS.get('PROFILE', 0)
if int(profile):
    suffix = "profile"
    env.Append(CCFLAGS=["-fprofile-arcs", "-ftest-coverage", "-DDEBUG=0",
                        "-DNDEBUG", "-O2"])
    env.Append(LINKFLAGS=["-coverage"])
    env.Append(LIBS=["-lgcov"])
elif int(debug):
    if ARGUMENTS.get('VALGRIND', 0):
        suffix = "valgrind"
        env.Append(CCFLAGS=["-g", "-O1", "-DDEBUG=1"])
        maybe_valgrind = "valgrind --leak-check=yes --error-exitcode=42 "
    else:
        suffix = "debug"
        env.Append(CCFLAGS=["-g", "-O2", "-DDEBUG=1"])
else:
    suffix = "release"
    env.Append(CCFLAGS=["-DDEBUG=0", "-O2", "-Os", "-DNDEBUG"])
if ARGUMENTS.get('ASAN', 0):
    env.Append(CCFLAGS=["-fsanitize=address","-fno-sanitize-recover=all"])
    env["LINK"] = env["CXX"]
    env.Append(LINKFLAGS=["-fsanitize=address"])
    suffix += "-asan"
if ARGUMENTS.get('UBSAN', 0):
    # "undefined,integer" is appealing but Boost breaks it all the time
    env.Append(CCFLAGS=["-fsanitize=undefined","-fno-sanitize-recover=all"])
    env["LINK"] = env["CXX"]
    env.Append(LINKFLAGS=["-fsanitize=undefined"])
    suffix += "-ubsan"
tidy = ARGUMENTS.get('TIDY', 0)
env.Append(CPPPATH="#obj/"+suffix)

if ARGUMENTS.get('VERBOSE') != "1":
    env["ARCOMSTR"]      = "Library        ${TARGET}"
    env["RANLIBCOMSTR"]  = "Ranlib         ${TARGET}"
    env["LINKCOMSTR"]    = "Linking        ${TARGET}"
    env["TESTSTR"]       = "Testing        ${SOURCE}"
    env["LINTSTR"]       = "Linting        ${SOURCE}"
    env["CREATESTR"]     = "Creating       ${TARGET}"
    env["CCCOMSTR"]      = "Compiling      ${SOURCE}"
    env["CXXCOMSTR"]     = "Compiling      ${SOURCE}"
testenv = env.Clone()
testenv.Append(CCFLAGS=["-DTEST", "-UNDEBUG"])
if ARGUMENTS.get('VERBOSE') != "1":
    testenv["CCOMSTR"]   = "Compiling test ${SOURCE}"
    testenv["CXXCOMSTR"] = "Compiling test ${SOURCE}"

#### AUTOGENERATED SOURCES

def XSLT(service, stylesheet, output):
    src = "libupnp/" + service + ".xml"
    env.Command(output, [src, stylesheet],
                Action(["xsltproc --stringparam class " + service + " -o " + output + " " + stylesheet + " " + src],
                       env.get("CREATESTR", "")))

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
                Action(["xsltproc --stringparam class " + service + " --stringparam mode serverheader -o " + out + " libupnp/scpd.xsl libupnp/"+ i],
                       env.get("CREATESTR", "")))

# Autogenerated source files from Qt meta-object-compiler
for i in [
        "libchoralecd/browse_widget.h",
        "libchoralecd/cd_progress.h",
        "libchoralecd/cd_widget.h",
        "libchoralecd/cd_window.h",
        "libchoralecd/db_widget.h",
        "libchoralecd/explorer_window.h",
        "libchoralecd/main_window.h",
        "libchoralecd/output_widget.h",
        "libchoralecd/resource_widget.h",
        "libchoralecd/setlist_model.h",
        "libchoralecd/setlist_window.h",
        "libchoralecd/settings_entry.h",
        "libchoralecd/settings_window.h",
        "libchoralecd/tag_editor_widget.h",
        "libchoralecd/tree_model.h",
        "libuiqt/scheduler.h",
        ]:
    out = "obj/" + suffix + "/" + os.path.splitext(i)[0] + ".moc.cpp"
    env.Command(out, i, Action(["moc " + i + " -o " + out],
                               env.get("CREATESTR", "")))

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
                    "inkscape --export-png-color-mode=RGBA_8 --export-filename="+png+" --export-area="+area+" -w "+str(w)+" -h "+str(h) + " ${SOURCE} > /dev/null 2>&1"],
                       env.get("CREATESTR", "")))
    xpm = "obj/" + suffix + "/imagery/" + i + ".xpm"
    xpm0 = xpm + ".0.xpm"
    env.Command(xpm, png,
                Action([
                    "convert -transparent white ${SOURCE} "+xpm0 + ";" \
                    + 'sed -e "s/static.*\[]/static const char *const '+i+'_xpm[]/" < '+xpm0 + " > " + xpm + ";" \
                    'rm '+xpm0],
                       env.get("CREATESTR", "")))

# The gifs
for base in [16,32,48]:
    png = "obj/" + suffix + "/imagery/icon"+str(base)+".png"
    gif = "obj/" + suffix + "/imagery/icon"+str(base)
    env.Command(gif + "_16.gif", png,
                Action([
                    "convert -colors 16 ${SOURCE} ${TARGET}"],
                       env.get("CREATESTR", "")))
    env.Command(gif + "_256.gif", png,
                Action([
                    "convert -map netscape: ${SOURCE} ${TARGET}"],
                       env.get("CREATESTR", "")))

# The icon (for favicon.ico)
ico = "obj/" + suffix + "/imagery/icon.ico"
base = "obj/" + suffix + "/imagery/icon"
env.Command(ico,
            [base + "16.png", base + "32.png", base + "48.png",
             base + "16_16.gif", base + "16_256.gif",
             base + "32_16.gif", base + "32_256.gif",
             base + "48_16.gif", base + "48_256.gif"],
            Action([
                "convert ${SOURCES} ${TARGET}"
                ],
                   env.get("CREATESTR", "")))

libs = []
lcovs = []

#### LIB AND BIN BUILDS

def ChoraleLib(name):
    global libs
    srcdir = "lib"+name
    objdir = "obj/"+suffix+"/lib"+name
    srcs = Glob(srcdir + "/*.cpp") + Glob(srcdir + "/*.c")
    objs = []
    for j in srcs:
        obj = "#obj/"+suffix+"/"+os.path.splitext(str(j))[0]+".o"
        env.StaticObject(obj, j)
        objs.append(obj)

        tidied = "#obj/"+suffix+"/tidied/" + str(j)
        if tidy and env["CLANGTIDY"]:
            env.Command(tidied, [j, ".clang-tidy"],
                        Action([
                            "${CLANGTIDY} --config-file=.clang-tidy ${SOURCE} -- ${CCFLAGS} -I. -Iobj/"+suffix+" && touch ${TARGET}" ],
                               env.get("LINTSTR", "")))

    # Autogenerated ones
    auto_srcs = Glob(objdir + "/*.cpp")
    for j in auto_srcs:
        obj = os.path.splitext(str(j))[0] + ".o"
        env.StaticObject(obj, j)
        objs.append(obj)

    lib0 = env.StaticLibrary(target=objdir+"/lib"+name+"-untested.a",
                             source=objs)

    tests = []
    tested = []
    testobjs = []
    for j in srcs:
        testbin = "obj/"+suffix+"/"+os.path.splitext(str(j))[0]+".test"
        testobj = testbin + ".o"
        testenv.StaticObject(target = testobj, source = j)
        testflag = testbin + ".tested"
        testoutput = testbin + ".output"
        test = testenv.Program(target=testbin, source=[testobj, lib0],
                               LIBS=libs + env["LIBS"])
        tests.append(test)
        testobjs.append(testobj)
        tested.append(
            env.Command(
                testflag,
                testbin,
                Action([
                    maybe_valgrind + str(testbin) + " > "+testoutput + " || { cat "+testoutput+" ; exit 1; } ;" \
                    "date >> " + testflag ],
                       env.get("TESTSTR", ""))
            ))

    if profile:
        zerofile = objdir + "/zero.info"
        infofile = objdir + "/lcov.info"
        env.Command(zerofile, objs + testobjs,
                    Action(['lcov -q --rc "lcov_branch_coverage=1" -c -i -d '+objdir+' -o '+zerofile],
                           env.get("CREATESTR", "")))
        for t in tests:
            env.Depends(t, zerofile)
        env.Command(infofile, tested,
                    Action([
                    'lcov -q --rc "lcov_branch_coverage=1" -c -d '+objdir+' -o '+objdir+'/test.info ; ' + \
                    'lcov -q --rc "lcov_branch_coverage=1" -a '+objdir+'/zero.info -a '+objdir+'/test.info -o '+objdir+'/lcov0.info ; ' + \
                    'lcov -q --rc "lcov_branch_coverage=1" -e '+objdir+'/lcov0.info \'*/lib'+name+'/*\' -o '+infofile
                    ],env.get("CREATESTR", "")))
        lcovs.append(infofile)

    lib = env.StaticLibrary(target="#obj/"+suffix+"/lib"+name+".a", source=objs)
    env.Depends(lib, tested)
    libs = [lib] + libs

    bins = Glob("lib"+name+"/bin/*.cpp")
    for j in bins:
        name = os.path.splitext(os.path.split(str(j))[1])[0]
        obj = "#obj/"+suffix+"/"+os.path.splitext(str(j))[0] + ".o"
        env.StaticObject(target = obj, source = j)
        bin = "#obj/"+suffix+"/bin/"+name
        env.Program(target=bin, source=obj,
                    LIBS = libs + env["LIBS"])

        tidied = "#obj/"+suffix+"/tidied/" + str(j)
        if tidy and env["CLANGTIDY"]:
            env.Command(tidied, [j, ".clang-tidy"],
                        Action([
                                "${CLANGTIDY} --config-file=.clang-tidy ${SOURCE} -- ${CCFLAGS} -I. -Iobj/"+suffix+" && touch ${TARGET}" ],
                                   env.get("LINTSTR", "")))

def ChoraleBin(name):
    srcs = Glob(name + "/*.cpp")
    if srcs:
        objs = []
        for j in srcs:
            obj = "#obj/"+suffix+"/"+os.path.splitext(str(j))[0]+".o"
            env.StaticObject(obj, j)
            objs.append(obj)

            tidied = "#obj/"+suffix+"/tidied/" + str(j)
            if tidy and env["CLANGTIDY"]:
                env.Command(tidied, [j, ".clang-tidy"],
                            Action([
                                "${CLANGTIDY} --config-file=.clang-tidy ${SOURCE} -- ${CCFLAGS} -I. -Iobj/"+suffix+" && touch ${TARGET}" ],
                                   env.get("LINTSTR", "")))

        # Autogenerated ones
        auto_srcs = Glob("obj/" + suffix + "/" + name + "/*.cpp")
        for j in auto_srcs:
            obj = os.path.splitext(str(j))[0] + ".o"
            env.StaticObject(obj, j)
            objs.append(obj)

        bin = env.Program(target = "obj/"+suffix+"/bin/"+name,
                          source = objs,
                          LIBS = libs + env["LIBS"])

    bins = Glob(name+"/bin/*.cpp")
    for j in bins:
        name = os.path.splitext(os.path.split(str(j))[1])[0]
        obj = "#obj/"+suffix+"/"+os.path.splitext(str(j))[0] + ".o"
        env.StaticObject(target = obj, source = j)
        bin = "#obj/"+suffix+"/bin/"+name
        env.Program(target=bin, source=obj,
                    LIBS = libs + env["LIBS"])

        tidied = "#obj/"+suffix+"/tidied/" + str(j)
        if tidy and env["CLANGTIDY"]:
            env.Command(tidied, [j, ".clang-tidy"],
                        Action([
                                "${CLANGTIDY} --config-file=.clang-tidy ${SOURCE} -- ${CCFLAGS} -I. -Iobj/"+suffix+" && touch ${TARGET}" ],
                                   env.get("LINTSTR", "")))

# Libraries listed in dependency order
LIBDIRS = [
        "util",

        "db",
        "empeg",

        "dbsteam",
        "mediadb",
        "upnp",

        "dbupnp",
        "dbempeg",
        "import",
        "receiver",
        "dbmerge",
        "output",
        "tv",

        "dbreceiver",
        "dblocal",

        "uiqt",
        "upnpd",
        "receiverd",
        "mediatree",

    "choralecd"
]

for i in LIBDIRS:
    ChoraleLib(i)

BINDIRS= [
        "choraled",
        "choraleutil",
]

for i in BINDIRS:
    ChoraleBin(i)

if profile:
    infofile = "obj/profile/lcov.info"
    htmldir = "obj/lcov"
    env.Command(infofile,
                    lcovs,
                    [ 'lcov -q --rc "lcov_branch_coverage=1"' + ''.join([' -a '+str(a) for a in lcovs]) + ' -o '+infofile,
                      'genhtml -q  --rc "lcov_branch_coverage=1" -p `pwd` -o '+htmldir+' '+infofile])

#### METRICS

metrics = "obj/metrics/"
SUBDIRS = BINDIRS + ["lib"+lib for lib in LIBDIRS]
env.Command(metrics+"libdeps.dot", ["SConstruct", "scripts/make-libdeps"],
            Action(["scripts/make-libdeps "+" ".join(SUBDIRS)+" > ${TARGET}"],
                   env.get("CREATESTR", "")))
env.Command(metrics+"filedeps.dot", ["SConstruct", "scripts/make-filedeps"],
            Action(["scripts/make-filedeps "+" ".join(SUBDIRS)+" > ${TARGET}"],
                   env.get("CREATESTR", "")))
env.Command(metrics+"filedeps-all.dot",
            [metrics+"filedeps.dot", "SConstruct", "scripts/transitive-closure"],
            Action(["scripts/transitive-closure < ${SOURCE} > ${TARGET}"],
                   env.get("CREATESTR", "")))
env.Command(metrics+"fan.txt",
            ["SConstruct", "scripts/fan", metrics+"filedeps-all.dot"],
            Action(["scripts/fan "+metrics+"filedeps-all.dot > ${TARGET}"],
                   env.get("CREATESTR", "")))
env.Command(metrics+"compdeps.dot", [metrics+"filedeps.dot", "SConstruct"],
            Action(["	sed -e 's/[.]cpp//g' -e 's/[.]h//g' -e 's/_posix//g'  -e 's/_linux//g' -e 's/_win32//g' \
	    -e 's,libupnp/\([A-Z][a-zA-Z0-9]*\)_client,libupnp/\\1,g' \
	    -e 's,libupnp/\([A-Z][a-zA-Z0-9]*\)_server,libupnp/\\1,g' \
            -e 's,^\\(.*\\) -> \\1$,,g' \
	    -e 's/_internal//g' < ${SOURCE} > ${TARGET}"],
                   env.get("CREATESTR", "")))
env.Command(metrics+"compdeps2.dot", [metrics+"compdeps.dot", "scripts/make-compdeps2"],
            Action(["scripts/make-compdeps2 ${SOURCE} "+" ".join(SUBDIRS)+" > ${TARGET}"],
                   env.get("CREATESTR", "")))
env.Command(metrics+"cycles.png", [metrics+"compdeps.dot", "scripts/make-cycles"],
            Action(["scripts/make-cycles ${SOURCE} ${TARGET}"],
                   env.get("CREATESTR", "")))

DOTS=["libdeps", "filedeps", "compdeps", "compdeps2"]
for dot in DOTS:
    env.Command(metrics+dot+".svg", metrics+dot+".dot",
                Action(['tred ${SOURCE} | dot -Tsvg -Nfontname="Luxi Sans" -Gfontnames=gd -o ${TARGET}'],
                       env.get("CREATESTR", "")))
    env.Command(metrics+dot+".png", metrics+dot+".svg",
                Action(['inkscape -b white -y 1.0 --export-filename=${TARGET} ${SOURCE} > /dev/null 2>&1'],
                       env.get("CREATESTR", "")))

#### INSTALL

env.Alias("install", PREFIX)
built = "obj/"+suffix+"/"
for (src,dest) in [
        ("libupnp/AVTransport.xml", "share/chorale/upnp"),
        ("libupnp/RenderingControl.xml", "share/chorale/upnp"),
        ("libupnp/ContentDirectory.xml", "share/chorale/upnp"),
        ("libupnp/ConnectionManager.xml", "share/chorale/upnp"),
        ("libupnp/OpticalDrive.xml", "share/chorale/upnp"),
        ("imagery/default.css", "share/chorale/layout"),
        ("imagery/search-amazon.png", "share/chorale/layout"),
        ("imagery/search-imdb.png", "share/chorale/layout"),
        ("imagery/search-google.png", "share/chorale/layout"),
        ("imagery/search-wikipedia.png", "share/chorale/layout"),
        (built+"imagery/icon32.png", "share/chorale/layout"),
        (built+"imagery/icon32s.png", "share/chorale/layout"),
        (built+"imagery/icon.ico", "share/chorale/layout"),
        (built+"imagery/noart32.png", "share/chorale/layout"),
        (built+"imagery/noart48.png", "share/chorale/layout"),
        (built+"imagery/icon16.png", "share/icons/hicolor/16x16/apps"),
        (built+"imagery/icon32.png", "share/icons/hicolor/32x32/apps"),
        (built+"imagery/icon48.png", "share/icons/hicolor/48x48/apps"),
        (built+"bin/choraled", "bin"),
        (built+"bin/choralecd", "bin"),
        (built+"bin/protocoltool", "bin"),
        (built+"bin/tageditor", "bin"),
        ("choraleutil/protocoltool.1", "man/man1"),
        ("README", "var"),
        ]:
    env.Install(PREFIX+"/"+dest, src)
