TOP?=../
DIR:=libreceiver/
SRCS:=\
	ssdp.cpp \
	tags.cpp

TEST_SRCS:= \
	ssdp.cpp

all-libreceiver:

all: all-libreceiver

include $(TOP)Make.common

LIBRECEIVER_OBJS:=$(LOBJS)
LIBRECEIVER_TESTS:=$(TESTS)

libreceiver:=$(TOP)$(DIR)$(TARGETDIR)libreceiver.la

all-libreceiver: $(libreceiver)

$(TOP)$(DIR)% : CXXFLAGS += $(TAGLIB_CXXFLAGS) $(BOOST_CXXFLAGS) \
	$(LIBCDIOP_CXXFLAGS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif

$(LIBRECEIVER_TESTS) : TEST_LIBS+= $(BOOST_LIBS) $(LIBS)

$(LIBRECEIVER_TESTS) : $(libutil) 

$(libreceiver): $(LIBRECEIVER_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(LIBRECEIVER_OBJS)

CLEANS += $(libreceiver)
