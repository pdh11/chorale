TOP?=../
DIR:=libuiqt/
SRCS:= \
	frame_buffer.cpp \
	frame_buffer_window.cpp \
	frame_buffer_window.moc.cpp \
	scheduler.cpp \
	scheduler.moc.cpp

TEST_SRCS:= \
	scheduler.cpp

all-libuiqt:

include $(TOP)Make.common

LIBUIQT_OBJS:=$(LOBJS)
LIBUIQT_TESTS:=$(TESTS)
LIBUIQT_DIR:=$(DIR)

libuiqt:=$(TOP)$(DIR)$(TARGETDIR)libuiqt.a

all-libuiqt: $(libuiqt)

$(TOP)$(DIR)% : CXXFLAGS += $(BOOST_CXXFLAGS) $(QT_CXXFLAGS) $(CAIRO_CXXFLAGS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif

$(LIBUIQT_TESTS): TEST_LIBS+= $(libuiqt) $(libutil) $(QT_LIBS) $(BOOST_LIBS) $(CAIRO_LIBS) $(LIBS)

$(LIBUIQT_TESTS): $(libuiqt) $(libutil)

ifeq ($(QT),yes)

all: all-libuiqt

$(libuiqt): $(LIBUIQT_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(LIBUIQT_OBJS)

else

$(libuiqt):
	@echo "Ignoring libuiqt (dependencies not met)"

endif
