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

readtags:=$(TOP)$(DIR)readtags
READTAGS_OBJS:=$(TOP)$(DIR)$(TARGETDIR)readtags.lo

ifneq ($(TARGET),i586-mingw32)
squirt:=$(TOP)$(DIR)squirt
SQUIRT_OBJS:=$(TOP)$(DIR)$(TARGETDIR)squirt.lo
endif

timedb:=$(TOP)$(DIR)timedb
TIMEDB_OBJS:=$(TOP)$(DIR)$(TARGETDIR)timedb.lo

CUTIL_OBJS:=$(READTAGS_OBJS) $(PLAY_OBJS) $(PPARSE_OBJS) $(PLAYLIST2DIR_OBJS) \
		$(SQUIRT_OBJS) $(TIMEDB_OBJS)
CUTIL_DEPS:=$(CUTIL_OBJS:%.lo=%.lo.dep)

all-choraleutil: $(pparse) $(playlist2dir) $(readtags) $(play) $(squirt) \
	$(timedb)

$(TOP)$(DIR)% : CXXFLAGS += $(TAGLIB_CXXFLAGS) $(BOOST_CXXFLAGS) \
	$(LIBCDIOP_CXXFLAGS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif
ifeq ($(libdb),)
include $(TOP)libdb/Makefile
endif
ifeq ($(libmediadb),)
include $(TOP)libmediadb/Makefile
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

$(play): $(PLAY_OBJS) $(liboutput) $(libdb) $(libutil) \
		Makefile $(libdbsteam) $(libmediadb) $(libimport)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CXX) -o $@ $(PLAY_OBJS) \
		$(liboutput) $(libdbsteam) $(libmediadb) \
		$(libdb) $(libimport) $(libutil) $(LIBFLAC_LIBS) \
		$(BOOST_LIBS) $(TAGLIB_LIBS) \
		$(GSTREAMER_LIBS) $(LIBS)

$(pparse): $(PPARSE_OBJS) $(libimport) $(libutil) Makefile
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --silent \
		 $(CXX) -o $@ $(PPARSE_OBJS) $(libimport) $(libutil) \
		 $(BOOST_LIBS) $(LIBS)

$(playlist2dir): $(PLAYLIST2DIR_OBJS) $(libimport) $(libutil) Makefile
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		 $(CXX) -o $@ $(PLAYLIST2DIR_OBJS) $(libimport) $(libutil) \
		 $(BOOST_LIBS) $(LIBS)

$(readtags): $(READTAGS_OBJS) $(libimport) $(libdb) $(libutil) \
		Makefile $(libdbsteam) $(libmediadb)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CXX) -o $@ $(READTAGS_OBJS) \
		$(libimport) $(libdbsteam) $(libmediadb) \
		$(libdb) $(libutil) \
		$(BOOST_LIBS) $(TAGLIB_LIBS) $(LIBS)

$(squirt): $(SQUIRT_OBJS) $(libimport) $(libdb) $(libutil) \
		$(libdbsteam) $(libmediadb)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CXX) -o $@ $(SQUIRT_OBJS) \
		$(libimport) $(libdbsteam) $(libmediadb) \
		$(libdb) $(libutil) \
		$(BOOST_LIBS) $(TAGLIB_LIBS) $(LIBS)

$(timedb): $(TIMEDB_OBJS) $(libimport) $(libdb) $(libutil) \
		Makefile $(libdbsteam) $(libmediadb)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CXX) -o $@ $(CXXFLAGS) $(TIMEDB_OBJS) \
		$(libimport) $(libdbsteam) $(libmediadb) \
		$(libdb) $(libutil) \
		$(BOOST_LIBS) $(TAGLIB_LIBS) $(LIBS)

CLEANS += $(pparse) $(playlist2dir) $(PPARSE_OBJS) \
	$(PLAYLIST2DIR_OBJS) $(readtags) $(play) $(squirt) $(timedb)

-include $(CUTIL_DEPS)
