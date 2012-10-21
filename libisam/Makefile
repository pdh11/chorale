TOP?=../
DIR:=libisam/
SRCS:=\
	direct_page_handler.cpp \
	file.cpp \
	key_storage.cpp \
	page_allocator.cpp \
	page_locking.cpp

TEST_SRCS:= \
	direct_page_handler.cpp \
	file.cpp \
	key_storage.cpp \
	page_allocator.cpp \
	page_locking.cpp

all-libisam:

all: all-libisam

include $(TOP)Make.common

LIBISAM_OBJS:=$(LOBJS)
LIBISAM_TESTS:=$(TESTS)

libisam:=$(TOP)$(DIR)$(TARGETDIR)libisam.a

isamtest:=$(TOP)$(DIR)isamtest$(DOTEXE)
ISAMTEST_OBJS:=$(TOP)$(DIR)$(TARGETDIR)isamtest.lo

all-libisam: $(libisam) $(isamtest)

$(TOP)$(DIR)% : CXXFLAGS += $(TAGLIB_CXXFLAGS) $(BOOST_CXXFLAGS) \
	$(LIBCDIOP_CXXFLAGS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif

$(LIBISAM_TESTS) : TEST_LIBS += $(libisam) $(libutil) $(BOOST_LIBS) $(LIBS)

$(LIBISAM_TESTS) : $(libisam) $(libdbupnp) $(libdbsteam) \
		$(libdblocal) $(libimport) \
		$(libmediadb) $(libupnp) $(libdb) $(libutil)

$(libisam): $(LIBISAM_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(LIBISAM_OBJS)

$(isamtest): $(libisam) $(libutil) $(ISAMTEST_OBJS) Makefile
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --silent \
		--preserve-dup-deps \
		$(CXX) $(LDFLAGS) $(CXXFLAGS) \
		-o $@ $(ISAMTEST_OBJS) $(libisam) $(libutil) \
		$(BOOST_LIBS) $(LIBS) -Wl,-Map,$@.map

CLEANS += $(libisam) $(ISAM_OBJS) $(ISAMTEST_OBJS)
