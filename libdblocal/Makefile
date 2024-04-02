TOP?=../
DIR:=libdblocal/
SRCS:= \
	database_updater.cpp \
	db.cpp \
	file_scanner.cpp \
	tag_serialiser.cpp

TEST_SRCS:= \
	file_scanner.cpp \
	db.cpp \
	tag_serialiser.cpp

all-dblocal:

all: all-dblocal

include $(TOP)Make.common

DBLOCAL_OBJS:=$(LOBJS)
DBLOCAL_TESTS:=$(TESTS)

libdblocal:=$(TOP)$(DIR)$(TARGETDIR)libdblocal.a

all-dblocal: $(libdblocal)

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

$(DBLOCAL_TESTS) : TEST_LIBS += $(libdblocal) $(libmediadb) $(libdbsteam) $(libdb) $(libimport) $(libutil) $(TAGLIB_LIBS) $(BOOST_LIBS) $(AVFORMAT_LIBS) $(LIBS)

$(DBLOCAL_TESTS) : $(libdblocal) $(libmediadb) $(libdbempeg) $(libempeg) $(libdbsteam) $(libdb) $(libimport) $(libutil)

$(libdblocal): $(DBLOCAL_OBJS)
	rm -f $@
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(DBLOCAL_OBJS)

CLEANS += $(libdblocal)
