TOP?=../
DIR:=choraleutil/
SRCS:=

all-choraleutil:

all: all-choraleutil

include $(TOP)Make.common

play:=$(TOP)$(DIR)play$(DOTEXE)
PLAY_OBJS:=$(TOP)$(DIR)$(TARGETDIR)play.lo

wall:=$(TOP)$(DIR)wall$(DOTEXE)
WALL_OBJS:=$(TOP)$(DIR)$(TARGETDIR)wall.lo

pparse:=$(TOP)$(DIR)pparse$(DOTEXE)
PPARSE_OBJS:=$(TOP)$(DIR)$(TARGETDIR)pparse.lo

ifneq ($(TARGET),i586-mingw32)
playlist2dir:=$(TOP)$(DIR)playlist2dir$(DOTEXE)
PLAYLIST2DIR_OBJS:=$(TOP)$(DIR)$(TARGETDIR)playlist2dir.lo

tcp:=$(TOP)$(DIR)tcp
TCP_OBJS:=$(TOP)$(DIR)$(TARGETDIR)tcp.lo
endif

ifneq ($(TAGLIB),no)
protocoltool:=$(TOP)$(DIR)protocoltool$(DOTEXE)
PROTOCOLTOOL_OBJS:=$(TOP)$(DIR)$(TARGETDIR)protocoltool.lo

doctor:=$(TOP)$(DIR)doctor$(DOTEXE)
DOCTOR_OBJS:=$(TOP)$(DIR)$(TARGETDIR)doctor.lo
endif

readtags:=$(TOP)$(DIR)readtags$(DOTEXE)
READTAGS_OBJS:=$(TOP)$(DIR)$(TARGETDIR)readtags.lo

ifneq ($(TARGET),i586-mingw32)
squirt:=$(TOP)$(DIR)squirt$(DOTEXE)
SQUIRT_OBJS:=$(TOP)$(DIR)$(TARGETDIR)squirt.lo
endif

timedb:=$(TOP)$(DIR)timedb$(DOTEXE)
TIMEDB_OBJS:=$(TOP)$(DIR)$(TARGETDIR)timedb.lo

CUTIL_OBJS:=$(READTAGS_OBJS) $(PLAY_OBJS) $(PPARSE_OBJS) $(PLAYLIST2DIR_OBJS) \
		$(SQUIRT_OBJS) $(TIMEDB_OBJS) $(PROTOCOLTOOL_OBJS)
CUTIL_DEPS:=$(CUTIL_OBJS:%.lo=%.lo.dep)

all-choraleutil: $(pparse) $(playlist2dir) $(readtags) $(play) $(squirt) \
	$(timedb) $(protocoltool) $(tcp) $(doctor) $(wall)

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
ifeq ($(libkarma),)
include $(TOP)libkarma/Makefile
endif
ifeq ($(libdbsteam),)
include $(TOP)libdbsteam/Makefile
endif
ifeq ($(libmediadb),)
include $(TOP)libmediadb/Makefile
endif
ifeq ($(libupnp),)
include $(TOP)libupnp/Makefile
endif
ifeq ($(libdbempeg),)
include $(TOP)libdbempeg/Makefile
endif
ifeq ($(libimport),)
include $(TOP)libimport/Makefile
endif
ifeq ($(liboutput),)
include $(TOP)liboutput/Makefile
endif
ifeq ($(libdblocal),)
include $(TOP)libdblocal/Makefile
endif

$(play): $(PLAY_OBJS) $(liboutput) $(libdb) $(libutil) \
		$(libdbsteam) $(libmediadb) $(libimport) $(libdblocal)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --silent \
		$(CXX) -o $@ $(LDFLAGS) $(PLAY_OBJS) \
		$(liboutput) $(libdblocal) $(libdbsteam) $(libmediadb) \
		$(libdb) $(libimport) $(libutil) $(LIBFLAC_LIBS) \
		$(BOOST_LIBS) $(TAGLIB_LIBS) \
		$(GSTREAMER_LIBS) $(LIBS)

$(tcp): $(TCP_OBJS) $(libutil)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --silent \
		 $(CXX) -o $@ $(TCP_OBJS) $(libutil) \
		 $(BOOST_LIBS) $(LIBS)

$(wall): $(WALL_OBJS) $(libutil)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --silent \
		 $(CXX) -o $@ $(WALL_OBJS) $(libutil) \
		 $(BOOST_LIBS) $(LIBS)

$(pparse): $(PPARSE_OBJS) $(libimport) $(libutil) Makefile
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --silent \
		 $(CXX) -o $@ $(LDFLAGS) $(PPARSE_OBJS) \
		 $(libimport) $(libutil) \
		 $(BOOST_LIBS) $(LIBS)

$(protocoltool): $(PROTOCOLTOOL_OBJS) \
		$(libdbempeg) $(libimport) $(libdblocal) \
		$(libmediadb) $(libdbsteam) $(libdb) $(libempeg) $(libkarma) \
		$(libupnp) $(libutil)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --silent \
		$(CXX) -o $@ $(LDFLAGS) $(PROTOCOLTOOL_OBJS) \
		$(libdbempeg) $(libdblocal) $(libimport) \
		$(libmediadb) $(libdbsteam) $(libdb) $(libempeg) $(libkarma) \
		$(libupnp) $(libutil) \
		$(BOOST_LIBS) $(TAGLIB_LIBS) $(LIBS)
	@$(STRIP) $@

$(doctor): $(DOCTOR_OBJS) \
		$(libdbempeg) $(libimport) $(libdblocal) \
		$(libmediadb) $(libdbsteam) $(libdb) $(libempeg) $(libutil)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --silent \
		$(CXX) -o $@ $(LDFLAGS) $(DOCTOR_OBJS) \
		$(libdbempeg) $(libdblocal) $(libimport) \
		$(libmediadb) $(libdbsteam) $(libdb) $(libempeg) $(libutil) \
		$(BOOST_LIBS) $(TAGLIB_LIBS) $(LIBS)
	@$(STRIP) $@

$(playlist2dir): $(PLAYLIST2DIR_OBJS) $(libimport) $(libutil)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		 $(CXX) -o $@ $(PLAYLIST2DIR_OBJS) $(libimport) $(libutil) \
		 $(BOOST_LIBS) $(LIBS)

$(readtags): $(READTAGS_OBJS) $(libimport) $(libdb) $(libutil) \
		$(libdbsteam) $(libmediadb) $(libdblocal)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --silent \
		$(CXX) -o $@ $(LDFLAGS) $(READTAGS_OBJS) $(libdblocal) \
		$(libimport) $(libdbsteam) $(libmediadb) \
		$(libdb) $(libutil) \
		$(BOOST_LIBS) $(TAGLIB_LIBS) $(LIBS)

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
	$(PLAYLIST2DIR_OBJS) $(readtags) $(play) $(squirt) $(timedb) \
	$(tcp) $(doctor)

-include $(CUTIL_DEPS)
