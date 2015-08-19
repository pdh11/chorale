TOP?=../
DIR:=libkarma/
SRCS:=\
	db_writer.cpp \
	protocol_client.cpp \

all-libkarma:

all: all-libkarma

include $(TOP)Make.common

LIBKARMA_OBJS:=$(LOBJS)
LIBKARMA_TESTS:=$(TESTS)

libkarma:=$(TOP)$(DIR)$(TARGETDIR)libkarma.a

all-libkarma: $(libkarma)

$(TOP)$(DIR)% : CXXFLAGS += $(TAGLIB_CXXFLAGS) $(BOOST_CXXFLAGS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif

$(LIBKARMA_TESTS) : TEST_LIBS += $(libkarma) $(libutil) $(BOOST_LIBS) $(LIBS)

$(LIBKARMA_TESTS) : $(libkarma) $(libutil)

$(libkarma): $(LIBKARMA_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(LIBKARMA_OBJS)

CLEANS += $(libkarma)
