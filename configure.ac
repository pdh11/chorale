#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(chorale, [0.01], pdh@utter.chaos.org.uk)

AC_MSG_NOTICE([Required: Taglib, Boost, xsltproc, and libxml++.])
AC_MSG_NOTICE([Optional: qt, gstreamer, libcdio, libcddb, lame, flac, curl, libupnp, dbus, hal, libuuid, openssl.])
AC_MSG_NOTICE([])

AC_CONFIG_SRCDIR([libdb/db.h])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_AUX_DIR([autotools])

# Checks for programs.
AC_PROG_CC
AC_GNU_SOURCE

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
PDH_C_WARNING([-Wcast-align])
PDH_C_WARNING([-Wconversion])
PDH_C_WARNING([-Wwrite-strings])
PDH_C_WARNING([-Wpointer-arith])
PDH_C_WARNING([-Wstrict-prototypes])
PDH_C_WARNING([-Wmissing-prototypes])
PDH_C_WARNING([-Wdeclaration-after-statement])
PDH_C_WARNING([-fdiagnostics-show-option])

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
PDH_CXX_WARNING([-Wcast-align])
PDH_CXX_WARNING([-Wconversion])
PDH_CXX_WARNING([-Wwrite-strings])
PDH_CXX_WARNING([-Wpointer-arith])
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
AC_PATH_PROG(XSLTPROC, xsltproc, no)
if test "$XSLTPROC" = "no"; then
   HAVE_XSLTPROC="no"
else
   HAVE_XSLTPROC="yes"
fi

# Checks for libraries.

AC_CHECK_HEADERS([sys/inotify.h])
AC_CHECK_FUNCS([inotify_init])

AC_CHECK_LIB(mp3lame, lame_init, [LAME="yes" LAME_LIBS="-lmp3lame"], [LAME="no"], -lm)
if test "${LAME}" = "yes" ; then
    AC_CHECK_HEADER(lame/lame.h, , [LAME="no"])
fi
AC_SUBST(LAME_LIBS)

XIPH_PATH_OGG()

PKG_CHECK_MODULES(QT, QtGui >= 4.3.3 Qt3Support >= 4.3.3 QtCore >= 4.3.3, [QT="yes"], [QT="no"])
AC_SUBST(QT)
AC_SUBST(QT_CFLAGS)
AC_SUBST(QT_LIBS)
if test "${QT}" = "yes" ; then
  QT_MOC=`pkg-config --variable=moc_location QtGui`
  AC_SUBST(QT_MOC)
fi

PKG_CHECK_MODULES(HAL, hal >= 0.5.9.1, [HAL="yes"], [HAL="no"])
AC_SUBST(HAL)
AC_SUBST(HAL_CFLAGS)
AC_SUBST(HAL_LIBS)

PKG_CHECK_MODULES(UPNP, libupnp >= 1.6.0, [UPNP="yes"], [UPNP="no"])
AC_SUBST(UPNP)
AC_SUBST(UPNP_CFLAGS)
AC_SUBST(UPNP_LIBS)

PKG_CHECK_MODULES(CURL, libcurl >= 7.16.1, [CURL="yes"], [CURL="no"])
AC_SUBST(CURL)
AC_SUBST(CURL_CFLAGS)
AC_SUBST(CURL_LIBS)

PKG_CHECK_MODULES(DBUS, dbus-1 >= 1.1.2, [DBUS="yes"], [DBUS="no"])
AC_SUBST(DBUS)
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)

PKG_CHECK_MODULES(SQLITE, sqlite3 >= 3.3, [SQLITE="yes"], [SQLITE="no"])
AC_SUBST(SQLITE)
AC_SUBST(SQLITE_CFLAGS)
AC_SUBST(SQLITE_LIBS)

PKG_CHECK_MODULES(TAGLIB, taglib >= 1.4, [TAGLIB="yes"], [TAGLIB="no"])
AC_SUBST(TAGLIB)
AC_SUBST(TAGLIB_CFLAGS)
AC_SUBST(TAGLIB_LIBS)

PKG_CHECK_MODULES(LIBCDDB, libcddb >= 1.3.0, [LIBCDDB="yes"], [LIBCDDB="no"])
AC_SUBST(LIBCDDB)
AC_SUBST(LIBCDDB_CFLAGS)
AC_SUBST(LIBCDDB_LIBS)

PKG_CHECK_MODULES(LIBUUID, uuid >= 1.00, [LIBUUID="yes"], [LIBUUID="no"])
AC_SUBST(LIBUUID)
AC_SUBST(LIBUUID_CFLAGS)
AC_SUBST(LIBUUID_LIBS)

PKG_CHECK_MODULES(OPENSSL, openssl >= 0.9.0, [OPENSSL="yes"], [OPENSSL="no"])
AC_SUBST(OPENSSL)
AC_SUBST(OPENSSL_CFLAGS)
AC_SUBST(OPENSSL_LIBS)

PKG_CHECK_MODULES(LIBCDIOP, libcdio_paranoia >= 0.77, [LIBCDIOP="yes"], [LIBCDIOP="no"])
AC_SUBST(LIBCDIOP)
AC_SUBST(LIBCDIOP_CFLAGS)
AC_SUBST(LIBCDIOP_LIBS)

PKG_CHECK_MODULES(LIBXMLPP, libxml++-2.6 >= 2.14.0, [LIBXMLPP="yes"], [LIBXMLPP="no"])
AC_SUBST(LIBXMLPP)
AC_SUBST(LIBXMLPP_CFLAGS)
AC_SUBST(LIBXMLPP_LIBS)

PKG_CHECK_MODULES(GSTREAMER, gstreamer-0.10 >= 0.10.13, [GSTREAMER="yes"], [GSTREAMER="no"])
AC_SUBST(GSTREAMER)
AC_SUBST(GSTREAMER_CFLAGS)
AC_SUBST(GSTREAMER_LIBS)

AM_PATH_LIBFLAC([LIBFLAC="yes"],[LIBFLAC="no"])
AC_LANG_PUSH(C++)
AM_PATH_LIBFLACPP([LIBFLACPP="yes"],[LIBFLACPP="no"])
AC_LANG_POP()

#PKG_CHECK_MODULES(LIBFLAC, flac >= 1.2.0, [LIBFLAC="yes"], [LIBFLAC="no"])
AC_SUBST(LIBFLAC)
AC_SUBST(LIBFLAC_CFLAGS)
AC_SUBST(LIBFLAC_LIBS)

#PKG_CHECK_MODULES(LIBFLACPP, flac++ >= 1.2.0, [LIBFLACPP="yes"], [LIBFLACPP="no"])
AC_SUBST(LIBFLACPP)
AC_SUBST(LIBFLACPP_CFLAGS)
AC_SUBST(LIBFLACPP_LIBS)

# ax_boost.m4 is AllPermissive
AX_BOOST([1.33.1],[BOOST="yes"],[BOOST="no"])

# Checks for header files.
AC_CHECK_HEADERS([stdint.h unistd.h sys/types.h])

# Checks for typedefs, structures, and compiler characteristics.
#AC_HEADER_STDBOOL
#AC_C_CONST
#AC_C_INLINE

# Optional libraries
printf "\n"
if test "${LIBCDIOP}${LIBCDDB}${QT}${LAME}${LIBFLAC}${CURL}${UPNP}${GSTREAMER}${DBUS}${HAL}${SQLITE}${LIBUUID}${OPENSSL}" != "yesyesyesyesyesyesyesyesyesyesyesyesyes" ; then
test "${LIBFLAC}" = "yes"   || printf "** Install flac 1.2.0 or later to build optional choralecd\n   http://flac.sourceforge.net\n"
test "${LAME}" = "yes"      || printf "** Install Lame 3.96.1 or later to build optional choralecd\n   http://lame.sourceforge.net\n"
test "${LIBCDIOP}" = "yes"  || printf "** Install libcdio 0.77 or later to build optional choralecd\n   http://www.gnu.org/software/libcdio\n"
test "${LIBCDDB}" = "yes"   || printf "** Install libcddb 1.3.0 or later to build optional choralecd\n   http://libcddb.sourceforge.net\n"
test "${QT}" = "yes"        || printf "** Install Qt 4.3.3 or later to build optional choralecd\n   http://www.troll.no\n"
test "${CURL}" = "yes"      || printf "** Install curl 7.16.1 or later to build optional Rio Receiver client\n   http://curl.haxx.se\n"
test "${UPNP}" = "yes"      || printf "** Install libupnp-1.6.0 or later to build optional UPnP support\n   http://pupnp.sf.net\n"
test "${GSTREAMER}" = "yes" || printf "** Install gstreamer-0.10.13 or later to build optional choralecd\n   http://gstreamer.freedesktop.org\n"
test "${DBUS}" = "yes"      || printf "** Install libdbus-1.0.0 or later to build optional auto-configuration\n   http://dbus.freedesktop.org\n"
test "${HAL}" = "yes"       || printf "** Install hal-0.5.9.1 or later to build optional auto-configuration\n   http://hal.freedesktop.org\n"
test "${SQLITE}" = "yes"    || printf "** Install sqlite 3.3 or later to build optional SQL support\n   http://www.sqlite.org\n"
test "${LIBUUID}" = "yes"   || printf "** Install libuuid 1.00 or later to build optional UPnP support\n   http://e2fsprogs.sourceforge.net\n"
test "${OPENSSL}" = "yes"   || printf "** Install openssl 0.9.0 or later to build optional UPnP support\n   http://www.openssl.org\n"
printf "\n"
fi

# Required libraries
if test "${TAGLIB}${BOOST}${LIBXMLPP}${HAVE_XSLTPROC}" != "yesyesyesyes" ; then
test "${TAGLIB}" = "yes"   || printf "** Taglib 1.4 or later is required\n   http://developer.kde.org/~wheeler/taglib.html\n"
test "${BOOST}" = "yes"    || printf "** Boost 1.33.1 or later is required\n   http://www.boost.org\n"
test "${LIBXMLPP}" = "yes" || printf "** Libxml++ 2.14.0 or later is required\n   http://libxmlplusplus.sourceforge.net\n"
test "${HAVE_XSLTPROC}" = "yes" || printf "** Xsltproc from libxslt 1.0.0 or later is required\n  http://xmlsoft.org/XSLT/index.html\n"
printf "\n"
AC_MSG_ERROR([Library dependencies not met])
fi

AC_CONFIG_FILES([Make.config stamp-h])
AC_OUTPUT
