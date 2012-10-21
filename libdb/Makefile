TOP?=../
DIR:=libdb/
SRCS:=	delegating_query.cpp \
	delegating_rs.cpp \
	free_rs.cpp \
	query.cpp \
	readonly_rs.cpp

all-db:

all: all-db

include $(TOP)Make.common

libdb:=$(TOP)$(DIR)$(TARGETDIR)libdb.la

all-db: $(libdb)

DB_OBJS:=$(LOBJS)

$(TOP)$(DIR)% : CXXFLAGS += $(BOOST_CXXFLAGS)

$(libdb): $(DB_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(DB_OBJS)

CLEANS += $(libdb)
