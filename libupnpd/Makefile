TOP?=../
DIR:=libupnpd/
SRCS:=\
	avtransport.cpp \
	media_renderer.cpp 

TEST_SRCS:=

all-libupnpd:

all: all-libupnpd

include $(TOP)Make.common

LIBUPNPD_OBJS:=$(LOBJS)

libupnpd:=$(TOP)$(DIR)$(TARGETDIR)libupnpd.la

all-libupnpd: $(libupnpd)

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
ifeq ($(libdbsteam),)
include $(TOP)libdbsteam/Makefile
endif

$(libupnpd): $(LIBUPNPD_OBJS)
	$(AR_PRE)libtool --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(LIBUPNPD_OBJS)

CLEANS += $(libupnpd) $(UPNPD_OBJS)
