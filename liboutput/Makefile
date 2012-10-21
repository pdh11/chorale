TOP?=../
DIR:=liboutput/
SRCS:=\
	gstreamer.cpp \
	queue.cpp \
	registry.cpp \
	upnpav.cpp

ifeq ($(DARWIN),yes)
SRCS += avfoundation.mm
endif

TEST_SRCS:=\
	queue.cpp

#	flac.cpp \
#	oss.cpp \
#

all-liboutput:

all: all-liboutput

include $(TOP)Make.common

LIBOUTPUT_OBJS:=$(LOBJS)
LIBOUTPUT_TESTS:=$(TESTS)

liboutput:=$(TOP)$(DIR)$(TARGETDIR)liboutput.a

all-liboutput: $(liboutput)

$(TOP)$(DIR)% : CXXFLAGS += $(TAGLIB_CXXFLAGS) $(BOOST_CXXFLAGS) \
	$(LIBCDIOP_CXXFLAGS) $(GSTREAMER_CXXFLAGS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif
ifeq ($(libmediadb),)
include $(TOP)libmediadb/Makefile
endif

$(LIBOUTPUT_TESTS) : TEST_LIBS += $(liboutput) $(libmediadb) $(libdbsteam) \
	$(libdb) $(libutil) $(BOOST_LIBS)

$(LIBOUTPUT_TESTS) : $(liboutput) $(libmediadb) $(libdbsteam) $(libdb) \
				$(libutil)

$(liboutput): $(LIBOUTPUT_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		 $(CC) -o $@ $(LIBOUTPUT_OBJS)

CLEANS += $(liboutput)
