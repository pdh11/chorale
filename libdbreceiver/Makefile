TOP?=../
DIR:=libdbreceiver/
SRCS:=\
	db.cpp \
	query.cpp \
	rs.cpp

all-libdbreceiver:

all: all-libdbreceiver

include $(TOP)Make.common

LIBDBRECEIVER_OBJS:=$(LOBJS)

libdbreceiver:=$(TOP)$(DIR)$(TARGETDIR)libdbreceiver.la

receiverscan:=$(TOP)$(DIR)receiverscan
RECEIVERSCAN_OBJS:=$(TOP)$(DIR)$(TARGETDIR)receiverscan.lo

all-libdbreceiver: $(libdbreceiver) $(receiverscan)

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
ifeq ($(libreceiver),)
include $(TOP)libreceiver/Makefile
endif

$(libdbreceiver): $(LIBDBRECEIVER_OBJS)
	$(AR_PRE)libtool --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(LIBDBRECEIVER_OBJS)

$(receiverscan): $(RECEIVERSCAN_OBJS) $(libdbreceiver) $(libreceiver) \
		$(libdb) $(libutil) Makefile $(libmediadb)
	$(LD_PRE)libtool --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(RECEIVERSCAN_OBJS) $(libdbreceiver) \
		$(libreceiver) $(libmediadb) $(libdb) $(libutil) \
		$(LIBXMLPP_LIBS) $(BOOST_LIBS) $(CURL_LIBS)

-include $(RECEIVERSCAN_OBJS:%.lo=%.lo.dep)

CLEANS += $(libdbreceiver) $(RECEIVERSCAN_OBJS) $(receiverscan)
