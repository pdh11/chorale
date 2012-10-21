TOP?=../
DIR:=libdb/
SRCS:=	dbc.cpp \
	free_rs.cpp \
	query_impl.cpp \
	readonly_rs.cpp

all-db:

all: all-db

include $(TOP)Make.common

libdb:=$(TOP)$(DIR)$(TARGETDIR)libdb.la

all-db: $(libdb)

DB_OBJS:=$(LOBJS)

$(TOP)$(DIR)% : CXXFLAGS += $(BOOST_CXXFLAGS) $(LIBXMLPP_CXXFLAGS)

$(libdb): $(DB_OBJS)
	$(AR_PRE)libtool --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(DB_OBJS)

CLEANS += $(libdb)
