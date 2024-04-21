PACKAGE="chorale"
PACKAGE_VERSION="0.21"

import os
SetOption('num_jobs', os.cpu_count())

env = Environment()
if not env.GetOption('clean'):
    conf = env.Configure(config_h = "config.h")
    for header in [
            "sys/utsname.h",
            "net/if.h",
            "net/if_dl.h",
            "sys/types.h",
            "sys/socket.h",
            "netinet/ip.h",
    ]:
        conf.CheckHeader(header)
    for f in [
            "gettid",
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
    conf.Define("HAVE_VASPRINTF", 1)
    conf.Define("HAVE_STDINT_H", 1)
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
    env["ARCOMSTR"] = "Gathering ${TARGET.file}"
    env["CCCOMSTR"] = "Compiling ${SOURCE}"
    env["CXXCOMSTR"] = "Compiling ${SOURCE}"

libs = []

def ChoraleLib(name):
    objs = []
    for j in Glob("lib+"+ name + "/*.cpp"):
        obj = "#obj/"+suffix+"/lib"+str(j)[:-4]+".o"
        env.StaticObject(obj, j)
        objs.append(obj)
    env.StaticLibrary(target="#obj/"+suffix+"/lib"+name+".a", source=objs)
    libs.append("-l"+name)

for i in ["util"]:
    ChoraleLib(i)
