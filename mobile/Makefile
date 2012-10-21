TOP?=../
DIR:=mobile/
SRCS:= \
	context.cpp \
	main.mm \
	resource_table.mm

mobile:=$(TOP)$(DIR)Chorale

mobileapp:=$(DEST)Chorale.app
mobileapptargz:=$(DEST)Chorale.app.tar.gz

all-mobile:

include $(TOP)Make.common

CLEANS += $(mobile) $(mobileapptargz)

MOBILE_OBJS:=$(LOBJS)
MOBILE_DIR:=$(DIR)

DEST ?= $(TOP)$(MOBILE_DIR)

$(TOP)$(DIR)% : CXXFLAGS += $(BOOST_CXXFLAGS) $(QT_CXXFLAGS) \
		$(TAGLIB_CXXFLAGS) -Wno-shadow

SRCS:= \
	fake.cpp

fake:=$(TOP)$(DIR)fake

include $(TOP)Make.common

FAKE_OBJS:=$(LOBJS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif
ifeq ($(libdb),)
include $(TOP)libdb/Makefile
endif
ifeq ($(libdblocal),)
include $(TOP)libdblocal/Makefile
endif
ifeq ($(libempeg),)
include $(TOP)libempeg/Makefile
endif
ifeq ($(libreceiver),)
include $(TOP)libreceiver/Makefile
endif
ifeq ($(libupnp),)
include $(TOP)libupnp/Makefile
endif
ifeq ($(libmediadb),)
include $(TOP)libmediadb/Makefile
endif
ifeq ($(libdbempeg),)
include $(TOP)libdbempeg/Makefile
endif
ifeq ($(libdbreceiver),)
include $(TOP)libdbreceiver/Makefile
endif
ifeq ($(libdbupnp),)
include $(TOP)libdbupnp/Makefile
endif
ifeq ($(libimport),)
include $(TOP)libimport/Makefile
endif
ifeq ($(libmediatree),)
include $(TOP)libmediatree/Makefile
endif
ifeq ($(liboutput),)
include $(TOP)liboutput/Makefile
endif
ifeq ($(libuidarwin),)
include $(TOP)libuidarwin/Makefile
endif
ifeq ($(libuiqt),)
include $(TOP)libuiqt/Makefile
endif
ifeq ($(libui),)
include $(TOP)libui/Makefile
endif
ifeq ($(libdav),)
include $(TOP)libdav/Makefile
endif
ifeq ($(imagery),)
include $(TOP)imagery/Makefile
endif

ifeq ($(DARWIN),yes)

all: all-mobile

all-mobile: $(mobile) $(mobileapptargz)

$(mobile): $(libutil) $(libimport) $(libdbreceiver) $(libuidarwin) \
		$(libreceiver) $(libdbupnp) $(libmediatree) $(libupnpd) \
		$(libmediadb) $(libupnp) $(libdbempeg) $(libempeg) \
		$(libdbsteam) $(libdb) $(liboutput) $(libdav) $(MOBILE_OBJS) \
		$(TOP)$(MOBILE_DIR)Makefile
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(OBJCXX) -o $@ $(OBJCXXFLAGS) \
		$(MOBILE_OBJS) $(libuidarwin) $(libupnpd) $(libdav) \
		$(libmediatree) $(libdbupnp) $(libimport) \
		$(libdbreceiver) $(liboutput) $(libupnp) $(libmediadb) \
		$(libdbsteam) $(libdb) $(libreceiver)  \
		$(libutil) $(BOOST_LIBS) \
		-F/usr/arm-apple-darwin9/arm-root/System/Library/Frameworks \
		-lobjc -framework CoreFoundation -framework Foundation \
		-framework UIKit -framework AVFoundation -lstdc++

## Packaged Darwin application
#
# To transfer to (jailbroken) target device:
#   rsync -a -e ssh Chorale.app root@target:/Applications
#   ssh root@target
#     ldid -S /Applications/Chorale.app/Chorale
#     rm /private/var/mobile/Library/Caches/com.apple.mobile.installation.plist
#     killall SpringBoard
#
$(mobileapptargz): $(mobile) $(TOP)imagery/icon60.png
	rm -rf $(mobileapp)
	mkdir -p $(mobileapp)/
	cp $(mobile) $(mobileapp)/
	cp $(TOP)imagery/icon60.png $(mobileapp)/icon.png
	cp $(TOP)imagery/320x480.png $(mobileapp)/Default.png
	cp $(TOP)imagery/network.png $(mobileapp)/network.png
	cp $(TOP)$(MOBILE_DIR)Info.plist $(mobileapp)/
	chmod -R g+w $(mobileapp)
	tar czf $(mobileapptargz) -C $(DEST) Chorale.app

else
ifeq ($(QT)$(CAIRO),yesyesno)

all: all-mobile

all-mobile: $(fake)

$(fake): $(libutil) $(libimport) $(libdbreceiver) $(libuiqt) $(libui) \
		$(libreceiver) $(libdbupnp) $(libmediatree) $(libupnpd) \
		$(libmediadb) $(libupnp) $(libdbempeg) $(libempeg) \
		$(libdbsteam) $(libdb) $(liboutput) $(FAKE_OBJS)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(OBJCXX) -o $@ $(OBJCXXFLAGS) \
		$(FAKE_OBJS) $(libuiqt) $(libui) $(libupnpd) \
		$(libmediatree) $(libdbupnp) $(libimport) $(libdbempeg) \
		$(libdbreceiver) $(liboutput) $(libupnp) $(libmediadb) \
		$(libdbsteam) $(libdb) $(libreceiver) $(libempeg) \
		$(libutil) $(BOOST_LIBS) $(QT_LIBS) -lcairo

else

all-mobile:
	@echo "Ignoring mobile Chorale (dependencies not met)"

endif
endif
