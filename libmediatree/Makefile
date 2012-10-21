TOP?=../
DIR:=libmediatree/
SRCS:=\
	directory.cpp \
	query.cpp \
	results.cpp \
	root.cpp

all-libmediatree:

all: all-libmediatree

include $(TOP)Make.common

LIBMEDIATREE_OBJS:=$(LOBJS)

libmediatree:=$(TOP)$(DIR)$(TARGETDIR)libmediatree.la

all-libmediatree: $(libmediatree)

$(TOP)$(DIR)% : CXXFLAGS += $(TAGLIB_CXXFLAGS) $(BOOST_CXXFLAGS) \
	$(LIBCDIOP_CXXFLAGS) $(LIBXMLPP_CXXFLAGS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif
ifeq ($(libdb),)
include $(TOP)libdb/Makefile
endif
ifeq ($(libreceiver),)
include $(TOP)libreceiver/Makefile
endif

$(libmediatree): $(LIBMEDIATREE_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(LIBMEDIATREE_OBJS)

CLEANS += $(libmediatree)
