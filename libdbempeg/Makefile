TOP?=../
DIR:=libdbempeg/
SRCS:=\
	allocate_id.cpp \
	connection.cpp \
	db.cpp \
	query.cpp \
	rs.cpp

TEST_SRCS:=\
	db.cpp

all-libdbempeg:

all: all-libdbempeg

include $(TOP)Make.common

LIBDBEMPEG_OBJS:=$(LOBJS)
LIBDBEMPEG_TESTS:=$(TESTS)

libdbempeg:=$(TOP)$(DIR)$(TARGETDIR)libdbempeg.a

all-libdbempeg: $(libdbempeg)

$(TOP)$(DIR)% : CXXFLAGS += $(TAGLIB_CXXFLAGS) $(BOOST_CXXFLAGS)

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
ifeq ($(libempeg),)
include $(TOP)libempeg/Makefile
endif

$(LIBDBEMPEG_TESTS) : TEST_LIBS +=  $(libdbempeg) $(libmediadb) $(libdbsteam) \
	$(libdb) $(libempeg) $(libutil) $(BOOST_LIBS) $(LIBCDIOP_LIBS) $(LIBS)

$(LIBDBEMPEG_TESTS) : $(libdbempeg) $(libmediadb) $(libdbsteam) \
	$(libdb) $(libempeg) $(libutil)

$(libdbempeg): $(LIBDBEMPEG_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(LIBDBEMPEG_OBJS)

CLEANS += $(libdbempeg)
