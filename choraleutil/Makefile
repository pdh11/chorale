TOP?=../
DIR:=choraleutil/
SRCS:=

all-choraleutil:

all: all-choraleutil

include $(TOP)Make.common

play:=$(TOP)$(DIR)play
PLAY_OBJS:=$(TOP)$(DIR)$(TARGETDIR)play.lo

pparse:=$(TOP)$(DIR)pparse
PPARSE_OBJS:=$(TOP)$(DIR)$(TARGETDIR)pparse.lo

playlist2dir:=$(TOP)$(DIR)playlist2dir
PLAYLIST2DIR_OBJS:=$(TOP)$(DIR)$(TARGETDIR)playlist2dir.lo

readtags:=$(TOP)$(DIR)readtags
READTAGS_OBJS:=$(TOP)$(DIR)$(TARGETDIR)readtags.lo

squirt:=$(TOP)$(DIR)squirt
SQUIRT_OBJS:=$(TOP)$(DIR)$(TARGETDIR)squirt.lo

timedb:=$(TOP)$(DIR)timedb
TIMEDB_OBJS:=$(TOP)$(DIR)$(TARGETDIR)timedb.lo

CUTIL_OBJS:=$(READTAGS_OBJS) $(PLAY_OBJS) $(PPARSE_OBJS) $(PLAYLIST2DIR_OBJS)
CUTIL_DEPS:=$(CUTIL_OBJS:%.lo=%.lo.dep)

all-choraleutil: $(pparse) $(playlist2dir) $(readtags) $(play) $(squirt) \
	$(timedb)

$(TOP)$(DIR)% : CXXFLAGS += $(TAGLIB_CXXFLAGS) $(BOOST_CXXFLAGS) \
	$(LIBCDIOP_CXXFLAGS) $(LIBXMLPP_CXXFLAGS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif
ifeq ($(libdb),)
include $(TOP)libdb/Makefile
endif
ifeq ($(libmediadb),)
include $(TOP)libmediadb/Makefile
endif
ifeq ($(libdbsqlite),)
include $(TOP)libdbsqlite/Makefile
endif
ifeq ($(libdbsteam),)
include $(TOP)libdbsteam/Makefile
endif
ifeq ($(libimport),)
include $(TOP)libimport/Makefile
endif
ifeq ($(liboutput),)
include $(TOP)liboutput/Makefile
endif

$(play): $(PLAY_OBJS) $(liboutput) $(libdbsqlite) $(libdb) $(libutil) \
		Makefile $(libdbsteam) $(libmediadb) $(libimport)
	$(LD_PRE)libtool --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(PLAY_OBJS) \
		$(liboutput) $(libdbsqlite) $(libdbsteam) $(libmediadb) \
		$(libdb) $(libimport) $(libutil) $(LIBFLAC_LIBS) \
		$(BOOST_LIBS) $(TAGLIB_LIBS) $(SQLITE_LIBS) $(LIBXMLPP_LIBS) \
		$(GSTREAMER_LIBS)

$(pparse): $(PPARSE_OBJS) $(libimport) $(libutil) Makefile
	$(LD_PRE)libtool --tag=CXX --mode=link --quiet \
		 $(CC) -o $@ $(PPARSE_OBJS) $(libimport) $(libutil) \
		 $(LIBXMLPP_LIBS) $(BOOST_LIBS)

$(playlist2dir): $(PLAYLIST2DIR_OBJS) $(libimport) $(libutil) Makefile
	$(LD_PRE)libtool --tag=CXX --mode=link --quiet \
		 $(CC) -o $@ $(PLAYLIST2DIR_OBJS) $(libimport) $(libutil) \
		 $(LIBXMLPP_LIBS) $(BOOST_LIBS)

$(readtags): $(READTAGS_OBJS) $(libimport) $(libdbsqlite) $(libdb) $(libutil) \
		Makefile $(libdbsteam) $(libmediadb)
	$(LD_PRE)libtool --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(READTAGS_OBJS) \
		$(libimport) $(libdbsqlite) $(libdbsteam) $(libmediadb) \
		$(libdb) $(libutil) \
		$(BOOST_LIBS) $(TAGLIB_LIBS) $(SQLITE_LIBS) $(LIBXMLPP_LIBS)

$(squirt): $(SQUIRT_OBJS) $(libimport) $(libdbsqlite) $(libdb) $(libutil) \
		Makefile $(libdbsteam) $(libmediadb)
	$(LD_PRE)libtool --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(SQUIRT_OBJS) \
		$(libimport) $(libdbsqlite) $(libdbsteam) $(libmediadb) \
		$(libdb) $(libutil) \
		$(BOOST_LIBS) $(TAGLIB_LIBS) $(SQLITE_LIBS) $(LIBXMLPP_LIBS)

$(timedb): $(TIMEDB_OBJS) $(libimport) $(libdbsqlite) $(libdb) $(libutil) \
		Makefile $(libdbsteam) $(libmediadb)
	$(LD_PRE)libtool --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(CXXFLAGS) $(TIMEDB_OBJS) \
		$(libimport) $(libdbsqlite) $(libdbsteam) $(libmediadb) \
		$(libdb) $(libutil) \
		$(BOOST_LIBS) $(TAGLIB_LIBS) $(SQLITE_LIBS) $(LIBXMLPP_LIBS)

CLEANS += $(pparse) $(playlist2dir) $(PPARSE_OBJS) \
	$(PLAYLIST2DIR_OBJS) $(readtags) $(play) $(squirt) $(timedb)

-include $(CUTIL_DEPS)
