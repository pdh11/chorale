TOP?=../
DIR:=libmediadb/
SRCS:= 	allocate_id.cpp \
	localdb.cpp \
	registry.cpp \
	schema.cpp \
	xml.cpp

TEST_SRCS:= \
	registry.cpp \
	schema.cpp \
	xml.cpp

all-mediadb:

all: all-mediadb

include $(TOP)Make.common

MEDIADB_OBJS:=$(LOBJS)
MEDIADB_TESTS:=$(TESTS)

libmediadb:=$(TOP)$(DIR)$(TARGETDIR)libmediadb.la

all-mediadb: $(libmediadb)

$(TOP)$(DIR)% : CXXFLAGS += $(BOOST_CXXFLAGS) $(LIBXMLPP_CXXFLAGS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif
ifeq ($(libdb),)
include $(TOP)libdb/Makefile
endif

$(MEDIADB_TESTS) : TEST_LIBS += $(BOOST_LIBS) $(LIBXMLPP_LIBS)

$(MEDIADB_TESTS) : $(libmediadb) $(libdb) $(libutil)

$(libmediadb): $(MEDIADB_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(MEDIADB_OBJS)

CLEANS += $(libmediadb)
