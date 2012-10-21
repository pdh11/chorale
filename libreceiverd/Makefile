TOP?=../
DIR:=libreceiverd/
SRCS:=\
	content_factory.cpp \
	mount.cpp \
	nfs.cpp \
	portmap.cpp \
	rpc.cpp \
	tarfs.cpp 

TEST_SRCS:=\
	content_factory.cpp \
	nfs.cpp

all-libreceiverd:

all: all-libreceiverd

include $(TOP)Make.common

LIBRECEIVERD_OBJS:=$(LOBJS)
LIBRECEIVERD_TESTS:=$(TESTS)

libreceiverd:=$(TOP)$(DIR)$(TARGETDIR)libreceiverd.a

all-libreceiverd: $(libreceiverd)

$(TOP)$(DIR)% : CXXFLAGS += $(TAGLIB_CXXFLAGS) $(BOOST_CXXFLAGS) \
	$(LIBCDIOP_CXXFLAGS)

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
ifeq ($(libdbmerge),)
include $(TOP)libdbmerge/Makefile
endif
ifeq ($(libreceiver),)
include $(TOP)libreceiver/Makefile
endif

$(LIBRECEIVERD_TESTS) : TEST_LIBS +=  $(libreceiverd) $(libreceiver) \
	$(libdbsteam) $(libdbmerge) \
	$(libdblocal) $(libimport) $(libmediadb) $(libdb) $(libutil) \
	$(TAGLIB_LIBS) $(BOOST_LIBS) \
	$(DBUS_LIBS) $(HAL_LIBS) $(LIBS)

$(LIBRECEIVERD_TESTS) : $(libreceiverd) $(libreceiver) $(libdbsteam) \
		$(libdbmerge) \
		$(libdblocal) $(libimport) $(libmediadb) $(libdb) $(libutil)

$(libreceiverd): $(LIBRECEIVERD_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(LIBRECEIVERD_OBJS)

CLEANS += $(libreceiverd) $(RECEIVERD_OBJS) $(MODRECEIVER_OBJS)
