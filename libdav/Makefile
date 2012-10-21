TOP?=../
DIR:=libdav/
SRCS:= \
	server.cpp

TEST_SRCS:= \
	server.cpp

all-libdav:

include $(TOP)Make.common

LIBDAV_OBJS:=$(LOBJS)
LIBDAV_TESTS:=$(TESTS)
LIBDAV_DIR:=$(DIR)

libdav:=$(TOP)$(DIR)$(TARGETDIR)libdav.a

all-libdav: $(libdav)

$(TOP)$(DIR)% : CXXFLAGS += $(BOOST_CXXFLAGS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif

$(LIBDAV_TESTS): TEST_LIBS+= $(libdav) $(libutil) $(BOOST_LIBS) $(LIBS)

$(LIBDAV_TESTS): $(libdav) $(libutil)

ifneq ($(EXCEPTIONAL_boost_regex_hpp),)
$(LIBDAV_TESTS): CXXFLAGS += -fexceptions -UBOOST_NO_EXCEPTIONS
endif

all: all-libdav

$(libdav): $(LIBDAV_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(LIBDAV_OBJS)
