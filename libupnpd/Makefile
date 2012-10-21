TOP?=../
DIR:=libupnpd/
SRCS:= \
	avtransport.cpp \
	content_directory.cpp \
	media_renderer.cpp \
	media_server.cpp \
	optical_drive.cpp \
	search.cpp

TEST_SRCS:= \
	avtransport.cpp \
	content_directory.cpp \
	search.cpp

all-libupnpd:

all: all-libupnpd

include $(TOP)Make.common

LIBUPNPD_DIR:=$(TOP)$(DIR)
LIBUPNPD_OBJS:=$(LOBJS)
LIBUPNPD_TESTS:=$(TESTS)

libupnpd:=$(TOP)$(DIR)$(TARGETDIR)libupnpd.a

all-libupnpd: $(libupnpd)

$(TOP)$(DIR)% : CXXFLAGS += $(TAGLIB_CXXFLAGS) $(BOOST_CXXFLAGS) \
	$(LIBCDIOP_CXXFLAGS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif
ifeq ($(libupnp),)
include $(TOP)libupnp/Makefile
endif
ifeq ($(libdbupnp),)
include $(TOP)libdbupnp/Makefile
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

$(LIBUPNPD_DIR)$(TARGETDIR)search%: CXXFLAGS += -fexceptions -UBOOST_NO_EXCEPTIONS

$(LIBUPNPD_TESTS) : TEST_LIBS += $(libupnpd) $(libdbupnp) $(libdbsteam) \
		$(libdblocal) $(libimport) \
		$(libmediadb) $(libupnp) $(libdb) $(libutil) \
		$(TAGLIB_LIBS) $(BOOST_LIBS) $(LIBS)

$(LIBUPNPD_TESTS) : $(libupnpd) $(libdbupnp) $(libdbsteam) \
		$(libdblocal) $(libimport) \
		$(libmediadb) $(libupnp) $(libdb) $(libutil)

$(libupnpd): $(LIBUPNPD_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(LIBUPNPD_OBJS)

CLEANS += $(libupnpd) $(UPNPD_OBJS)
