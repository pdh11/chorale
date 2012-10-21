TOP?=../
DIR:=libdbinno/
SRCS:=  database.cpp \
	recordset.cpp

TEST_SRCS:= database.cpp

all-dbinno:

all: all-dbinno

include $(TOP)Make.common

DBINNO_OBJS:=$(LOBJS)
DBINNO_TESTS:=$(TESTS)

libdbinno:=$(TOP)$(DIR)$(TARGETDIR)libdbinno.a

all-dbinno: $(libdbinno)

$(TOP)$(DIR)% : CXXFLAGS += $(BOOST_CXXFLAGS) $(INNODB_CXXFLAGS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif
ifeq ($(libdb),)
include $(TOP)libdb/Makefile
endif
ifeq ($(libdblocal),)
include $(TOP)libdblocal/Makefile
endif
ifeq ($(libmediadb),)
include $(TOP)libmediadb/Makefile
endif

$(DBINNO_TESTS) : TEST_LIBS += $(BOOST_LIBS) $(INNODB_LIBS)

$(DBINNO_TESTS) : $(libdbinno) $(libdblocal) $(libmediadb) $(libdb) $(libutil)

$(libdbinno): $(DBINNO_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(DBINNO_OBJS)

CLEANS += $(libdbinno)
