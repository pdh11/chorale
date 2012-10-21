#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(chorale, [0.09], pdh@utter.chaos.org.uk)

AC_MSG_NOTICE([Required: Taglib, Boost, xsltproc, and libxml++.])
AC_MSG_NOTICE([Optional: qt, gstreamer, libcdio, libcddb, lame, flac, curl, libupnp, dbus, hal, libuuid, openssl, libmpg123, inkscape, imagemagick.])
AC_MSG_NOTICE([])

AC_CONFIG_SRCDIR([libdb/db.h])
AC_CONFIG_AUX_DIR([autotools])

# Checks for programs.
AC_PROG_CC
AC_GNU_SOURCE
AC_PROG_LIBTOOL

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
PDH_CXX_WARNING([-Waddress])
PDH_CXX_WARNING([-Wlogical-op])
PDH_CXX_WARNING([-Wcast-align])
PDH_CXX_WARNING([-Wconversion])
PDH_CXX_WARNING([-Wwrite-strings])
PDH_CXX_WARNING([-Wpointer-arith])
# PDH_CXX_WARNING([-Wsign-conversion])
PDH_CXX_WARNING([-Wstrict-overflow])
PDH_CXX_WARNING([-Wnon-virtual-dtor])
PDH_CXX_WARNING([-Woverloaded-virtual])
PDH_CXX_WARNING([-pedantic -Wno-long-long])
PDH_CXX_WARNING([-fdiagnostics-show-option])

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

# Checks for libraries.

AC_CHECK_LIB(mp3lame, lame_init, [LAME="yes" LAME_LIBS="-lmp3lame"], [LAME="no"], -lm)
if test "${LAME}" = "yes" ; then
    AC_CHECK_HEADER(lame/lame.h, , [LAME="no"])
fi
AC_SUBST(LAME)
AC_SUBST(LAME_LIBS)
if test "${LAME}" = "yes" ; then
    AC_DEFINE(HAVE_LAME, 1, [Define to 1 if you have LAME])
fi

XIPH_PATH_OGG()

PKG_CHECK_MODULES(QT, QtGui >= 4.2.0 Qt3Support >= 4.2.0 QtCore >= 4.2.0, [QT="yes"], [QT="no"])
AC_SUBST(QT)
AC_SUBST(QT_CFLAGS)
AC_SUBST(QT_LIBS)
if test "${QT}" = "yes" ; then
  QT_MOC=`pkg-config --variable=moc_location QtGui`
  AC_SUBST(QT_MOC)
fi

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
PDH_PKGCONFIG(LIBUUID,   uuid,      1.00)
PDH_PKGCONFIG(DBUS,      dbus-1,    1.1.1)
PDH_PKGCONFIG(TAGLIB,    taglib,    1.4)
PDH_PKGCONFIG(LIBCDDB,   libcddb,   1.2.1)
PDH_PKGCONFIG(CURL,      libcurl,   7.16.1)
PDH_PKGCONFIG(UPNP,      libupnp,   1.2.1)
PDH_PKGCONFIG(OPENSSL,   openssl,   0.9.0)
PDH_PKGCONFIG(MPG123,    libmpg123, 1.2.0)
PDH_PKGCONFIG(LIBXMLPP,  libxml++-2.6, 2.14.0)
PDH_PKGCONFIG(GSTREAMER, gstreamer-0.10, 0.10.13)
PDH_PKGCONFIG(LIBCDIOP,  libcdio_paranoia, 0.76)

# NB do NOT use LIBFLAC_CFLAGS/LIBFLACPP_CFLAGS -- they introduce a directory
# with a bogus <assert.h>.

# ax_boost.m4 is AllPermissive
AX_BOOST([1.33.1],[BOOST="yes"],[BOOST="no"])
if test "${ax_cv_boost_thread}${ax_cv_boost_regex}" != "yesyes" ; then
  BOOST="no"
fi

# Checks for header files.
AC_CHECK_HEADERS([stdint.h unistd.h sys/types.h sys/socket.h netinet/in.h netinet/tcp.h linux/dvb/dmx.h linux/dvb/frontend.h])

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

# Checks for typedefs, structures, and compiler characteristics.
#AC_HEADER_STDBOOL
#AC_C_CONST
#AC_C_INLINE

# Optional libraries
printf "\n"
pdh_ok=yes
pdh_fail=no
if test "${LIBCDIOP}${LIBCDDB}${QT}${LAME}${LIBFLAC}${CURL}${UPNP}${GSTREAMER}${DBUS}${HAL}${LIBUUID}${OPENSSL}${MPG123}${TAGLIB}${HAVE_CONVERT}" != "yesyesyesyesyesyesyesyesyesyesyesyesyesyesyes" ; then
test "${QT}" = "yes"        || printf "** Install Qt 4.2.0 or later to build optional choralecd\n   http://www.troll.no\n"
test "${LIBFLAC}" = "yes"   || printf "** Install flac 1.1.3 or later to build optional CD support\n   http://flac.sourceforge.net\n"
test "${LAME}" = "yes"      || printf "** Install Lame 3.96.1 or later to build optional CD support\n   http://lame.sourceforge.net\n"
test "${LIBCDIOP}" = "yes"  || printf "** Install libcdio 0.76 or later to build optional CD support\n   http://www.gnu.org/software/libcdio\n"
test "${LIBCDDB}" = "yes"   || printf "** Install libcddb 1.2.1 or later to build optional CD support\n   http://libcddb.sourceforge.net\n"
test "${LIBUUID}" = "yes"   || printf "** Install libuuid 1.00 or later to build optional UPnP support\n   http://e2fsprogs.sourceforge.net\n"
test "${OPENSSL}" = "yes"   || printf "** Install openssl 0.9.0 or later to build optional UPnP support\n   http://www.openssl.org\n"
test "${UPNP}" = "yes"      || printf "** Install libupnp-1.4.3 or later to build optional UPnP support\n   http://pupnp.sf.net\n"
test "${HAL}" = "yes"       || printf "** Install hal 0.5.9.1 or later to build optional auto-configuration\n   http://hal.freedesktop.org\n"
test "${CURL}" = "yes"      || printf "** Install curl 7.16.1 or later to build optional Rio Receiver client\n   http://curl.haxx.se\n"
test "${DBUS}" = "yes"      || printf "** Install libdbus 1.1.1 or later to build optional auto-configuration\n   http://dbus.freedesktop.org\n"
test "${GSTREAMER}" = "yes" || printf "** Install gstreamer 0.10.13 or later to build optional playback support\n   http://gstreamer.freedesktop.org\n"
test "${LIBXMLPP}" = "yes"  || printf "** Install libxml++-2.14.0 or later to build optional media-server support\n   http://libxmlplusplus.sourceforge.net\n"
test "${TAGLIB}" = "yes"    || printf "** Install taglib-1.4 or later to build optional CD and media-server support\n   http://developer.kde.org/~wheeler/taglib.html\n"
test "${MPG123}" = "yes"    || printf "** Install libmpg123 1.2.0 or later to build optional MP2 transcoding support\n   http://www.mpg123.de\n"
test "${HAVE_CONVERT}" = "yes"  || printf "** Install convert from imagemagick 6.2.4 or later to build optional choralecd\n   http://www.imagemagick.org\n"
printf "\n"
pdh_ok=no
fi

# Required libraries
if test "${BOOST}${HAVE_XSLTPROC}${HAVE_INKSCAPE}" != "yesyes" ; then
test "${HAVE_INKSCAPE}" = "yes" || printf "** Inkscape 0.44.1 or later is required\n   http://www.inkscape.org\n"
test "${BOOST}" = "yes"         || printf "** Boost 1.33.1 or later (at least -regex and -thread) is required\n   http://www.boost.org\n"
test "${HAVE_XSLTPROC}" = "yes" || printf "** Xsltproc from libxslt 1.0.0 or later is required\n  http://xmlsoft.org/XSLT/index.html\n"
printf "\n"
pdh_ok=no
pdh_fail=yes
fi

if test "${pdh_ok}" = "no" ; then
printf "With most distributions, it's the '-dev' packages you need. In Ubuntu\n"
printf "or Debian, try:\n"
printf "   sudo apt-get install g++ autoconf automake libflac-dev libflac++-dev\n"
printf "        libdbus-1-dev libhal-dev uuid-dev libssl-dev libtag1-dev\n"
printf "        libboost-thread-dev libboost-regex-dev libxml++2.6-dev liblame-dev\n"
printf "        libcurl4-openssl-dev libupnp-dev libcdio-paranoia-dev libcddb2-dev\n"
printf "        libgstreamer0.10-dev libqt4-dev qt4-dev-tools\n"
printf "        libmysqlclient15-dev imagemagick inkscape\n"
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
AC_OUTPUT

if test -f libtool ; then
  echo "#!/bin/sh" > libtool.${TARGET}
  echo "echo=echo" >> libtool.${TARGET}
  cat libtool >> libtool.${TARGET}
  rm libtool
fi
