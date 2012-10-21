TOP?=../
DIR:=choraled/
SRCS:=choraled.cpp

choraled:=$(TOP)$(DIR)choraled

all-choraled: $(choraled)

all: all-choraled

include $(TOP)Make.common

CHORALED_OBJS:=$(LOBJS)

$(TOP)$(DIR)% : CXXFLAGS += $(BOOST_CXXFLAGS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif
ifeq ($(libimport),)
include $(TOP)libimport/Makefile
endif
ifeq ($(libdbsteam),)
include $(TOP)libdbsteam/Makefile
endif
ifeq ($(libdb),)
include $(TOP)libdb/Makefile
endif
ifeq ($(libmediadb),)
include $(TOP)libmediadb/Makefile
endif
ifeq ($(libreceiver),)
include $(TOP)libreceiver/Makefile
endif
ifeq ($(libreceiverd),)
include $(TOP)libreceiverd/Makefile
endif
ifeq ($(liboutput),)
include $(TOP)liboutput/Makefile
endif
ifeq ($(libupnp),)
include $(TOP)libupnp/Makefile
endif
ifeq ($(libupnpd),)
include $(TOP)libupnpd/Makefile
endif

$(choraled): $(CHORALED_OBJS) $(libreceiverd) $(libreceiver) $(libimport) \
		$(libdbsteam) $(libdb) $(libutil) $(libmediadb) \
		$(libupnpd) $(libupnp) $(liboutput)
	$(LD_PRE)libtool --tag=CXX --mode=link --quiet \
		 $(CC) -o $@ $(CHORALED_OBJS) -Wl,-Map,map.txt \
		 $(libreceiverd) $(libreceiver) $(libupnpd) $(libupnp) \
		 $(liboutput) \
		 $(libimport) $(libdbsteam) $(libmediadb) $(libdb) $(libutil) \
		 $(TAGLIB_LIBS) $(BOOST_LIBS) $(UPNP_LIBS) $(LIBXMLPP_LIBS) \
		 $(GSTREAMER_LIBS) $(LIBUUID_LIBS) $(OPENSSL_LIBS)
	@echo $@: `ldd $@ | wc -l` shared libraries | fmt

CLEANS += $(choraled)
