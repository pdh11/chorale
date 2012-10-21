TOP?=../
DIR:=choraled/
SRCS:=  cd.cpp \
	choraled.cpp \
	choralesvc.cpp \
	database.cpp \
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
ifeq ($(libtv),)
include $(TOP)libtv/Makefile
endif

$(libchoraled): $(CHORALED_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(CHORALED_OBJS)

$(choraled): $(libchoraled) $(libtv) \
		 $(libreceiverd) $(libreceiver) $(libimport) \
		$(libdbsteam) $(libdb) $(libutil) $(libmediadb) \
		$(libupnpd) $(libupnp) $(liboutput) Makefile
	$(LD_PRE)libtool --tag=CXX --mode=link --quiet \
		$(CXX) -o $@-lib $(LDFLAGS) $(TOP)choraled/$(TARGETDIR)$(CHORALED_MAIN).lo \
		-Wl,-Map,map.txt \
		$(libchoraled) $(libtv) \
		$(libreceiverd) $(libreceiver) $(libupnpd) $(libupnp) \
		$(liboutput) \
		$(libimport) $(libdbsteam) $(libmediadb) $(libdb) $(libutil) \
		-Wl,--as-needed $(MPG123_LIBS) \
		$(TAGLIB_LIBS) $(BOOST_LIBS) $(PARANOIA_LIBS) \
		$(GSTREAMER_LIBS) $(HAL_LIBS) $(LIBCDIOP_LIBS) $(LIBS)
ifeq ($(FINAL),no)
	@mv -f $@-lib $@
else
	@grep ^$(TOP)[a-z].*\.a\( map.txt | sed -e 's,/obj-.*[\(],/,' -e 's,.o[\)],.cpp",' -e 's,^,#include ",' | sort \
		 > $@.all-$(FULLTARGET).h
	@$(CXX) $(CXXFLAGS) -I$(TOP). \
		-include $@.all-$(FULLTARGET).h -E -dD \
		-C $(TOP)choraled/$(CHORALED_MAIN).cpp \
		-o $@.all-$(FULLTARGET).i -fwhole-program $(BOOST_CXXFLAGS) \
		$(TAGLIB_CXXFLAGS) \
		$(DBUS_CXXFLAGS) $(HAL_CXXFLAGS) $(GSTREAMER_CXXFLAGS) &
	$(CC_PRE)libtool --tag=CXX --mode=compile --silent \
		$(CXX) $(CXXFLAGS) -I$(TOP). \
		-include $@.all-$(FULLTARGET).h \
		-c $(TOP)choraled/$(CHORALED_MAIN).cpp \
		-o $(TOP)choraled/$(TARGETDIR)all-cpp.lo \
		-fwhole-program $(BOOST_CXXFLAGS) \
		$(TAGLIB_CXXFLAGS) \
		$(DBUS_CXXFLAGS) $(HAL_CXXFLAGS) $(GSTREAMER_CXXFLAGS)
	$(LD_PRE)libtool --tag=CXX --mode=link --quiet \
		$(CXX) -o $@ $(TOP)choraled/$(TARGETDIR)all-cpp.lo \
		-Wl,-Map,map2.txt \
		$(libchoraled) $(libtv) \
		$(libreceiverd) $(libreceiver) $(libupnpd) $(libupnp) \
		$(liboutput) \
		$(libimport) $(libdbsteam) $(libmediadb) $(libdb) $(libutil) \
		-Wl,--as-needed $(MPG123_LIBS) \
		$(TAGLIB_LIBS) $(BOOST_LIBS) $(PARANOIA_LIBS) \
		$(GSTREAMER_LIBS) $(HAL_LIBS) $(LIBCDIOP_LIBS) $(LIBS)
endif
	@$(STRIP) $@
	@echo $@: `ldd $@ | wc -l` shared libraries | fmt

CLEANS += $(CHORALED_DIR)choraled $(CHORALED_DIR)choralesvc.exe \
	$(CHORALED_DIR)map2.txt
