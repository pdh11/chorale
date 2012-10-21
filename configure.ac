#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(chorale, [0.14], pdh@utter.chaos.org.uk)
AC_SUBST(PACKAGE_WEBSITE, [http://chorale.sf.net])
AC_DEFINE_UNQUOTED([PACKAGE_WEBSITE], ["${PACKAGE_WEBSITE}"], [Define to the package's website])

AC_MSG_NOTICE([Required: Boost.])
AC_MSG_NOTICE([Optional: Taglib, Qt4, gstreamer, cdparanoia or libcdio, libcddb, lame, flac, dbus, hal, libmpg123, inkscape, xsltproc, imagemagick.])
AC_MSG_NOTICE([])

AC_CONFIG_SRCDIR([libdb/db.h])
AC_CONFIG_AUX_DIR([autotools])

# Checks for programs.
AC_PROG_CC
AC_GNU_SOURCE
AC_PROG_LIBTOOL
AC_C_BIGENDIAN

AC_DEFUN([PDH_LD_WARNING],
[
    AC_MSG_CHECKING([whether linker ${CC} accepts $1])

    ok_LDFLAGS="$LDFLAGS"
    LDFLAGS="$LDFLAGS $1"
    AC_LINK_IFELSE([AC_LANG_PROGRAM()],
		   [AC_MSG_RESULT([yes])],
		   [AC_MSG_RESULT([no]); LDFLAGS=$ok_LDFLAGS])
])

PDH_LD_WARNING([-mthreads])
PDH_LD_WARNING([-Wl,--as-needed])

AC_DEFUN([PDH_C_WARNING],
[
    AC_MSG_CHECKING([whether ${CC} accepts $1])

    ok_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS $1"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM()],
		      [AC_MSG_RESULT([yes])],
		      [AC_MSG_RESULT([no]); CFLAGS=$ok_CFLAGS])
])

PDH_C_WARNING([-W])
PDH_C_WARNING([-Wall])
PDH_C_WARNING([-Wextra])
PDH_C_WARNING([-Wundef])
PDH_C_WARNING([-Wshadow])
PDH_C_WARNING([-Waddress])
PDH_C_WARNING([-Wcoercion])
PDH_C_WARNING([-Wcast-align])
PDH_C_WARNING([-Wconversion])
PDH_C_WARNING([-Wwrite-strings])
PDH_C_WARNING([-Wpointer-arith])
PDH_C_WARNING([-Wbad-function-cast])
PDH_C_WARNING([-Wstrict-prototypes])
PDH_C_WARNING([-Wmissing-prototypes])
PDH_C_WARNING([-pedantic -Wno-long-long])
PDH_C_WARNING([-fdiagnostics-show-option])
PDH_C_WARNING([-Wdeclaration-after-statement])

AC_PROG_CXX
AC_CHECK_TOOL([GCOV], [gcov], [:])

AC_DEFUN([PDH_CXX_WARNING],
[
    AC_MSG_CHECKING([whether ${CXX} accepts $1])

    AC_LANG_PUSH([C++])

    ok_CXXFLAGS="$CXXFLAGS"
    CXXFLAGS="$CXXFLAGS $1"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM()],
		      [AC_MSG_RESULT([yes])],
		      [AC_MSG_RESULT([no]); CXXFLAGS=$ok_CXXFLAGS])

    AC_LANG_POP
])

PDH_CXX_WARNING([-W])
PDH_CXX_WARNING([-Wall])
PDH_CXX_WARNING([-Wextra])
PDH_CXX_WARNING([-Wundef])
PDH_CXX_WARNING([-Wshadow])
PDH_CXX_WARNING([-mthreads])
PDH_CXX_WARNING([-Waddress])
PDH_CXX_WARNING([-Wlogical-op])
PDH_CXX_WARNING([-Wcast-align])
PDH_CXX_WARNING([-Wconversion])
PDH_CXX_WARNING([-Wwrite-strings])
PDH_CXX_WARNING([-Wpointer-arith])
#PDH_CXX_WARNING([-Wsign-conversion])
#PDH_CXX_WARNING([-Wstrict-overflow])
PDH_CXX_WARNING([-Wnon-virtual-dtor])
PDH_CXX_WARNING([-Wstrict-overflow=5])
PDH_CXX_WARNING([-Woverloaded-virtual])
PDH_CXX_WARNING([-pedantic -Wno-long-long])
PDH_CXX_WARNING([-fdiagnostics-show-option])

AC_MSG_CHECKING([whether headers need -fpermissive])
AC_LANG_PUSH([C++])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <linux/cdrom.h>
]],
[[
    return 0;
]])], [AC_MSG_RESULT([no]) ; ac_pdh_fperm=no],
      [AC_MSG_RESULT([yes]) ; ac_pdh_fperm=yes])
AC_LANG_POP

if test "$ac_pdh_fperm" = "yes"; then
PDH_CXX_WARNING([-fpermissive])
fi

AC_PROG_INSTALL
AC_SYS_LARGEFILE
if test "x$ac_cv_sys_file_offset_bits" != "x" -a "x$ac_cv_sys_file_offset_bits" != "xno" -a "x$ac_cv_sys_file_offset_bits" != "xunknown" ; then
  LARGEFILES=-D_FILE_OFFSET_BITS=$ac_cv_sys_file_offset_bits
fi
if test "x$ac_cv_sys_large_files" != "x"; then
  LARGEFILES=-D_LARGE_FILES=$ac_cv_sys_large_files
fi
AC_SUBST(LARGEFILES)
AC_PATH_PROG(CONVERT, convert, no)
if test "$CONVERT" = "no"; then
   HAVE_CONVERT="no"
else
   HAVE_CONVERT="yes"
fi
AC_PATH_PROG(XSLTPROC, xsltproc, no)
if test "$XSLTPROC" = "no"; then
   HAVE_XSLTPROC="no"
else
   HAVE_XSLTPROC="yes"
fi
AC_PATH_PROG(INKSCAPE, inkscape, no)
if test "$INKSCAPE" = "no"; then
   HAVE_INKSCAPE="no"
else
   HAVE_INKSCAPE="yes"
fi
AC_PATH_PROG(VALGRIND, valgrind, no)
VALGRIND_OPTS=""
if test "$VALGRIND" = "no"; then
   HAVE_VALGRIND="no"
else
   HAVE_VALGRIND="yes"
   if valgrind --help | grep -q -e --read-var-info ; then
	VALGRIND_OPTS="$VALGRIND_OPTS --read-var-info=yes"
   fi
   if valgrind --help | grep -q -e --track-origins ; then
	VALGRIND_OPTS="$VALGRIND_OPTS --track-origins=yes"
   fi
fi
AC_SUBST(HAVE_CONVERT)
AC_SUBST(HAVE_XSLTPROC)
AC_SUBST(HAVE_INKSCAPE)
AC_SUBST(HAVE_VALGRIND)
AC_SUBST(VALGRIND_OPTS)


# Checks for libraries.

AC_ARG_WITH([gpl],
[AS_HELP_STRING([--without-gpl],[Avoid using GPL libraries (e.g. Qt 4.4)])],
[],[with_gpl=yes])
AC_ARG_WITH([lgpl],
[AS_HELP_STRING([--without-lgpl],[Avoid using GPL and LGPL libraries])],
[],[with_lgpl=yes])
AC_ARG_WITH([libwrap],
[AS_HELP_STRING([--without-libwrap],[Avoid using libwrap from tcp_wrappers])],
[],[with_libwrap=yes])

if test "x${with_lgpl}" = xno ; then
  with_gpl=no
fi

if test "x${with_libwrap}" != xno; then
AC_CHECK_HEADER([tcpd.h],[HAVE_LIBWRAP="yes"],[HAVE_LIBWRAP="no"])
if test "x${HAVE_LIBWRAP}" != xno; then
AC_MSG_CHECKING([for libwrap])
ok_LIBS=${LIBS}
LIBS="$LIBS -lwrap"
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <tcpd.h>
int allow_severity = 0;
int deny_severity = 0;
]],
[[
    return hosts_access();
]])], [AC_MSG_RESULT([yes]); HAVE_LIBWRAP="yes" ; LIBWRAP="-lwrap"],
      [AC_MSG_RESULT([no]) ; HAVE_LIBWRAP="no" ] )
# Set it back unconditionally; we don't want to link with -lwrap unless we
# have to.
LIBS=$ok_LIBS
fi
fi
AC_SUBST(LIBWRAP)
AC_SUBST(HAVE_LIBWRAP)
if test "x${HAVE_LIBWRAP}" != xno; then
AC_DEFINE(HAVE_LIBWRAP, 1, [Define to 1 if you have libwrap from tcp_wrappers])
fi

if test "x${with_lgpl}" != xno ; then
AC_CHECK_LIB(mp3lame, lame_init, [LAME="yes" LAME_LIBS="-lmp3lame"], [LAME="no"])
 if test "${LAME}" = "no" ; then
  # Sometimes (e.g. Debian 5.0) lame needs -lm but isn't linked against it, so
  # it fails to link with --as-needed. So we try it again with --no-as-needed.
  #
  # Test for a different symbol so result isn't cached
  AC_CHECK_LIB(mp3lame, lame_close, [LAME="yes" LAME_LIBS="-lmp3lame"], [LAME="no"],[-Wl,--no-as-needed -lm])
 fi
if test "${LAME}" = "yes" ; then
    AC_CHECK_HEADER(lame/lame.h, , [LAME="no"])
fi
AC_SUBST(LAME)
AC_SUBST(LAME_LIBS)
if test "${LAME}" = "yes" ; then
    AC_DEFINE(HAVE_LAME, 1, [Define to 1 if you have LAME])
fi
fi

if test "x${with_lgpl}" != xno ; then
AC_CHECK_LIB([cdda_paranoia],[paranoia_version], [PARANOIA="yes" PARANOIA_LIBS="-lcdda_paranoia"],[PARANOIA="no"])
if test "${PARANOIA}" = "yes" ; then
	AC_CHECK_HEADER([cdda_interface.h],,[PARANOIA="no"])
fi
if test "${PARANOIA}" = "yes" ; then
	AC_CHECK_HEADER([cdda_paranoia.h],,[PARANOIA="no"],[
AC_INCLUDES_DEFAULT()
#include <cdda_interface.h>
])
fi
AC_SUBST(PARANOIA)
AC_SUBST(PARANOIA_LIBS)
if test "${LAME}" = "yes" ; then
    AC_DEFINE(HAVE_PARANOIA, 1, [Define to 1 if you have `real' cdparanoia (not libcdio)])
fi
fi

PKG_PROG_PKG_CONFIG

XIPH_PATH_OGG()

# Qt 4.5.0+ is LGPL, earlier Qt is GPL
QT="no"
if test "x${with_lgpl}" != xno ; then
AC_MSG_CHECKING([for Qt])
PKG_CHECK_EXISTS([QtGui >= 4.5.0 Qt3Support >= 4.5.0 QtCore >= 4.5.0],
 [QT="yes"
  AC_MSG_RESULT([yes (LGPL)])
 ],[
  if test "x${with_gpl}" != xno ; then
    PKG_CHECK_EXISTS([QtGui >= 4.2.0 Qt3Support >= 4.2.0 QtCore >= 4.2.0],
     [QT="yes"
      AC_MSG_RESULT([yes (GPL)])
     ],[
      AC_MSG_RESULT([no])
     ])
   else
      AC_MSG_RESULT([no])
   fi
])

 if test "${QT}" = "yes" ; then
  QT_MOC=`pkg-config --variable=moc_location QtGui`
  QT_CFLAGS=`pkg-config --cflags QtGui Qt3Support QtCore`
  QT_LIBS=`pkg-config --libs QtGui Qt3Support QtCore`
  AC_SUBST(QT_MOC)
  AC_SUBST(QT_CFLAGS)
  AC_SUBST(QT_LIBS)
 fi
fi
AC_SUBST(QT)

AC_DEFUN([PDH_PKGCONFIG], [
AC_REQUIRE([PKG_PROG_PKG_CONFIG])
AC_MSG_CHECKING([for $2])
if AC_RUN_LOG([$PKG_CONFIG --exists --print-errors "$2"]); then
  if AC_RUN_LOG([$PKG_CONFIG --print-errors "$2 >= $3"]); then
    AC_MSG_RESULT([yes])
    $1_CFLAGS=`$PKG_CONFIG --cflags $2 2>/dev/null`
    $1_LIBS=`$PKG_CONFIG --libs $2 2>/dev/null`
    $1="yes"
    AC_DEFINE(HAVE_$1, 1, [Define to 1 if you have $2 >= $3])
  else
    pdh_result=`$PKG_CONFIG --modversion $2`
    AC_MSG_RESULT([no (have $pdh_result, need $3)])
    $1="no"
  fi
else
  AC_MSG_RESULT([no])
  $1="no"
fi
AC_SUBST($1)
AC_SUBST($1_CFLAGS)
AC_SUBST($1_LIBS)
])


PDH_PKGCONFIG(HAL,       hal,       0.5.9.1)
PDH_PKGCONFIG(LIBFLAC,   flac,      1.1.3)

AC_MSG_CHECKING([for Win32])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
#ifdef WIN32
#error My sympathies
#endif
],[])], [pdh_win32="no"], [pdh_win32="yes"])
if test "$pdh_win32" = "yes"; then
  AC_MSG_RESULT([yes])
  WIN32="yes"
else
  AC_MSG_RESULT([no])
  WIN32="no"
fi
AC_SUBST(WIN32)

PDH_PKGCONFIG(DBUS,      dbus-1,    1.1.1)
if test "x${with_lgpl}" != xno ; then
PDH_PKGCONFIG(TAGLIB,    taglib,    1.4)
PDH_PKGCONFIG(LIBCDDB,   libcddb,   1.2.1)
fi
if test "x${with_lgpl}" != xno ; then
PDH_PKGCONFIG(MPG123,    libmpg123, 1.2.0)
PDH_PKGCONFIG(GSTREAMER, gstreamer-0.10, 0.10.13)
fi
if test "x${with_gpl}" != xno ; then
if test "${PARANOIA}" == "no" ; then
PDH_PKGCONFIG(LIBCDIOP,  libcdio_paranoia, 0.76)
fi
fi

# NB do NOT use LIBFLAC_CFLAGS/LIBFLACPP_CFLAGS -- they introduce a directory
# with a bogus <assert.h>.

# ax_boost.m4 is AllPermissive
AX_BOOST([1.35.0],[BOOST="yes"],[BOOST="no"])
if test "${ax_cv_boost_thread}${ax_cv_boost_regex}" != "yesyes" ; then
  BOOST="no"
else
  AC_LANG_PUSH([C++])
  AC_CHECK_HEADERS([boost/spirit/include/classic.hpp])
  AC_LANG_POP
fi

# Checks for header files.
AC_CHECK_HEADERS([io.h poll.h errno.h sched.h net/if.h stdint.h stdlib.h unistd.h shlwapi.h pthread.h ws2tcpip.h sys/poll.h sys/time.h sys/types.h sys/socket.h netinet/in.h netinet/tcp.h sys/resource.h linux/dvb/dmx.h linux/dvb/frontend.h])
AC_CHECK_FUNCS([mkstemp localtime_r setpriority getaddrinfo gettimeofday gethostbyname_r sched_setscheduler])

AC_CHECK_LIB([ws2_32],[main])
AC_CHECK_LIB([shlwapi],[main])
AC_CHECK_LIB([pthread],[main])

AC_CHECK_DECLS([DMX_OUT_TS_TAP,DMX_OUT_TSDEMUX_TAP],,,[
#ifdef HAVE_LINUX_DVB_DMX_H
#include <linux/dvb/dmx.h>
#endif
])

AC_CHECK_HEADERS([sys/inotify.h])
AC_CHECK_FUNCS([inotify_init])
if test "$ac_cv_func_inotify_init" = "no"; then
  AC_CHECK_HEADERS([linux/unistd.h linux/inotify.h])
  AC_MSG_CHECKING([for syscall(__NR_inotify_init) for Debian's benefit])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_LINUX_UNISTD_H
#include <linux/unistd.h>
#endif
],[
 int fd = syscall(__NR_inotify_init);
])], [pdh_nr_inotify_init="yes"], [pdh_nr_inotify_init="no"])
  if test "$pdh_nr_inotify_init" = "yes"; then
    AC_MSG_RESULT([yes])
    AC_DEFINE([HAVE_NR_INOTIFY_INIT], 1,
	      [Define if you have __NR_inotify_init])
  else
    AC_MSG_RESULT([no])
  fi
fi

AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

AC_DEFUN([PDH_CHECK_SOCKOPT], [
AC_MSG_CHECKING([for $1])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#include <stdio.h>
#include <stdlib.h>
]],
[[
    int flag = 1;
    return setsockopt(0, IPPROTO_TCP, $1, &flag, sizeof(flag));
]])], [AC_MSG_RESULT([yes]) ; ac_pdh_sockopt=yes],
      [AC_MSG_RESULT([no]) ; ac_pdh_sockopt=no])
if test "$ac_pdh_sockopt" = "yes"; then
AC_DEFINE_UNQUOTED([HAVE_$1], 1, [Define to 1 if you have the socket option $1])
fi
])

PDH_CHECK_SOCKOPT([TCP_CORK])
PDH_CHECK_SOCKOPT([IP_RECVIF])
PDH_CHECK_SOCKOPT([TCP_NOPUSH])
PDH_CHECK_SOCKOPT([IP_PKTINFO])
PDH_CHECK_SOCKOPT([TCP_NODELAY])

AC_DEFUN([PDH_CHECK_ERRNO], [
AC_MSG_CHECKING([for $1])
AC_LANG_PUSH([C++])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include <boost/cerrno.hpp>
]],
[[
    return (errno == $1);
]])], [AC_MSG_RESULT([yes]) ; ac_pdh_errno=yes],
      [AC_MSG_RESULT([no]) ; ac_pdh_errno=no])
if test "$ac_pdh_errno" = "yes"; then
AC_DEFINE_UNQUOTED([HAVE_$1], 1, [Define to 1 if you have the error-code $1])
fi
AC_LANG_POP
])

PDH_CHECK_ERRNO([EISCONN])
PDH_CHECK_ERRNO([EALREADY])
PDH_CHECK_ERRNO([EOPNOTSUPP])
PDH_CHECK_ERRNO([EINPROGRESS])
PDH_CHECK_ERRNO([EWOULDBLOCK])

AC_ARG_ENABLE([final],
[AS_HELP_STRING([--enable-final],
[Compile final binaries in a single compiler invocation. Smaller output, but needs ~1GB RAM (or swap) during compilation.])],FINAL=yes,FINAL=no)
AC_SUBST(FINAL)

# Checks for typedefs, structures, and compiler characteristics.
#AC_HEADER_STDBOOL
#AC_C_CONST
#AC_C_INLINE

pdh_cd=no
if test "${PARANOIA}" == "yes" ; then
  pdh_cd=yes
else
  if test "${LIBCDIOP}" == "yes" ; then
    pdh_cd=yes
    printf "\n** Warning: cdparanoia 10 not found, using libcdio instead\n   A future version of Chorale will require cdparanoia\n"
  fi
fi

# Optional libraries
printf "\n"
pdh_ok=yes
pdh_fail=no
if test "${pdh_cd}${LIBCDDB}${QT}${LAME}${LIBFLAC}${GSTREAMER}${DBUS}${HAL}${MPG123}${TAGLIB}${HAVE_CONVERT}${HAVE_XSLTPROC}${HAVE_INKSCAPE}" != "yesyesyesyesyesyesyesyesyesyesyesyesyes" ; then
test "${QT}" = "yes"        || printf "** Install Qt 4.2.0 or later to build optional choralecd (4.5.0 for LGPL)\n   http://www.troll.no\n"
test "${LIBFLAC}" = "yes"   || printf "** Install flac 1.1.3 or later to build optional CD support\n   http://flac.sourceforge.net\n"
test "${LAME}" = "yes"      || printf "** Install Lame 3.96.1 or later to build optional CD support\n   http://lame.sourceforge.net\n"
test "${pdh_cd}" = "yes"    || printf "** Install cdparanoia 10 or later, or libcdio 0.76 or later to build\n   optional CD support\n   http://www.xiph.org/paranoia/\n   http://www.gnu.org/software/libcdio\n"
test "${LIBCDDB}" = "yes"   || printf "** Install libcddb 1.2.1 or later to build optional CD support\n   http://libcddb.sourceforge.net\n"
test "${HAL}" = "yes"       || printf "** Install hal 0.5.9.1 or later to build optional auto-configuration\n   http://hal.freedesktop.org\n"
test "${DBUS}" = "yes"      || printf "** Install libdbus 1.1.1 or later to build optional auto-configuration\n   http://dbus.freedesktop.org\n"
test "${GSTREAMER}" = "yes" || printf "** Install gstreamer 0.10.13 or later to build optional playback support\n   http://gstreamer.freedesktop.org\n"
test "${TAGLIB}" = "yes"    || printf "** Install taglib-1.4 or later to build optional CD and media-server support\n   http://developer.kde.org/~wheeler/taglib.html\n"
test "${MPG123}" = "yes"    || printf "** Install libmpg123 1.2.0 or later to build optional MP2 transcoding support\n   http://www.mpg123.de\n"
test "${HAVE_CONVERT}" = "yes"   || printf "** Install convert from imagemagick 6.2.4 or later if you edit Chorale's images\n   http://www.imagemagick.org\n"
test "${HAVE_INKSCAPE}" = "yes"  || printf "** Install Inkscape 0.44.1 or later if you edit Chorale's images\n   http://www.inkscape.org\n"
test "${HAVE_XSLTPROC}" = "yes"  || printf "** Install xsltproc from libxslt 1.1.20 or later if you edit Chorale's XML\n  http://xmlsoft.org/XSLT/index.html\n"
printf "\n"
pdh_ok=no
fi

# Required libraries
if test "${BOOST}" != "yes" ; then
test "${BOOST}" = "yes"         || printf "** Boost 1.35 or later (at least -regex and -thread) is required\n   http://www.boost.org\n"
printf "\n"
pdh_ok=no
pdh_fail=yes
fi

if test "${pdh_ok}" = "no" ; then
printf "With most distributions, it's the '-dev' packages you need. In Ubuntu\n"
printf "or Debian, try:\n"
printf "   sudo apt-get install inkscape make g++ autoconf pkg-config\n"
printf "        libboost1.35-dev xsltproc libtool\n"
printf "        libhal-dev libflac-dev libqt4-dev libgstreamer0.10-dev\n"
printf "        libcdparanoia-dev libtag1-dev libcddb-dev libmpg123-dev\n"
printf "        liblame-dev\n"
printf "\n"
fi

if test "${pdh_fail}" = "yes" ; then
AC_MSG_ERROR([Library dependencies not met])
fi

TARGET=`${CC} -dumpmachine`
AC_SUBST(TARGET)
LIBTOOL=${LIBTOOL}.${TARGET}

AC_CONFIG_HEADER([config.${TARGET}.h:config.h.in])
AC_CONFIG_FILES([Make.config.${TARGET}:Make.config.in stamp-h])

# Out-of-tree builds get their compiler defaulted
if test `readlink -f .` != `readlink -f ${srcdir}` ; then
echo "TOP:=${srcdir}/" > Makefile
echo "CC:=${CC}" >> Makefile
echo "include Make.config.${TARGET}" >> Makefile
echo 'include $(TOP)Makefile' >> Makefile
fi

AC_OUTPUT

if test -f libtool ; then
  echo "#!/bin/sh" > libtool.${TARGET}
  echo "echo=echo" >> libtool.${TARGET}
  cat libtool >> libtool.${TARGET}
  rm libtool
fi
