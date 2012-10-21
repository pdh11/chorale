TOP?=../
DIR:=libdbupnp/
SRCS:=\
	db.cpp \
	query.cpp \
	rs.cpp

TEST_SRCS:= \
	db.cpp

all-libdbupnp:

all: all-libdbupnp

include $(TOP)Make.common

LIBDBUPNP_OBJS:=$(LOBJS)
LIBDBUPNP_TESTS:=$(TESTS)

libdbupnp:=$(TOP)$(DIR)$(TARGETDIR)libdbupnp.la

all-libdbupnp: $(libdbupnp)

$(TOP)$(DIR)% : CXXFLAGS += $(TAGLIB_CXXFLAGS) $(BOOST_CXXFLAGS) \
	$(LIBCDIOP_CXXFLAGS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif
ifeq ($(libdb),)
include $(TOP)libdb/Makefile
endif
ifeq ($(libupnp),)
include $(TOP)libupnp/Makefile
endif
ifeq ($(libmediadb),)
include $(TOP)libmediadb/Makefile
endif

$(LIBDBUPNP_TESTS) : TEST_LIBS+= $(BOOST_LIBS) $(CURL_LIBS) $(UPNP_LIBS)

$(LIBDBUPNP_TESTS) : $(libdbupnp) $(libmediadb) $(libdb) $(libupnp) $(libutil) 

$(libdbupnp): $(LIBDBUPNP_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(LIBDBUPNP_OBJS)

CLEANS += $(libdbupnp)
