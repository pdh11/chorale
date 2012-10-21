TOP?=../
DIR:=libmediadb/
LIBMEDIADB_DIR:=$(TOP)$(DIR)
SRCS:= 	allocate_id.cpp \
	didl.cpp \
	registry.cpp \
	schema.cpp \
	sync.cpp \
	web_content.cpp \
	xml.cpp

TEST_SRCS:= \
	didl.cpp \
	registry.cpp \
	schema.cpp \
	sync_test.cpp \
	xml.cpp

all-mediadb:

all: all-mediadb

include $(TOP)Make.common

MEDIADB_OBJS:=$(LOBJS)
MEDIADB_TESTS:=$(TESTS)

libmediadb:=$(TOP)$(DIR)$(TARGETDIR)libmediadb.a

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
ifeq ($(libdblocal),)
include $(TOP)libdblocal/Makefile
endif
ifeq ($(libimport),)
include $(TOP)libimport/Makefile
endif

$(MEDIADB_TESTS) : TEST_LIBS += $(libdblocal) $(libmediadb) $(libdbsteam) $(libdb) $(libimport) $(libutil) $(TAGLIB_LIBS) $(BOOST_LIBS) $(LIBS)

$(MEDIADB_TESTS) : $(libdblocal) $(libmediadb) $(libdbsteam) $(libdb) $(libutil)

ifneq ($(EXCEPTIONAL_boost_function_hpp),)
$(LIBMEDIADB_DIR)$(TARGETDIR)didl%: CXXFLAGS += -fexceptions -UBOOST_NO_EXCEPTIONS
endif
ifneq ($(RTTI_boost_algorithm_string_split_hpp),)
$(LIBMEDIADB_DIR)$(TARGETDIR)didl%: CXXFLAGS += -frtti
endif

$(libmediadb): $(MEDIADB_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(MEDIADB_OBJS)

CLEANS += $(libmediadb)
