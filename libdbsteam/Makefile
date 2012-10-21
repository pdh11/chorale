TOP?=../
DIR:=libdbsteam/
SRCS:=	db.cpp \
	query.cpp \
	rs.cpp \
	test.cpp

TEST_SRCS:= \
	db.cpp \
	query.cpp \
	rs.cpp

all-dbsteam:

all: all-dbsteam

include $(TOP)Make.common

DBSTEAM_OBJS:=$(LOBJS)
DBSTEAM_TESTS:=$(TESTS)

libdbsteam:=$(TOP)$(DIR)$(TARGETDIR)libdbsteam.a

all-dbsteam: $(libdbsteam)

$(TOP)$(DIR)% : CXXFLAGS += $(BOOST_CXXFLAGS)

ifneq ($(EXCEPTIONAL_boost_regex_hpp),)
$(TOP)$(DIR)$(TARGETDIR)query%: CXXFLAGS += -fexceptions -UBOOST_NO_EXCEPTIONS
endif

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif
ifeq ($(libdb),)
include $(TOP)libdb/Makefile
endif
#ifeq ($(libimport),)
#include $(TOP)libimport/Makefile
#endif

$(DBSTEAM_TESTS) : TEST_LIBS += $(libdbsteam) $(libdb) $(libutil) $(BOOST_LIBS)

$(DBSTEAM_TESTS) : $(libdbsteam) $(libimport) $(libdb) $(libutil)

$(libdbsteam): $(DBSTEAM_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(DBSTEAM_OBJS)

CLEANS += $(libdbsteam)
