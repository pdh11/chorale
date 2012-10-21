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

ifneq ($(TARGET),i586-mingw32)
playlist2dir:=$(TOP)$(DIR)playlist2dir
PLAYLIST2DIR_OBJS:=$(TOP)$(DIR)$(TARGETDIR)playlist2dir.lo
endif

protocoltool:=$(TOP)$(DIR)protocoltool
PROTOCOLTOOL_OBJS:=$(TOP)$(DIR)$(TARGETDIR)protocoltool.lo

readtags:=$(TOP)$(DIR)readtags
READTAGS_OBJS:=$(TOP)$(DIR)$(TARGETDIR)readtags.lo

ifneq ($(TARGET),i586-mingw32)
squirt:=$(TOP)$(DIR)squirt
SQUIRT_OBJS:=$(TOP)$(DIR)$(TARGETDIR)squirt.lo
endif

timedb:=$(TOP)$(DIR)timedb
TIMEDB_OBJS:=$(TOP)$(DIR)$(TARGETDIR)timedb.lo

CUTIL_OBJS:=$(READTAGS_OBJS) $(PLAY_OBJS) $(PPARSE_OBJS) $(PLAYLIST2DIR_OBJS) \
		$(SQUIRT_OBJS) $(TIMEDB_OBJS) $(PROTOCOLTOOL_OBJS)
CUTIL_DEPS:=$(CUTIL_OBJS:%.lo=%.lo.dep)

all-choraleutil: $(pparse) $(playlist2dir) $(readtags) $(play) $(squirt) \
	$(timedb) $(protocoltool)

$(TOP)$(DIR)% : CXXFLAGS += $(TAGLIB_CXXFLAGS) $(BOOST_CXXFLAGS) \
	$(LIBCDIOP_CXXFLAGS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif
ifeq ($(libdb),)
include $(TOP)libdb/Makefile
endif
ifeq ($(libempeg),)
include $(TOP)libempeg/Makefile
endif
ifeq ($(libdbempeg),)
include $(TOP)libdbempeg/Makefile
endif
ifeq ($(libmediadb),)
include $(TOP)libmediadb/Makefile
endif
ifeq ($(libdbsteam),)
include $(TOP)libdbsteam/Makefile
endif
ifeq ($(libdblocal),)
include $(TOP)libdblocal/Makefile
endif
ifeq ($(libimport),)
include $(TOP)libimport/Makefile
endif
ifeq ($(liboutput),)
include $(TOP)liboutput/Makefile
endif

$(play): $(PLAY_OBJS) $(liboutput) $(libdb) $(libutil) \
		$(libdbsteam) $(libmediadb) $(libimport) $(libdblocal)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CXX) -o $@ $(PLAY_OBJS) \
		$(liboutput) $(libdblocal) $(libdbsteam) $(libmediadb) \
		$(libdb) $(libimport) $(libutil) $(LIBFLAC_LIBS) \
		$(BOOST_LIBS) $(TAGLIB_LIBS) \
		$(GSTREAMER_LIBS) $(LIBS)

$(pparse): $(PPARSE_OBJS) $(libimport) $(libutil)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --silent \
		 $(CXX) -o $@ $(PPARSE_OBJS) $(libimport) $(libutil) \
		 $(BOOST_LIBS) $(LIBS)

$(protocoltool): $(PROTOCOLTOOL_OBJS) \
		$(libdbempeg) $(libimport) $(libdblocal) \
		$(libmediadb) $(libdbsteam) $(libdb) $(libempeg) $(libutil)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --silent \
		$(CXX) -o $@ $(CXXFLAGS) $(PROTOCOLTOOL_OBJS) \
		$(libdbempeg) $(libdblocal) $(libimport) \
		$(libmediadb) $(libdbsteam) $(libdb) $(libempeg) $(libutil) \
		$(BOOST_LIBS) $(TAGLIB_LIBS) $(LIBS)

$(playlist2dir): $(PLAYLIST2DIR_OBJS) $(libimport) $(libutil)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		 $(CXX) -o $@ $(PLAYLIST2DIR_OBJS) $(libimport) $(libutil) \
		 $(BOOST_LIBS) $(LIBS)

$(readtags): $(READTAGS_OBJS) $(libimport) $(libdb) $(libutil) \
		$(libdbsteam) $(libmediadb) $(libdblocal)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CXX) -o $@ $(READTAGS_OBJS) $(libdblocal) \
		$(libimport) $(libdbsteam) $(libmediadb) \
		$(libdb) $(libutil) \
		$(BOOST_LIBS) $(TAGLIB_LIBS) $(LIBS) -Wl,-Map,map.txt

$(squirt): $(SQUIRT_OBJS) $(libimport) $(libdb) $(libutil) \
		$(libdblocal) $(libdbsteam) $(libmediadb)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CXX) -o $@ $(SQUIRT_OBJS) \
		$(libdblocal) $(libimport) $(libdbsteam) $(libmediadb) \
		$(libdb) $(libutil) \
		$(BOOST_LIBS) $(TAGLIB_LIBS) $(LIBS)

$(timedb): $(TIMEDB_OBJS) $(libimport) $(libdb) $(libutil) \
		$(libdbsteam) $(libmediadb)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CXX) -o $@ $(CXXFLAGS) $(TIMEDB_OBJS) \
		$(libimport) $(libdbsteam) $(libmediadb) \
		$(libdb) $(libutil) \
		$(BOOST_LIBS) $(TAGLIB_LIBS) $(LIBS)

CLEANS += $(pparse) $(playlist2dir) $(PPARSE_OBJS) $(protocoltool) \
	$(PLAYLIST2DIR_OBJS) $(readtags) $(play) $(squirt) $(timedb)

-include $(CUTIL_DEPS)
