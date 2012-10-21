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

libdbreceiver:=$(TOP)$(DIR)$(TARGETDIR)libdbreceiver.a

receiverscan:=$(TOP)$(DIR)receiverscan
RECEIVERSCAN_OBJS:=$(TOP)$(DIR)$(TARGETDIR)receiverscan.lo

all-libdbreceiver: $(libdbreceiver) $(receiverscan)

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
ifeq ($(libreceiver),)
include $(TOP)libreceiver/Makefile
endif

$(libdbreceiver): $(LIBDBRECEIVER_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CXX) -o $@ $(LIBDBRECEIVER_OBJS)

$(receiverscan): $(RECEIVERSCAN_OBJS) $(libdbreceiver) $(libreceiver) \
		$(libdb) $(libutil) Makefile $(libmediadb)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CXX) -o $@ $(RECEIVERSCAN_OBJS) $(libdbreceiver) \
		$(libreceiver) $(libmediadb) $(libdb) $(libutil) \
		$(BOOST_LIBS) $(LIBS)

-include $(RECEIVERSCAN_OBJS:%.lo=%.lo.dep)

CLEANS += $(libdbreceiver) $(RECEIVERSCAN_OBJS) $(receiverscan)
