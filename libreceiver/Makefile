TOP?=../
DIR:=libreceiver/
SRCS:=\
	ssdp.cpp \
	tags.cpp

all-libreceiver:

all: all-libreceiver

include $(TOP)Make.common

LIBRECEIVER_OBJS:=$(LOBJS)

libreceiver:=$(TOP)$(DIR)$(TARGETDIR)libreceiver.la

all-libreceiver: $(libreceiver)

$(TOP)$(DIR)% : CXXFLAGS += $(TAGLIB_CXXFLAGS) $(BOOST_CXXFLAGS) \
	$(LIBCDIOP_CXXFLAGS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif

$(libreceiver): $(LIBRECEIVER_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(LIBRECEIVER_OBJS)

CLEANS += $(libreceiver)
