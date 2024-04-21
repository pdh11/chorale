PACKAGE="chorale"
PACKAGE_VERSION="0.21"

import subprocess
import os
SetOption('num_jobs', os.cpu_count())

env = Environment()
if not env.GetOption('clean'):
    conf = env.Configure(config_h = "config.h")
    conf.CheckLib('wrap')
    conf.CheckLib('boost_system')
    conf.CheckLib('boost_thread')
    conf.CheckLib('pthread')
    for header in [
            "poll.h",
            "sched.h",
            "stdint.h",
            "sys/poll.h",
            "sys/utsname.h",
            "sys/resource.h",
            "net/if.h",
            "net/if_dl.h",
            "sys/types.h",
            "sys/socket.h",
            "netinet/ip.h",
    ]:
        conf.CheckHeader(header)
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
    conf.Define("HAVE_IP_PKTINFO", "HAVE_DECL_IP_PKTINFO")
    conf.Define("PACKAGE_NAME", '"'+PACKAGE+'"')
    conf.Define("PACKAGE_VERSION", '"'+PACKAGE_VERSION+'"')
    env = conf.Finish()
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

libs = []

def ChoraleLib(name):
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
        test = testenv.Program(target=testbin, source=[testobj, lib0])
        tests.append(test)
        tested.append(env.Command(testflag, test,
                                  [ "@echo Testing "+str(j),
                                    "@" + str(testbin) + " > "+testoutput + " || { cat "+testoutput+" ; exit 1; }",
                                    '@date >> ' + testflag ]))

    lib = env.StaticLibrary(target="#obj/"+suffix+"/lib"+name+".a", source=objs)
    env.Depends(lib, tested)
    libs.append("-l"+name)

for i in ["util"]:
    ChoraleLib(i)
