TOP?=../
DIR:=choraled/
SRCS:=  cd.cpp \
	choraled.cpp \
	choralesvc.cpp \
	database.cpp \
	ip_filter.cpp \
	main.cpp \
	nfs.cpp \
	web.cpp

all-choraled:

all: all-choraled

include $(TOP)Make.common

ifeq ($(WIN32),yes)
choraled:=$(TOP)$(DIR)choralesvc.exe
CHORALED_MAIN:=choralesvc
else
choraled:=$(TOP)$(DIR)choraled
CHORALED_MAIN:=choraled
endif

all-choraled: $(choraled)

libchoraled:=$(TOP)$(DIR)$(TARGETDIR)libchoraled.a

CHORALED_OBJS:=$(LOBJS)

CHORALED_DIR:=$(TOP)$(DIR)

$(TOP)$(DIR)% : CXXFLAGS += $(BOOST_CXXFLAGS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif
ifeq ($(libupnp),)
include $(TOP)libupnp/Makefile
endif
ifeq ($(libimport),)
include $(TOP)libimport/Makefile
endif
ifeq ($(libdbsteam),)
include $(TOP)libdbsteam/Makefile
endif
ifeq ($(libdblocal),)
include $(TOP)libdblocal/Makefile
endif
ifeq ($(libdbreceiver),)
include $(TOP)libdbreceiver/Makefile
endif
ifeq ($(libdbmerge),)
include $(TOP)libdbmerge/Makefile
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
ifeq ($(libtv),)
include $(TOP)libtv/Makefile
endif
ifeq ($(libupnpd),)
include $(TOP)libupnpd/Makefile
endif

$(libchoraled): $(CHORALED_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(CHORALED_OBJS)

NON_FINAL_SOURCE:= tcp_wrappers_c.c search.cpp mutex.cpp

ifeq ($(DARWIN),no)

$(choraled): $(libchoraled) $(libtv) $(libdblocal) $(libdbmerge) \
		$(libreceiverd) $(libdbreceiver) $(libreceiver) $(libimport) \
		$(libdbsteam) $(libdb) $(libutil) $(libmediadb) \
		$(libupnpd) $(libupnp) $(liboutput) Makefile
	$(LD_PRE)libtool --tag=CXX --mode=link --quiet \
		$(CXX) -o $@ $(LDFLAGS) $(TOP)choraled/$(TARGETDIR)$(CHORALED_MAIN).lo \
		$(LDMAP) \
		$(libchoraled) $(libtv) $(libdbreceiver) \
		$(libreceiverd) $(libreceiver) $(libupnpd) $(libupnp) \
		$(liboutput) $(libdblocal) $(libdbmerge) \
		$(libimport) $(libdbsteam) $(libmediadb) $(libdb) $(libutil) \
		$(MPG123_LIBS) \
		$(TAGLIB_LIBS) $(BOOST_LIBS) $(PARANOIA_LIBS) $(LIBWRAP) \
		$(GSTREAMER_LIBS) $(HAL_LIBS) $(LIBCDIOP_LIBS) $(LIBS)
ifneq ($(FINAL),no)
	@$(RM) $@
	@grep ^$(TOP)[a-z].*\.a\( map-choraled-$(FULLTARGET).txt | \
		sed -e 's,/obj-.*[\(],/,' -e 's,.o[\)],.cpp",' \
			-e 's,^,#include ",' | \
		grep -v tcp_wrappers_c.c | grep -v mutex.cpp | \
		grep -v search.cpp | grep -v whole.cpp | sort \
		 > $(TOP)choraled/all-$(FULLTARGET).h
	@$(CXX) $(CXXFLAGS) -I$(TOP). \
		-include $(TOP)choraled/all-$(FULLTARGET).h -E -dD \
		-C $(TOP)choraled/$(CHORALED_MAIN).cpp \
		-o $(TOP)choraled/$(TARGETDIR)all.i -fwhole-program \
		$(BOOST_CXXFLAGS) $(TAGLIB_CXXFLAGS) \
		$(DBUS_CXXFLAGS) $(HAL_CXXFLAGS) $(GSTREAMER_CXXFLAGS) &
	$(CC_PRE)libtool --tag=CXX --mode=compile --silent \
		$(CXX) $(CXXFLAGS) -I$(TOP). \
		-include $(TOP)choraled/all-$(FULLTARGET).h \
		-c $(TOP)choraled/$(CHORALED_MAIN).cpp \
		-o $(TOP)choraled/$(TARGETDIR)all.lo \
		-fwhole-program $(BOOST_CXXFLAGS) \
		$(TAGLIB_CXXFLAGS) \
		$(DBUS_CXXFLAGS) $(HAL_CXXFLAGS) $(GSTREAMER_CXXFLAGS)
	$(LD_PRE)libtool --tag=CXX --mode=link --quiet \
		$(CXX) -o $@ $(TOP)choraled/$(TARGETDIR)all.lo \
		-fno-exceptions -Wl,-z,combreloc \
		-Wl,-Map,map-choraled-$(FULLTARGET)-final.txt -Wl,--gc-sections \
		$(libchoraled) $(libtv) \
		$(libreceiverd) $(libreceiver) $(libupnpd) $(libupnp) \
		$(liboutput) \
		$(libimport) $(libdbsteam) $(libmediadb) $(libdb) $(libutil) \
		$(MPG123_LIBS) \
		$(TAGLIB_LIBS) $(BOOST_LIBS) $(PARANOIA_LIBS) $(LIBWRAP) \
		$(GSTREAMER_LIBS) $(HAL_LIBS) $(LIBCDIOP_LIBS) $(LIBS)
endif
	@$(STRIP) $@
	@echo $@: `ldd $@ | wc -l` shared libraries | fmt

CLEANS += $(CHORALED_DIR)choraled $(CHORALED_DIR)choralesvc.exe

else

$(choraled):
	@echo "Ignoring choraled (dependencies not met)"

endif
