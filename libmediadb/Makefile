TOP?=../
DIR:=libmediadb/
SRCS:= 	allocate_id.cpp \
	didl.cpp \
	localdb.cpp \
	registry.cpp \
	schema.cpp \
	sync.cpp \
	web_content.cpp \
	xml.cpp

TEST_SRCS:= \
	didl.cpp \
	localdb.cpp \
	registry.cpp \
	schema.cpp \
	sync_test.cpp \
	xml.cpp

all-mediadb:

all: all-mediadb

include $(TOP)Make.common

MEDIADB_OBJS:=$(LOBJS)
MEDIADB_TESTS:=$(TESTS)

libmediadb:=$(TOP)$(DIR)$(TARGETDIR)libmediadb.la

all-mediadb: $(libmediadb)

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
ifeq ($(libdbempeg),)
include $(TOP)libdbempeg/Makefile
endif

$(MEDIADB_TESTS) : TEST_LIBS += $(BOOST_LIBS) $(LIBS)

$(MEDIADB_TESTS) : $(libmediadb) $(libdbempeg) $(libempeg) $(libdbsteam) $(libdb) $(libutil)

$(libmediadb): $(MEDIADB_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(MEDIADB_OBJS)

CLEANS += $(libmediadb)
