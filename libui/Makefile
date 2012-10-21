TOP?=../
DIR:=libui/
SRCS:= \
	frame_buffer.cpp \
	window.cpp

TEST_SRCS:= \

all-libui:

include $(TOP)Make.common

LIBUI_OBJS:=$(LOBJS)
LIBUI_TESTS:=$(TESTS)
LIBUI_DIR:=$(DIR)

libui:=$(TOP)$(DIR)$(TARGETDIR)libui.a

all-libui: $(libui)

$(TOP)$(DIR)% : CXXFLAGS += $(BOOST_CXXFLAGS) $(QT_CXXFLAGS) $(CAIRO_CXXFLAGS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif

$(LIBUI_TESTS): TEST_LIBS+= $(libui) $(libutil) $(QT_LIBS) $(CAIRO_LIBS) $(BOOST_LIBS) $(LIBS)

$(LIBUI_TESTS): $(libui) $(libutil)

all: all-libui

$(libui): $(LIBUI_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(LIBUI_OBJS)
