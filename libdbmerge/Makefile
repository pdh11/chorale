TOP?=../
DIR:=libdbmerge/
SRCS:= \
	db.cpp

TEST_SRCS:= \
	db.cpp

all-dbmerge:

all: all-dbmerge

include $(TOP)Make.common

DBMERGE_OBJS:=$(LOBJS)
DBMERGE_TESTS:=$(TESTS)

libdbmerge:=$(TOP)$(DIR)$(TARGETDIR)libdbmerge.a

all-dbmerge: $(libdbmerge)

$(TOP)$(DIR)% : CXXFLAGS += $(BOOST_CXXFLAGS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif
ifeq ($(libdb),)
include $(TOP)libdb/Makefile
endif
ifeq ($(libdbsteam),)
include $(TOP)libdbsteam/Makefile
endif
ifeq ($(libmediadb),)
include $(TOP)libmediadb/Makefile
endif
ifeq ($(libimport),)
include $(TOP)libimport/Makefile
endif

$(DBMERGE_TESTS) : TEST_LIBS += $(libdbmerge) $(libmediadb) $(libdbempeg) $(libempeg) $(libdbsteam) $(libdb) $(libimport) $(libutil) $(TAGLIB_LIBS) $(BOOST_LIBS) $(LIBS)

$(DBMERGE_TESTS) : $(libdbmerge) $(libmediadb) $(libdbempeg) $(libempeg) $(libdbsteam) $(libdb) $(libimport) $(libutil)

$(libdbmerge): $(DBMERGE_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(DBMERGE_OBJS)

CLEANS += $(libdbmerge)
