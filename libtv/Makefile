TOP?=../
DIR:=libtv/
SRCS:=\
	database.cpp \
	dvb.cpp \
	dvb_audio_stream.cpp \
	dvb_video_stream.cpp \
	dvb_recording.cpp \
	dvb_service.cpp \
	epg.cpp \
	epg_database.cpp \
	mp3_stream.cpp \
	program_stream.cpp \
	recording.cpp \
	web_epg.cpp

TEST_SRCS:= \
	dvb.cpp \
	dvb_recording.cpp \
	epg.cpp

all-libtv:

all: all-libtv

include $(TOP)Make.common

LIBTV_OBJS:=$(LOBJS)
LIBTV_TESTS:=$(TESTS)

libtv:=$(TOP)$(DIR)$(TARGETDIR)libtv.a

tsdemux:=$(TOP)$(DIR)tsdemux$(DOTEXE)
TSDEMUX_OBJS:=$(TOP)$(DIR)$(TARGETDIR)tsdemux.lo

all-libtv: $(libtv) $(tsdemux)

$(TOP)$(DIR)% : CXXFLAGS += $(TAGLIB_CXXFLAGS) $(BOOST_CXXFLAGS) \
	$(LIBCDIOP_CXXFLAGS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif
ifeq ($(libdb),)
include $(TOP)libdb/Makefile
endif
ifeq ($(libdbsteam),)
include $(TOP)libdbsteam/Makefile
endif

$(LIBTV_TESTS) : TEST_LIBS += $(libtv) $(libdbsteam) $(libdb) $(libutil) \
	$(BOOST_LIBS)

$(LIBTV_TESTS) : $(libtv) $(libdbsteam) $(libdb) $(libutil)

$(libtv): $(LIBTV_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CXX) -o $@ $(LIBTV_OBJS)

$(tsdemux): $(TSDEMUX_OBJS)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CXX) -o $@ $(LDFLAGS) $(TSDEMUX_OBJS)

CLEANS += $(libtv) $(LIBTV_OBJS) $(tsdemux)
