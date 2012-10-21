TOP?=../
DIR:=libuidarwin/
SRCS:= \
	scheduler.cpp

all-libuidarwin:

include $(TOP)Make.common

LIBUIDARWIN_OBJS:=$(LOBJS)
LIBUIDARWIN_TESTS:=$(TESTS)
LIBUIDARWIN_DIR:=$(DIR)

libuidarwin:=$(TOP)$(DIR)$(TARGETDIR)libuidarwin.a

all-libuidarwin: $(libuidarwin)

$(TOP)$(DIR)% : CXXFLAGS += $(BOOST_CXXFLAGS) $(QT_CXXFLAGS) 

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif

$(LIBUIDARWIN_TESTS): TEST_LIBS+= $(libuidarwin) $(libutil) $(BOOST_LIBS) $(LIBS)

$(LIBUIDARWIN_TESTS): $(libuidarwin) $(libutil)

ifeq ($(DARWIN),yes)

all: all-libuidarwin

$(libuidarwin): $(LIBUIDARWIN_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(LIBUIDARWIN_OBJS)

else

$(libuidarwin):
	@echo "Ignoring libuidarwin (dependencies not met)"

endif
