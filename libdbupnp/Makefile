TOP?=../
DIR:=libdbupnp/
SRCS:=\
	connection.cpp \
	database.cpp \
	query.cpp \
	recordset.cpp

TEST_SRCS:= \
	database.cpp

all-libdbupnp:

all: all-libdbupnp

include $(TOP)Make.common

LIBDBUPNP_OBJS:=$(LOBJS)
LIBDBUPNP_TESTS:=$(TESTS)

libdbupnp:=$(TOP)$(DIR)$(TARGETDIR)libdbupnp.a

all-libdbupnp: $(libdbupnp)

$(TOP)$(DIR)% : CXXFLAGS += $(TAGLIB_CXXFLAGS) $(BOOST_CXXFLAGS) \
	$(LIBCDIOP_CXXFLAGS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif
ifeq ($(libdb),)
include $(TOP)libdb/Makefile
endif
ifeq ($(libdblocal),)
include $(TOP)libdblocal/Makefile
endif
ifeq ($(libimport),)
include $(TOP)libimport/Makefile
endif
ifeq ($(libdbsteam),)
include $(TOP)libdbsteam/Makefile
endif
ifeq ($(libupnp),)
include $(TOP)libupnp/Makefile
endif
ifeq ($(libupnpd),)
include $(TOP)libupnpd/Makefile
endif
ifeq ($(libmediadb),)
include $(TOP)libmediadb/Makefile
endif

$(LIBDBUPNP_TESTS) : TEST_LIBS+= $(libdbupnp) $(libupnpd) $(libmediadb) $(libdblocal) $(libimport) $(libdbsteam) $(libdb) $(libupnp) $(libutil) $(TAGLIB_LIBS) $(BOOST_LIBS) $(LIBS)

$(LIBDBUPNP_TESTS) : $(libdbupnp) $(libupnpd) $(libmediadb) $(libdblocal) $(libimport) $(libdbsteam) $(libdb) $(libupnp) $(libutil) 

$(libdbupnp): $(LIBDBUPNP_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(LIBDBUPNP_OBJS)

CLEANS += $(libdbupnp)
