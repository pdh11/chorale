#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(chorale, [0.20a3], pdh@utter.chaos.org.uk)
AC_SUBST(PACKAGE_WEBSITE, [http://chorale.sf.net])
AC_DEFINE_UNQUOTED([PACKAGE_WEBSITE], ["${PACKAGE_WEBSITE}"], [Define to the package's website])


AC_MSG_NOTICE([Required: Boost.])
AC_MSG_NOTICE([Optional: Taglib, Qt4, gstreamer, cdparanoia or libcdio, libcddb, lame, flac, dbus, hal, libmpg123, inkscape, xsltproc, imagemagick.])
AC_MSG_NOTICE([])

AC_CONFIG_SRCDIR([libdb/db.h])
AC_CONFIG_MACRO_DIR([autotools])
AC_CONFIG_AUX_DIR([autotools])

AC_DEFUN([PDH_REQUISITIONS],
[
	AC_REQUIRE([AC_GNU_SOURCE])
	AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])
])
PDH_REQUISITIONS

# Checks for programs.
AC_PROG_CC
CFLAGS="-O2" # Override -g

AC_PROG_CXX
CXXFLAGS="-O2" # Override -g

AC_PROG_LIBTOOL
AC_C_BIGENDIAN

AC_SYS_LARGEFILE
if test "x$ac_cv_sys_file_offset_bits" != "x" -a "x$ac_cv_sys_file_offset_bits" != "xno" -a "x$ac_cv_sys_file_offset_bits" != "xunknown" ; then
  LARGEFILES=-D_FILE_OFFSET_BITS=$ac_cv_sys_file_offset_bits
fi
if test "x$ac_cv_sys_large_files" != "x" -a "x$ac_cv_sys_large_files" != "xunknown" ; then
  LARGEFILES=-D_LARGE_FILES=$ac_cv_sys_large_files
fi
AC_SUBST(LARGEFILES)

AC_CHECK_DECL([__APPLE__],[DARWIN="yes"],[DARWIN="no"])
AC_CHECK_DECL([__WIN32__],[WIN32="yes"],[WIN32="no"])

if test "$WIN32" = "yes"; then
  DOTEXE=".exe"
  # We need at least 0x500 (Win2K) not the default 0x400 (95/NT4)
  CFLAGS="$CFLAGS -DWINVER=0x500 -DTAGLIB_STATIC -DBOOST_THREAD_USE_LIB"
  CXXFLAGS="$CXXFLAGS -DWINVER=0x500 -DTAGLIB_STATIC -DBOOST_THREAD_USE_LIB"
else
  DOTEXE=""
fi
AC_SUBST(DARWIN)
AC_SUBST(WIN32)
AC_SUBST(DOTEXE)

AC_DEFUN([PDH_C_WARNING],
[
    AC_MSG_CHECKING([whether ${CC} accepts $1])

    ok_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS $1"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM()],
		      [AC_MSG_RESULT([yes])],
		      [AC_MSG_RESULT([no]); CFLAGS=$ok_CFLAGS; $2])
])

PDH_C_WARNING([-W])
PDH_C_WARNING([-Wall])
PDH_C_WARNING([-Wextra])
PDH_C_WARNING([-Wundef])
PDH_C_WARNING([-Wshadow])
PDH_C_WARNING([-Wswitch])
PDH_C_WARNING([-Waddress])
PDH_C_WARNING([-std=gnu1x], PDH_C_WARNING([-std=gnu99]))
PDH_C_WARNING([-Wcoercion])
PDH_C_WARNING([-Wcast-align])
PDH_C_WARNING([-Wconversion])
PDH_C_WARNING([-Wwrite-strings])
PDH_C_WARNING([-Wpointer-arith])
PDH_C_WARNING([-Wbad-function-cast])
PDH_C_WARNING([-Wstrict-prototypes])
PDH_C_WARNING([-Wmissing-prototypes])
PDH_C_WARNING([-Wmissing-declarations])
PDH_C_WARNING([-pedantic -Wno-long-long])
PDH_C_WARNING([-fdiagnostics-show-option])
PDH_C_WARNING([-Wdeclaration-after-statement])

AC_MSG_CHECKING(bitfield ordering in structs)
AC_LINK_IFELSE([
int conftest_undefined_reference_();
int main() {
  union {
    unsigned int x;
    struct {
      unsigned int x0: 1;
      unsigned int x1: 31;
    } s;
  } u;

  u.x = 0;
  u.s.x0 = 1;
  if (u.x == 1)
    return conftest_undefined_reference_();
  return 0;
}
], [bf_lsbf=0; AC_MSG_RESULT(MSB first);], [bf_lsbf=1; AC_MSG_RESULT(LSB first);])
AC_DEFINE_UNQUOTED(BITFIELDS_LSB_FIRST, $bf_lsbf,
	[Define to 1 if C bitfields start from the LSB, or 0 for MSB])

AC_DEFUN([PDH_LD_MAP],
[
    AC_MSG_CHECKING([how to get link maps])

    ok_LDFLAGS="$LDFLAGS"
    LDFLAGS="$LDFLAGS -Wl,-Map,map.txt"
    AC_LINK_IFELSE([AC_LANG_PROGRAM()],
		   [AC_MSG_RESULT([-Map]); LDMAP='-Wl,-Map,$(@)-map-$(FULLTARGET).txt'],
	           [
		       LDFLAGS="$ok_LDFLAGS -Wl,-M"
		       AC_LINK_IFELSE([AC_LANG_PROGRAM()],
				      [AC_MSG_RESULT([-M]); LDMAP='-Wl,-M > $(@)-map-$(FULLTARGET).txt'],
				      [AC_MSG_RESULT([can't])])
		   ])
    LDFLAGS="$ok_LDFLAGS"

    AC_SUBST([LDMAP])
])

AC_DEFUN([PDH_LD_WARNING],
[
    AC_MSG_CHECKING([whether linker ${CC} accepts $1])

    ok_LDFLAGS="$LDFLAGS"
    LDFLAGS="$LDFLAGS $1"
    AC_LINK_IFELSE([AC_LANG_PROGRAM()],
		   [AC_MSG_RESULT([yes])],
		   [AC_MSG_RESULT([no]); LDFLAGS=$ok_LDFLAGS])
])

PDH_LD_MAP
PDH_LD_WARNING([-mthreads])
PDH_LD_WARNING([-Wl,--as-needed])

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
PDH_CXX_WARNING([-pedantic])
PDH_CXX_WARNING([-Wnoexcept])
PDH_CXX_WARNING([-Wcast-qual])
PDH_CXX_WARNING([-std=gnu++0x], PDH_CXX_WARNING([-std=gnu++98]))
PDH_CXX_WARNING([-Wlogical-op])
PDH_CXX_WARNING([-Wcast-align])
PDH_CXX_WARNING([-Wconversion])
PDH_CXX_WARNING([-Wc++0x-compat])
PDH_CXX_WARNING([-fstrict-enums])
PDH_CXX_WARNING([-Wno-long-long])
PDH_CXX_WARNING([-Wwrite-strings])
PDH_CXX_WARNING([-Wpointer-arith])
PDH_CXX_WARNING([-Wnon-virtual-dtor])
PDH_CXX_WARNING([-Wstrict-overflow=5])
PDH_CXX_WARNING([-Woverloaded-virtual])
PDH_CXX_WARNING([-Wno-sign-conversion])
PDH_CXX_WARNING([-Wmissing-declarations])
PDH_CXX_WARNING([-Wunused-but-set-variable])
PDH_CXX_WARNING([-fdiagnostics-show-option])

AC_CHECK_TOOL([GCOV], [gcov], [:])
AC_CHECK_TOOL([SIZE], [size], [:])
AC_CHECK_TOOL([WINDRES], [windres], [:])
AC_PATH_PROG(CADAVER, cadaver, no)
if test "$CADAVER" = "no"; then
   HAVE_CADAVER="no"
else
   HAVE_CADAVER="yes"
fi
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
PKG_PROG_PKG_CONFIG
AC_PROG_INSTALL
AC_SUBST(HAVE_CADAVER)
AC_SUBST(HAVE_CONVERT)
AC_SUBST(HAVE_XSLTPROC)
AC_SUBST(HAVE_INKSCAPE)
AC_SUBST(HAVE_VALGRIND)
AC_SUBST(HAVE_WINDRES)
AC_SUBST(VALGRIND_OPTS)
if test "${HAVE_CADAVER}" = "yes" ; then
	haveit=1
else
   	haveit=0
fi
AC_DEFINE_UNQUOTED(HAVE_CADAVER, $haveit,
	[Define to 1 if you have the cadaver program (used for testing), or 0 if you don't])


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

# Check for the existence of a system header, by trying to compile (but not
# link) a file that includes it.
#
AC_DEFUN([PDH_CHECK_HEADER], [

    AC_MSG_CHECKING([for <$1>])

    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[$pdh_all_headers
#include <$1>]])],
		  [AC_MSG_RESULT([yes]); pdh_haveit=1; pdh_yesno="yes"],
		  [AC_MSG_RESULT([no]);  pdh_haveit=0; pdh_yesno="no"])

    AC_DEFINE_UNQUOTED(AS_TR_CPP([HAVE_$1]), $pdh_haveit,
	 [Define to 1 if you have <$1>, or to 0 if you don't.])

    AS_TR_SH([ac_cv_header_$1])=$pdh_yesno
])

# Check for the existence of a system header, and add an attempted include
# of it to $pdh_all_headers, for later use when checking for functions.
#
AC_DEFUN([PDH_CHECK_HEADER_AND_ADD], [

    PDH_CHECK_HEADER([$1])

    pdh_varname=AS_TR_CPP([HAVE_$1])
    pdh_all_headers+=`echo -e "\n#if $pdh_varname\n#include <$1>\n#endif\n"`
])

# Check for each of a list of headers.
#
AC_DEFUN([PDH_CHECK_HEADERS], [
    m4_foreach_w([pdh_header], $1, [PDH_CHECK_HEADER(pdh_header)])
])

# Check for each of a list of headers, adding to $pdh_all_headers
#
AC_DEFUN([PDH_CHECK_HEADERS_AND_ADD], [
    m4_foreach_w([pdh_header], $1, [PDH_CHECK_HEADER_AND_ADD(pdh_header)])
])

# Check for libwrap, part of TCP Wrappers
#
AC_DEFUN([PDH_CHECK_TCP_WRAPPERS],[

    if test "x${with_libwrap}" != xno; then
        AC_MSG_CHECKING([for tcp_wrappers])
	ok_LIBS=${LIBS}
	LIBS="$LIBS -lwrap"
	AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <tcpd.h>
int allow_severity = 0;
int deny_severity = 0;
		]],
	        [[return hosts_access();]])],
	    [AC_MSG_RESULT([yes]); HAVE_LIBWRAP="yes"; LIBWRAP="-lwrap"],
            [AC_MSG_RESULT([no]) ; HAVE_LIBWRAP="no" ] )
        # Set it back unconditionally; we don't want to link with -lwrap
	# unless we have to.
	LIBS=$ok_LIBS
    fi

    AC_SUBST(LIBWRAP)
    AC_SUBST(HAVE_LIBWRAP)

    pdh_haveit=0
    if test "x${HAVE_LIBWRAP}" = xyes; then
	pdh_haveit=1
    fi
    AC_DEFINE_UNQUOTED(HAVE_LIBWRAP, $pdh_haveit,
	[Define to 1 if you have libwrap from tcp_wrappers, or to 0 if you don't])
])

# Check for LAME
#
AC_DEFUN([PDH_CHECK_LAME],[

    AC_MSG_CHECKING([for lame])

    if test "x${with_lgpl}" != xno ; then
   	ok_LIBS="$LIBS"
   	LIBS="$LIBS -lmp3lame"
   	LAME="no"
   	AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <lame/lame.h>]],
					[[(void)lame_init();]])],
   	    [AC_MSG_RESULT([yes]); LAME="yes"; LAME_LIBS="-lmp3lame"],
	    [
		# Sometimes (e.g. Debian 5.0) lame needs -lm but isn't linked
		# against it, so it fails to link with --as-needed. So we try
		# it again with --no-as-needed.
		LIBS="$LIBS -Wl,--no-as-needed -lm"
        	AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <lame/lame.h>]],
			   			[[(void)lame_init();]])],
   		    [
			AC_MSG_RESULT([yes (-lm)])
			LAME="yes"
			LAME_LIBS="-lmp3lame -Wl,--no-as-needed -lm"],
		    [AC_MSG_RESULT([no])])])
   	LIBS=$ok_LIBS
    else
   	AC_MSG_RESULT([no (LGPL)])
    fi

    AC_SUBST(LAME)
    AC_SUBST(LAME_LIBS)

    if test "${LAME}" = "yes" ; then
    	haveit=1
    else
    	haveit=0
    fi
    AC_DEFINE_UNQUOTED(HAVE_LAME, $haveit,
	[Define to 1 if you have LAME, or 0 if you don't])
])

# Check for CD-Paranoia
#
AC_DEFUN([PDH_CHECK_CDPARANOIA],[

    AC_MSG_CHECKING([for cdparanoia])

    pdh_haveit=0
    if test "x${with_lgpl}" != xno ; then

	ok_LIBS=$LIBS
	LIBS="$LIBS -lcdda_paranoia -lcdda_interface"

	AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <cdda_interface.h>
#include <cdda_paranoia.h>
]],[[(void)paranoia_version();]])],
			[AC_MSG_RESULT([yes]); pdh_haveit=1],
			[AC_MSG_RESULT([no])])

	LIBS=$ok_LIBS
    else
   	AC_MSG_RESULT([no (LGPL)])
    fi

    if test "$pdh_haveit" = 1 ; then
    	PARANOIA="yes"
    	PARANOIA_LIBS="-lcdda_paranoia -lcdda_interface"
    else
    	PARANOIA="no"
    fi
    AC_SUBST(PARANOIA)
    AC_SUBST(PARANOIA_LIBS)

    AC_DEFINE_UNQUOTED(HAVE_PARANOIA, $pdh_haveit,
	[Define to 1 if you have `real' cdparanoia (not libcdio), or to 0 if you don't])
])

## Check for Boost.regex and Boost.thread
#
AC_DEFUN([PDH_CHECK_BOOST], [
  AC_MSG_CHECKING([for Boost])
  if test "$WIN32" = "yes" ; then
    MAYBE_BOOST_THREAD="-lboost_thread_win32-mt -lboost_thread_win32 -lboost_thread-mt -lboost_thread"
  else
    MAYBE_BOOST_THREAD="-lboost_thread-mt -lboost_thread"
  fi
  AC_LANG_PUSH([C++])
  BOOST_THREAD=
  for i in $MAYBE_BOOST_THREAD ; do
    ok_LIBS=$LIBS
    LIBS="$LIBS $i"
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <boost/thread.hpp>
]],
[[
	boost::thread t;
]])],
	[BOOST_THREAD=$i ; break;],
	[])
    LIBS=$ok_LIBS
  done
  if test "$BOOST_THREAD" = "" ; then
    BOOST="no"
  else
    BOOST_REGEX=
    for i in -lboost_regex-mt -lboost_regex ; do
      ok_LIBS=$LIBS
      LIBS="$LIBS $i"
      AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <boost/regex.hpp>
]],
[[
	boost::regex rx("foo");
	boost::regex_match("afooa", rx);
]])],
	[BOOST_REGEX=$i ; break],[])
      LIBS=$ok_LIBS
    done
    if test "$BOOST_REGEX" = "" ; then
      BOOST="no"
    else
      BOOST="yes"
      BOOST_LIBS="$BOOST_THREAD $BOOST_REGEX"
    fi
  fi
  AC_LANG_POP
  AC_MSG_RESULT($BOOST)
  AC_SUBST(BOOST_LIBS)
])

## Check for InnoDB (NYI)
#
AC_DEFUN([PDH_CHECK_INNODB],[
AC_MSG_CHECKING([for innodb])
pdh_haveit=0
if test "x${with_gpl}" != xno ; then
# Listed in reverse order of preference
for i in /usr/include/embedded_innodb-1.0 /usr/include/embedded_innodb /usr/include/innodb; do
  ok_CFLAGS=$CFLAGS
  CFLAGS="$CFLAGS -I$i"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <api0api.h>
]])],
	[INNODB_CFLAGS=-I$i ; pdh_haveit=1],
	[])
  CFLAGS=$ok_CFLAGS
done
if test "$pdh_haveit" = 1 ; then
  ok_LIBS=$LIBS
  LIBS="$LIBS -linnodb -lpthread"
  AC_LINK_IFELSE([AC_LANG_PROGRAM([],[[
(void)ib_init();
  ]])],
	[AC_MSG_RESULT([yes]); pdh_haveit=1],
	[AC_MSG_RESULT([no]);  pdh_haveit=0])
  LIBS=$ok_LIBS
else
  AC_MSG_RESULT([no])
fi
else
  AC_MSG_RESULT([no (GPL)])
fi
if test "$pdh_haveit" = 1 ; then
    INNODB="yes"
    INNODB_LIBS="-linnodb -lpthread"
else
    INNODB="no"
fi
AC_SUBST(INNODB)
AC_SUBST(INNODB_CFLAGS)
AC_SUBST(INNODB_LIBS)
AC_DEFINE_UNQUOTED(HAVE_INNODB, $pdh_haveit, [Define to 1 if you have Embedded InnoDB, or to 0 if you don't])
])

disable_gpl=""
disable_lgpl=""
if test "x${with_gpl}" == xno ; then
  disable_gpl="GPL"
fi
if test "x${with_lgpl}" == xno ; then
  disable_gpl="GPL"
  disable_lgpl="LGPL"
fi

# Qt 4.5.0+ is LGPL, earlier Qt is GPL
QT="no"
AC_MSG_CHECKING([for Qt])
if test "x${with_lgpl}" != xno ; then
PKG_CHECK_EXISTS([QtGui >= 4.5.0 QtCore >= 4.5.0],
 [QT="yes"
  AC_MSG_RESULT([yes (LGPL)])
 ],[
  if test "x${with_gpl}" != xno ; then
    PKG_CHECK_EXISTS([QtGui >= 4.4.0 QtCore >= 4.4.0],
     [QT="yes"
      AC_MSG_RESULT([yes (GPL)])
     ],[
      AC_MSG_RESULT([no])
     ])
   else
      AC_MSG_RESULT([no (GPL)])
   fi
])
else
   AC_MSG_RESULT([no (LGPL)])
fi

if test "${QT}" = "yes" ; then
  QT_MOC=`$PKG_CONFIG --variable=moc_location QtGui`
  QT_CFLAGS=`$PKG_CONFIG --cflags QtGui QtCore`
  QT_LIBS=`$PKG_CONFIG --libs QtGui QtCore`
  AC_SUBST(QT_MOC)
  AC_SUBST(QT_CFLAGS)
  AC_SUBST(QT_LIBS)
fi
AC_SUBST(QT)

AC_DEFUN([PDH_PKGCONFIG], [
AC_REQUIRE([PKG_PROG_PKG_CONFIG])
AC_MSG_CHECKING([for $2])
ac_pdh_haveit=0
$1="no"
if test "x$4" != x ; then
  AC_MSG_RESULT([no ($4)])
else
  if AC_RUN_LOG([$PKG_CONFIG --exists --print-errors "$2"]); then
    if AC_RUN_LOG([$PKG_CONFIG --print-errors "$2 >= $3"]); then
      AC_MSG_RESULT([yes])
      $1_CFLAGS=`$PKG_CONFIG --cflags $2 2>/dev/null`
      $1_LIBS=`$PKG_CONFIG --libs $2 2>/dev/null`
      $1="yes"
      ac_pdh_haveit=1
    else
      pdh_result=`$PKG_CONFIG --modversion $2`
      AC_MSG_RESULT([no (have $pdh_result, need $3)])
    fi
  else
    AC_MSG_RESULT([no])
  fi
fi
AC_DEFINE_UNQUOTED(HAVE_$1, $ac_pdh_haveit, [Define to 1 if you have $2 >= $3, or to 0 if you don't])
AC_SUBST($1)
AC_SUBST($1_CFLAGS)
AC_SUBST($1_LIBS)
])


PDH_PKGCONFIG(HAL,       hal,       0.5.9.1)
PDH_CHECK_LAME
PDH_PKGCONFIG(LIBFLAC,   flac,      1.1.3)
PDH_PKGCONFIG(CAIRO,     cairo,     1.6.4)
PDH_CHECK_BOOST
PDH_CHECK_INNODB
PDH_PKGCONFIG(DBUS,      dbus-1,    1.1.1)
PDH_PKGCONFIG(TAGLIB,    taglib,    1.4,   $disable_lgpl)
PDH_PKGCONFIG(LIBCDDB,   libcddb,   1.2.1, $disable_lgpl)
PDH_PKGCONFIG(MPG123,    libmpg123, 1.2.0, $disable_lgpl)
PDH_CHECK_CDPARANOIA
PDH_PKGCONFIG(AVFORMAT,  libavformat, 50.0.0, $disable_lgpl)
PDH_CHECK_TCP_WRAPPERS
PDH_PKGCONFIG(GSTREAMER, gstreamer-0.10, 0.10.13, $disable_lgpl)
PDH_PKGCONFIG(LIBCDIOP,  libcdio_paranoia, 0.76,  $disable_gpl)

# NB do NOT use LIBFLAC_CFLAGS/LIBFLACPP_CFLAGS -- they introduce a directory
# with a bogus <assert.h>.

# Checks for header files.
PDH_CHECK_HEADERS_AND_ADD([io.h poll.h errno.h fcntl.h sched.h netdb.h stdio.h direct.h dirent.h net/if.h stdint.h stdlib.h string.h unistd.h windows.h pthread.h ws2tcpip.h sys/disk.h sys/poll.h sys/time.h sys/stat.h lame/lame.h net/if_dl.h sys/types.h sys/socket.h sys/syslog.h netinet/in.h sys/eventfd.h sys/utsname.h netinet/tcp.h sys/inotify.h sys/resource.h CoreFoundation/CFRunLoop.h])
PDH_CHECK_HEADERS([linux/cdrom.h linux/unistd.h linux/inotify.h linux/dvb/dmx.h linux/dvb/frontend.h]) # No AND_ADD as they may conflict with libc
AC_LANG_PUSH([C++])
PDH_CHECK_HEADER([boost/spirit/include/classic.hpp]) # No AND_ADD as it's C++
AC_LANG_POP

if test "$ac_cv_header_linux_cdrom_h" = "yes"; then
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
fi

AC_DEFUN([PDH_CHECK_RTTI], [
  AC_MSG_CHECKING([whether <$1> compiles with -fno-rtti])
  pdh_CXXFLAGS=$CXXFLAGS
  CXXFLAGS="$CXXFLAGS -fno-rtti -DBOOST_NO_TYPEID"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <$1>]],$2)],
    [AC_MSG_RESULT([yes]); AS_TR_SH([RTTI_$1])=""],
    [AC_MSG_RESULT([no]); AS_TR_SH([RTTI_$1])="yes"])
  CXXFLAGS=$pdh_CXXFLAGS
  AC_SUBST(AS_TR_SH([RTTI_$1]))
])

AC_DEFUN([PDH_CHECK_EXCEPTIONS], [
  AC_MSG_CHECKING([whether <$1> compiles with -fno-exceptions])
  pdh_CXXFLAGS=$CXXFLAGS
  CXXFLAGS="$CXXFLAGS -fno-exceptions"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <$1>]],$2)],
    [AC_MSG_RESULT([yes]); AS_TR_SH([EXCEPTIONAL_$1])=""],
    [AC_MSG_RESULT([no]); AS_TR_SH([EXCEPTIONAL_$1])="yes"])
  CXXFLAGS=$pdh_CXXFLAGS
  AC_SUBST(AS_TR_SH([EXCEPTIONAL_$1]))
])

AC_LANG_PUSH([C++])
PDH_CHECK_RTTI([boost/thread/mutex.hpp])
PDH_CHECK_RTTI([boost/algorithm/string/split.hpp])
PDH_CHECK_EXCEPTIONS([boost/regex.hpp],
	[ boost::regex rx("*foo*", boost::regex::icase);
	return boost::regex_match("barfoobar", rx); ])
PDH_CHECK_EXCEPTIONS([boost/function.hpp])
PDH_CHECK_EXCEPTIONS([boost/thread/tss.hpp],
	[ boost::thread_specific_ptr<int> t; return 0; ])
PDH_CHECK_EXCEPTIONS([boost/thread/mutex.hpp])
PDH_CHECK_EXCEPTIONS([boost/thread/thread.hpp],
	[ boost::thread t(main,NULL, NULL); return 0; ])
PDH_CHECK_EXCEPTIONS([boost/thread/condition.hpp])
AC_LANG_POP

AC_DEFUN([PDH_CHECK_FUNC], [
AC_MSG_CHECKING([for $1()])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[$pdh_all_headers]], [[(void(*)(void))$1;]])],
		[AC_MSG_RESULT([yes]); pdh_haveit=1; pdh_yesno="yes"],
		[AC_MSG_RESULT([no]);  pdh_haveit=0; pdh_yesno="no"])
AC_DEFINE_UNQUOTED(AS_TR_CPP([HAVE_$1]), $pdh_haveit, [Define to 1 if you have $1(), or to 0 if you don't.])
AS_TR_SH([ac_cv_func_$1])=$pdh_yesno
])

AC_DEFUN([PDH_CHECK_FUNCS], [
m4_foreach_w([pdh_func], $1, [PDH_CHECK_FUNC(pdh_func)])
])

PDH_CHECK_FUNCS([pipe fsync mkdir _wstat gettid _wmkdir eventfd pread64 mkstemp scandir gmtime_r pwrite64 realpath vasprintf ftruncate _vscprintf CreateEvent localtime_r setpriority getaddrinfo inotify_init gettimeofday posix_fadvise FindFirstFileW posix_fallocate gethostbyname_r sync_file_range GetFullPathNameW __mingw_vsnprintf GetCurrentThreadId sched_setscheduler lame_get_lametag_frame canonicalize_file_name])
pdh_nr_gettid=0
if test "$ac_cv_func_gettid$ac_cv_header_sys_types_h" = "noyes"; then
  AC_MSG_CHECKING([for syscall(__NR_gettid)])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([$pdh_all_headers
#include <linux/unistd.h>],[
 int fd = syscall(__NR_gettid);
])],
	[AC_MSG_RESULT([yes]); pdh_nr_gettid=1],
	[AC_MSG_RESULT([no])])
fi
AC_DEFINE_UNQUOTED([HAVE_NR_GETTID], $pdh_nr_gettid,
      [Define to 1 if you have __NR_gettid but not gettid(), or to 0 otherwise])

pdh_nr_inotify_init=0
if test "$ac_cv_func_inotify_init$ac_cv_header_unistd_h" = "noyes"; then
  AC_MSG_CHECKING([for syscall(__NR_inotify_init) for Debian's benefit])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([$pdh_all_headers],[
 int fd = syscall(__NR_inotify_init);
])],
	[AC_MSG_RESULT([yes]); pdh_nr_inotify_init=1],
	[AC_MSG_RESULT([no])])
fi
AC_DEFINE_UNQUOTED([HAVE_NR_INOTIFY_INIT], $pdh_nr_inotify_init,
      [Define to 1 if you have __NR_inotify_init but not inotify_init(), or to 0 otherwise])

AC_DEFUN([PDH_CHECK_SCANDIR],
[
  # This changed between glibc 2.9 and 2.10.1
  AC_MSG_CHECKING([type of scandir() comparator argument])
  AC_LANG_PUSH([C++])
  SCANDIR_COMPARATOR_ARG_T=
  for i in "const struct dirent**" "struct dirent**" "const void*" "void*" ; do
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <dirent.h>
    extern int my_compare($i, $i);
  ]],
  [[
    struct dirent **namelist;
    return scandir("/", &namelist, 0, my_compare);
  ]])],
  [
    SCANDIR_COMPARATOR_ARG_T=$i
    AC_MSG_RESULT([$i])
    break;
  ],[])
  done

  if test "$SCANDIR_COMPARATOR_ARG_T" == "" ; then
    AC_MSG_RESULT([n/a])
    HAVE_SCANDIR=no
  fi
  AC_DEFINE_UNQUOTED([SCANDIR_COMPARATOR_ARG_T], $SCANDIR_COMPARATOR_ARG_T,
    [Type of arguments to fourth argument of scandir])
  AC_LANG_POP
])

PDH_CHECK_SCANDIR


AC_DEFUN([PDH_CHECK_CONDITION_TIMED_WAIT], [
  AC_MSG_CHECKING([for boost::condition::timed_wait with interval])
  AC_LANG_PUSH([C++])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
  ]],
  [[
    boost::mutex mx;
    boost::mutex::scoped_lock lock(mx);
    boost::condition cond;
    cond.timed_wait(lock, boost::posix_time::seconds(3));
  ]])],
  [
    AC_MSG_RESULT([yes])
    HAVE_CONDITION_TIMED_WAIT_INTERVAL=1
  ],
  [
    AC_MSG_RESULT([no])
    HAVE_CONDITION_TIMED_WAIT_INTERVAL=0
  ])
  AC_LANG_POP
  AC_DEFINE_UNQUOTED([HAVE_CONDITION_TIMED_WAIT_INTERVAL],
		$HAVE_CONDITION_TIMED_WAIT_INTERVAL,
    [Define to 1 if boost::condition::timed_wait can take a time interval])
])

AC_DEFUN([PDH_CHECK_LIB], [
AC_MSG_CHECKING([for -l$1])
ok_LIBS="$LIBS"
LIBS="$LIBS -l$1"
AC_LINK_IFELSE([AC_LANG_PROGRAM()],
	[AC_MSG_RESULT([yes])],
	[AC_MSG_RESULT([no]); LIBS="$ok_LIBS"])
])

AC_DEFUN([PDH_CHECK_LIBS], [
m4_foreach_w([pdh_lib], $1, [ PDH_CHECK_LIB(pdh_lib) ])
])

# These libraries are in link order; don't reorder them just to look nice!
PDH_CHECK_LIBS([z mpr pthread ws2_32])

AC_CHECK_DECLS([LOG_ERR,LOG_NOTICE,LOG_WARNING],,,[
#if HAVE_SYS_SYSLOG_H
#include <sys/syslog.h>
#endif
])

AC_CHECK_DECLS([DMX_OUT_TS_TAP,DMX_OUT_TSDEMUX_TAP],,,[
#if HAVE_LINUX_DVB_DMX_H
#include <linux/dvb/dmx.h>
#endif
])

AC_CHECK_DECLS([PARANOIA_CB_CACHEERR],,,[
#if HAVE_PARANOIA
#include <cdda_interface.h>
#include <cdda_paranoia.h>
#elif HAVE_LIBCDIOP
#include <cdio/cdio.h>
#include <cdio/cdda.h>
#include <cdio/cd_types.h>
#include <cdio/paranoia.h>
#endif
])

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

AC_LANG_PUSH([C++])
AC_CHECK_SIZEOF([boost::mutex],,[[#include <boost/thread/mutex.hpp>]])
AC_CHECK_SIZEOF([boost::thread],,[[#include <boost/thread/thread.hpp>]])
AC_CHECK_SIZEOF([boost::condition],,[[#include <boost/thread/condition.hpp>]])
AC_CHECK_SIZEOF([boost::recursive_mutex],,[[#include <boost/thread/recursive_mutex.hpp>]])
AC_CHECK_SIZEOF([boost::mutex::scoped_lock],,[[#include <boost/thread/mutex.hpp>]])
AC_CHECK_SIZEOF([boost::recursive_mutex::scoped_lock],,[[#include <boost/thread/recursive_mutex.hpp>]])
AC_LANG_POP

PDH_CHECK_CONDITION_TIMED_WAIT

AC_DEFUN([PDH_CHECK_SOCKOPT], [
AC_MSG_CHECKING([for $1])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[$pdh_all_headers]],
[[
    int flag = 1;
    return setsockopt(0, IPPROTO_TCP, $1, (const char*)&flag, sizeof(flag));
]])], [AC_MSG_RESULT([yes]) ; ac_pdh_sockopt=1],
      [AC_MSG_RESULT([no]) ; ac_pdh_sockopt=0])
AC_DEFINE_UNQUOTED([HAVE_$1], $ac_pdh_sockopt, [Define to 1 if you have the socket option $1, or to 0 if you don't])
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
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#include <boost/cerrno.hpp>
]],
[[
    return (errno == $1);
]])], [AC_MSG_RESULT([yes]) ; ac_pdh_errno=1],
      [AC_MSG_RESULT([no]) ; ac_pdh_errno=0])
AC_DEFINE_UNQUOTED([HAVE_$1], $ac_pdh_errno, [Define to 1 if you have the error-code $1, or to 0 if you don't])
AC_LANG_POP
])

PDH_CHECK_ERRNO([EINVAL])
PDH_CHECK_ERRNO([EISCONN])
PDH_CHECK_ERRNO([EALREADY])
PDH_CHECK_ERRNO([ECANCELED])
PDH_CHECK_ERRNO([EOPNOTSUPP])
PDH_CHECK_ERRNO([EINPROGRESS])
PDH_CHECK_ERRNO([EWOULDBLOCK])

AC_DEFINE([HAVE_DVB], (HAVE_LINUX_DVB_DMX_H && HAVE_LINUX_DVB_FRONTEND_H), [Define to 1 if you have the Linux DVB API, or to 0 if you don't])

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
test "${QT}" = "yes"        || printf "** Install Qt 4.4.0 or later to build optional choralecd (4.5.0 for LGPL)\n   http://www.troll.no\n"
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
