TOP?=../
DIR:=libempeg/
SRCS:=\
	crc16.cpp \
	discovery.cpp \
	fid_stream.cpp \
	protocol_client.cpp

TEST_SRCS:=\
	discovery.cpp \
	protocol_client.cpp

all-libempeg:

all: all-libempeg

include $(TOP)Make.common

LIBEMPEG_OBJS:=$(LOBJS)
LIBEMPEG_TESTS:=$(TESTS)

libempeg:=$(TOP)$(DIR)$(TARGETDIR)libempeg.a

all-libempeg: $(libempeg)

$(TOP)$(DIR)% : CXXFLAGS += $(TAGLIB_CXXFLAGS) $(BOOST_CXXFLAGS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif

$(LIBEMPEG_TESTS) : TEST_LIBS += $(libempeg) $(libutil) $(BOOST_LIBS) $(LIBS)

$(LIBEMPEG_TESTS) : $(libempeg) $(libutil)

$(libempeg): $(LIBEMPEG_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(LIBEMPEG_OBJS)

CLEANS += $(libempeg)
