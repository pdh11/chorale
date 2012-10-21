TOP?=../
DIR:=liboutput/
SRCS:=\
	flac.cpp \
	oss.cpp \
	queue.cpp \
	upnp.cpp \
	urlplayer.cpp

all-liboutput:

all: all-liboutput

include $(TOP)Make.common

LIBOUTPUT_OBJS:=$(LOBJS)

liboutput:=$(TOP)$(DIR)$(TARGETDIR)liboutput.la

all-liboutput: $(liboutput)

$(TOP)$(DIR)% : CXXFLAGS += $(TAGLIB_CXXFLAGS) $(BOOST_CXXFLAGS) \
	$(LIBCDIOP_CXXFLAGS) $(LIBFLAC_CXXFLAGS) $(LIBXMLPP_CXXFLAGS) \
	$(GSTREAMER_CXXFLAGS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif

$(liboutput): $(LIBOUTPUT_OBJS)
	$(AR_PRE)libtool --tag=CXX --mode=link --quiet \
		 $(CC) -o $@ $(LIBOUTPUT_OBJS)

CLEANS += $(liboutput)
