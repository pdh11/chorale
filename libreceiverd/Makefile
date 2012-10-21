TOP?=../
DIR:=libreceiverd/
SRCS:=\
	content_factory.cpp

TEST_SRCS:=\
	content_factory.cpp

all-libreceiverd:

all: all-libreceiverd

include $(TOP)Make.common

LIBRECEIVERD_OBJS:=$(LOBJS)
LIBRECEIVERD_TESTS:=$(TESTS)

libreceiverd:=$(TOP)$(DIR)$(TARGETDIR)libreceiverd.la

all-libreceiverd: $(libreceiverd)

$(TOP)$(DIR)% : CXXFLAGS += $(TAGLIB_CXXFLAGS) $(BOOST_CXXFLAGS) \
	$(LIBCDIOP_CXXFLAGS) $(LIBFLAC_CXXFLAGS) $(LIBXMLPP_CXXFLAGS)

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
ifeq ($(libreceiver),)
include $(TOP)libreceiver/Makefile
endif

$(LIBRECEIVERD_TESTS) : TEST_LIBS += $(BOOST_LIBS) $(CURL_LIBS) $(UPNP_LIBS) \
	$(DBUS_LIBS) $(LIBXMLPP_LIBS) $(HAL_LIBS)

$(LIBRECEIVERD_TESTS) : $(libreceiverd) $(libreceiver) $(libdbsteam) \
		$(libmediadb) $(libdb) $(libutil)

$(libreceiverd): $(LIBRECEIVERD_OBJS)
	$(AR_PRE)libtool --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(LIBRECEIVERD_OBJS)

CLEANS += $(libreceiverd) $(RECEIVERD_OBJS) $(MODRECEIVER_OBJS)
