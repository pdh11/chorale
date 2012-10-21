TOP?=../
DIR:=libupnp/
LIBUPNP_DIR:=$(TOP)$(DIR)
SRCS:=\
	client.cpp \
	description.cpp \
	device.cpp \
	server.cpp \
	soap.cpp \
	ssdp.cpp

LIBUPNP_XML:= \
	AVTransport2.xml \
	ConnectionManager2.xml \
	ContentDirectory3.xml \
	OpticalDrive.xml \
	RenderingControl2.xml \
	ScheduledRecording2.xml \
	TestService.xml

SRCS:=$(sort $(SRCS) $(LIBUPNP_XML:%.xml=%.cpp) $(LIBUPNP_XML:%.xml=%_server.cpp) $(LIBUPNP_XML:%.xml=%_client.cpp))

TEST_SRCS:=\
	ssdp.cpp \
	test_service.cpp

all-libupnp:

all: all-libupnp

include $(TOP)Make.common

LIBUPNP_OBJS:=$(LOBJS)
LIBUPNP_TESTS:=$(TESTS)

libupnp:=$(TOP)$(DIR)$(TARGETDIR)libupnp.a

all-libupnp: $(libupnp)

$(TOP)$(DIR)% : CXXFLAGS += $(TAGLIB_CXXFLAGS) $(BOOST_CXXFLAGS) \
	$(LIBCDIOP_CXXFLAGS)

ifeq ($(HAVE_XSLTPROC),yes)

$(TOP)$(DIR)%.h : $(TOP)$(DIR)%.xml $(LIBUPNP_DIR)scpd-header.xsl \
		$(LIBUPNP_DIR)scpd-common.xsl
	$(GEN_PRE)$(XSLTPROC) --stringparam class $* \
		    -o $@ \
		    $(LIBUPNP_DIR)scpd-header.xsl $<

$(TOP)$(DIR)%.cpp : $(TOP)$(DIR)%.xml $(LIBUPNP_DIR)scpd-source.xsl \
		$(LIBUPNP_DIR)scpd-common.xsl
	$(GEN_PRE)$(XSLTPROC) --stringparam class $* \
		    -o $@ \
		    $(LIBUPNP_DIR)scpd-source.xsl $<

$(TOP)$(DIR)%_client.h : $(TOP)$(DIR)%.xml \
		$(LIBUPNP_DIR)scpd-clientheader.xsl \
		$(LIBUPNP_DIR)scpd-common.xsl
	$(GEN_PRE)$(XSLTPROC) --stringparam class $* \
		    -o $@ \
		    $(LIBUPNP_DIR)scpd-clientheader.xsl $<

$(TOP)$(DIR)%_client.cpp : $(TOP)$(DIR)%.xml \
		$(LIBUPNP_DIR)scpd-clientsource.xsl \
		$(LIBUPNP_DIR)scpd-common.xsl
	$(GEN_PRE)$(XSLTPROC) --stringparam class $* \
		    --stringparam mode client \
		    -o $@ \
		    $(LIBUPNP_DIR)scpd-clientsource.xsl $<

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
ifeq ($(libdbsteam),)
include $(TOP)libdbsteam/Makefile
endif

$(LIBUPNP_TESTS): TEST_LIBS+= $(libupnp) $(libutil) $(BOOST_LIBS) $(LIBS)

$(LIBUPNP_TESTS): $(libdbsteam) $(libdb) $(libupnp) $(libutil)

ifneq ($(EXCEPTIONAL_boost_thread_tss_hpp),)
$(LIBUPNP_DIR)$(TARGETDIR)server%: CXXFLAGS += -fexceptions -UBOOST_NO_EXCEPTIONS
endif

$(libupnp): $(LIBUPNP_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(LIBUPNP_OBJS)

$(upnpscan): $(UPNPSCAN_OBJS) $(libupnp) \
		$(libdb) $(libutil) Makefile
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(RECEIVERSCAN_OBJS) $(libupnp) \
		$(libdb) $(libutil) \
		$(BOOST_LIBS)

-include $(UPNPSCAN_OBJS:%.lo=%.lo.dep)

CLEANS += $(libupnp)

MAINTAINERCLEANS += \
	$(LIBUPNP_XML:%.xml=$(LIBUPNP_DIR)%.h) \
	$(LIBUPNP_XML:%.xml=$(LIBUPNP_DIR)%.cpp) \
	$(LIBUPNP_XML:%.xml=$(LIBUPNP_DIR)%_server.h) \
	$(LIBUPNP_XML:%.xml=$(LIBUPNP_DIR)%_server.cpp) \
	$(LIBUPNP_XML:%.xml=$(LIBUPNP_DIR)%_client.h) \
	$(LIBUPNP_XML:%.xml=$(LIBUPNP_DIR)%_client.cpp)
