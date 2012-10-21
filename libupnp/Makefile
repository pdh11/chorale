TOP?=../
DIR:=libupnp/
LIBUPNP_DIR:=$(TOP)$(DIR)
SRCS:=\
	client.cpp \
	description.cpp \
	device.cpp \
	didl.cpp \
	server.cpp \
	soap.cpp

LIBUPNP_XML:= \
	AVTransport2.xml \
	ConnectionManager2.xml \
	ContentDirectory2.xml \
	OpticalDrive.xml \
	RenderingControl2.xml \
	ScheduledRecording1.xml

SRCS:=$(sort $(SRCS) $(LIBUPNP_XML:%.xml=%.cpp) $(LIBUPNP_XML:%.xml=%_server.cpp) $(LIBUPNP_XML:%.xml=%_client.cpp))

TEST_SRCS:=\
	didl.cpp

all-libupnp:

all: all-libupnp

include $(TOP)Make.common

LIBUPNP_OBJS:=$(LOBJS)
LIBUPNP_TESTS:=$(TESTS)

libupnp:=$(TOP)$(DIR)$(TARGETDIR)libupnp.la

all-libupnp: $(libupnp)

$(TOP)$(DIR)% : CXXFLAGS += $(TAGLIB_CXXFLAGS) $(BOOST_CXXFLAGS) \
	$(LIBCDIOP_CXXFLAGS)

ifeq ($(HAVE_XSLTPROC),yes)

$(TOP)$(DIR)%.h : $(TOP)$(DIR)%.xml $(LIBUPNP_DIR)scpd.xsl
	$(GEN_PRE)$(XSLTPROC) --stringparam class $* \
		    --stringparam mode header \
		    -o $@ \
		    $(LIBUPNP_DIR)scpd.xsl $<

$(TOP)$(DIR)%.cpp : $(TOP)$(DIR)%.xml $(LIBUPNP_DIR)scpd.xsl
	$(GEN_PRE)$(XSLTPROC) --stringparam class $* \
		    --stringparam mode stubs \
		    -o $@ \
		    $(LIBUPNP_DIR)scpd.xsl $<

$(TOP)$(DIR)%_client.h : $(TOP)$(DIR)%.xml $(LIBUPNP_DIR)scpd.xsl
	$(GEN_PRE)$(XSLTPROC) --stringparam class $* \
		    --stringparam mode clientheader \
		    -o $@ \
		    $(LIBUPNP_DIR)scpd.xsl $<

$(TOP)$(DIR)%_client.cpp : $(TOP)$(DIR)%.xml $(LIBUPNP_DIR)scpd.xsl
	$(GEN_PRE)$(XSLTPROC) --stringparam class $* \
		    --stringparam mode client \
		    -o $@ \
		    $(LIBUPNP_DIR)scpd.xsl $<

$(TOP)$(DIR)%_server.h : $(TOP)$(DIR)%.xml $(LIBUPNP_DIR)scpd.xsl
	$(GEN_PRE)$(XSLTPROC) --stringparam class $* \
		    --stringparam mode serverheader \
		    -o $@ \
		    $(LIBUPNP_DIR)scpd.xsl $<

$(TOP)$(DIR)%_server.cpp : $(TOP)$(DIR)%.xml $(LIBUPNP_DIR)scpd.xsl
	$(GEN_PRE)$(XSLTPROC) --stringparam class $* \
		    --stringparam mode server \
		    -o $@ \
		    $(LIBUPNP_DIR)scpd.xsl $<

else

$(TOP)$(DIR)%.h $(TOP)$(DIR)%.cpp $(TOP)$(DIR)%_client.h \
	$(TOP)$(DIR)%_client.cpp $(TOP)$(DIR)%_server.h \
	$(TOP)$(DIR)%_server.cpp: $(TOP)$(DIR)%.xml $(LIBUPNP_DIR)scpd.xsl
	@echo "** XML modified but xsltproc not found"
	@echo "   Install it and re-run \"configure\" to enact changes"
	@echo

endif

.PRECIOUS: $(TOP)$(DIR)%.cpp $(TOP)$(DIR)%_server.cpp \
	$(TOP)$(DIR)%_client.cpp $(TOP)$(DIR)%.h

.force-header-generation: \
	$(LIBUPNP_XML:%.xml=$(LIBUPNP_DIR)%.h) \
	$(LIBUPNP_XML:%.xml=$(LIBUPNP_DIR)%_server.h) \
	$(LIBUPNP_XML:%.xml=$(LIBUPNP_DIR)%_client.h)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif
ifeq ($(libdb),)
include $(TOP)libdb/Makefile
endif
ifeq ($(libmediadb),)
include $(TOP)libmediadb/Makefile
endif
ifeq ($(libdbsteam),)
include $(TOP)libdbsteam/Makefile
endif

$(TOP)$(DIR)% : CXXFLAGS += $(UPNP_CXXFLAGS)

$(LIBUPNP_TESTS): TEST_LIBS+= $(UPNP_LIBS) $(BOOST_LIBS)

$(LIBUPNP_TESTS): $(libmediadb) $(libdbsteam) $(libdb) $(libupnp) $(libutil)

$(libupnp): $(LIBUPNP_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(LIBUPNP_OBJS)

$(upnpscan): $(UPNPSCAN_OBJS) $(libupnp) \
		$(libdb) $(libutil) Makefile $(libmediadb)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(RECEIVERSCAN_OBJS) $(libupnp) \
		$(libmediadb) $(libdb) $(libutil) \
		$(BOOST_LIBS) $(CURL_LIBS) $(UPNP_LIBS)

-include $(UPNPSCAN_OBJS:%.lo=%.lo.dep)

CLEANS += $(libupnp)

MAINTAINERCLEANS += \
	$(LIBUPNP_XML:%.xml=$(LIBUPNP_DIR)%.h) \
	$(LIBUPNP_XML:%.xml=$(LIBUPNP_DIR)%.cpp) \
	$(LIBUPNP_XML:%.xml=$(LIBUPNP_DIR)%_server.h) \
	$(LIBUPNP_XML:%.xml=$(LIBUPNP_DIR)%_server.cpp) \
	$(LIBUPNP_XML:%.xml=$(LIBUPNP_DIR)%_client.h) \
	$(LIBUPNP_XML:%.xml=$(LIBUPNP_DIR)%_client.cpp)
