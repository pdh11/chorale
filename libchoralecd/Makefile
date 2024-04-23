TOP?=../
DIR:=choralecd/

ifeq ($(QT),yes)

SRCS:= \
	browse_widget.cpp \
	browse_widget.moc.cpp \
	cd_progress.cpp \
	cd_progress.moc.cpp \
	cd_widget.cpp \
	cd_widget.moc.cpp \
	cd_window.cpp \
	cd_window.moc.cpp \
	choralecd.cpp \
	cloud.cpp \
	cloud_database_widget.cpp \
	cloud_database_widget.moc.cpp \
	cloud_style.cpp \
	cloud_style.moc.cpp \
	cloud_upnp_databases.cpp \
	cloud_window.cpp \
	cloud_window.moc.cpp \
	db_widget.cpp \
	db_widget.moc.cpp \
	explorer_window.cpp \
	explorer_window.moc.cpp \
	main_window.cpp \
	main_window.moc.cpp \
	output_widget.cpp \
	output_widget.moc.cpp \
	resource_widget.cpp \
	resource_widget.moc.cpp \
	setlist_model.cpp \
	setlist_model.moc.cpp \
	setlist_window.cpp \
	setlist_window.moc.cpp \
	settings.cpp \
	settings_entry.cpp \
	settings_entry.moc.cpp \
	settings_window.cpp \
	settings_window.moc.cpp \
	tageditor.cpp \
	tag_editor_widget.cpp \
	tag_editor_widget.moc.cpp \
	tree_model.cpp \
	tree_model.moc.cpp

choralecd:=$(TOP)$(DIR)choralecd$(DOTEXE)
cloud:=$(TOP)$(DIR)cloud$(DOTEXE)
tageditor:=$(TOP)$(DIR)tageditor$(DOTEXE)

all-choralecd: $(choralecd) $(cloud) $(tageditor)

include $(TOP)Make.common

libchoralecd:=$(TOP)$(DIR)$(TARGETDIR)libchoralecd.a

CLEANS += $(choralecd) $(cloud) $(TOP)$(DIR)*.moc.cpp $(choralecd).map.txt

CHORALECD_OBJS:=$(LOBJS)
CHORALECD_DIR:=$(DIR)

ifneq ($(WINDRES),:)
CHORALECD_RESOURCES:=$(TOP)$(DIR)$(TARGETDIR)choralecd.res.o
else
CHORALECD_RESOURCES:=
endif

$(TOP)$(DIR)% : CXXFLAGS += $(BOOST_CXXFLAGS) $(QT_CXXFLAGS) \
		$(TAGLIB_CXXFLAGS) -Wno-shadow

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif
ifeq ($(libdb),)
include $(TOP)libdb/Makefile
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
ifeq ($(libuiqt),)
include $(TOP)libuiqt/Makefile
endif
ifeq ($(imagery),)
include $(TOP)imagery/Makefile
endif

$(libchoralecd): $(CHORALECD_OBJS)
	rm -f $@
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(CHORALECD_OBJS)

all: all-choralecd
install: install-choralecd

$(choralecd): $(libchoralecd) $(libutil) $(libimport) $(libdbreceiver) \
		$(libreceiver) $(libdbupnp) $(libmediatree) \
		$(libmediadb) $(libupnp) $(libdbempeg) $(libempeg) \
		$(libdbsteam) $(libdb) $(liboutput) $(libuiqt) \
		$(CHORALECD_RESOURCES)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CXX) -o $@ $(LDFLAGS) \
		$(TOP)$(CHORALECD_DIR)$(TARGETDIR)choralecd.lo \
		$(CHORALECD_RESOURCES) \
		$(libchoralecd) $(libuiqt) \
		$(libmediatree) $(libdbupnp) $(libimport) $(libdbempeg) \
		$(libdbreceiver) $(liboutput) $(libupnp) $(libmediadb) \
		$(libdbsteam) $(libdb) $(libreceiver) $(libempeg) $(libupnp) \
		$(libutil) -Wl,--as-needed $(PARANOIA_LIBS) \
		$(LIBCDIOP_LIBS) $(LIBCDDB_LIBS) \
		$(LIBFLAC_LIBS) $(TAGLIB_LIBS) $(BOOST_LIBS) $(QT_LIBS) \
		$(LAME_LIBS) $(GSTREAMER_LIBS) $(HAL_LIBS) $(LIBS) \
		-Wl,-Map,map-choralecd-$(FULLTARGET).txt
ifneq ($(FINAL),no)
	@$(RM) $@
	@grep ^$(TOP)[a-z].*\.a\( map-choralecd-$(FULLTARGET).txt | sed -e 's,/obj-.*[\(],/,' -e 's,.o[\)],.cpp",' -e 's,^,#include ",' -e 's,/[a-z0-9_]*.moc.cpp,/obj-$(FULLTARGET)&,' | \
		grep -v tcp_wrappers_c.c | grep -v mutex.cpp | \
		grep -v search.cpp | sort \
		 > $(TOP)choralecd/all-$(FULLTARGET).cpp
	@$(CXX) $(CXXFLAGS) -I$(TOP). \
		-include $(TOP)choralecd/all-$(FULLTARGET).cpp \
		-c $(TOP)choralecd/choralecd.cpp \
		-o $(TOP)choralecd/all-$(FULLTARGET).i \
		-fwhole-program $(BOOST_CXXFLAGS) \
		$(TAGLIB_CXXFLAGS) -E -C -dD \
		$(DBUS_CXXFLAGS) $(HAL_CXXFLAGS) $(GSTREAMER_CXXFLAGS) &
	$(CC_PRE)$(LIBTOOL) --tag=CXX --mode=compile --silent \
		$(CXX) $(CXXFLAGS) -I$(TOP). \
		-include $(TOP)choralecd/all-$(FULLTARGET).cpp \
		-c $(TOP)choralecd/choralecd.cpp \
		-o $(TOP)choralecd/$(TARGETDIR)all-cpp.lo \
		-fwhole-program $(BOOST_CXXFLAGS) \
		$(TAGLIB_CXXFLAGS) \
		$(DBUS_CXXFLAGS) $(HAL_CXXFLAGS) $(GSTREAMER_CXXFLAGS)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CXX) -o $@ $(LDFLAGS) \
		$(TOP)choralecd/$(TARGETDIR)all-cpp.lo \
		$(CHORALECD_RESOURCES) \
		-Wl,-Map,map-choralecd-$(FULLTARGET)-final.txt \
		$(libchoralecd) $(libuiqt) \
		$(libmediatree) $(libdbupnp) $(libimport) $(libdbempeg) \
		$(libdbreceiver) $(liboutput) $(libupnp) $(libmediadb) \
		$(libdbsteam) $(libdb) $(libreceiver) $(libempeg) $(libupnp) \
		$(libutil) -Wl,--as-needed $(PARANOIA_LIBS) \
		$(LIBCDIOP_LIBS) $(LIBCDDB_LIBS) \
		$(LIBFLAC_LIBS) $(TAGLIB_LIBS) $(BOOST_LIBS) $(QT_LIBS) \
		$(LAME_LIBS) \
		$(GSTREAMER_LIBS) $(HAL_LIBS) $(LIBS)
endif
	@$(STRIP) $@
	@echo $@: `ldd $@ | wc -l` shared libraries | fmt

$(cloud): $(libchoralecd) $(libutil) $(libimport) $(libdbreceiver) \
		$(libreceiver) $(libdbupnp) $(libmediatree) \
		$(libmediadb) $(libupnp) $(libdbempeg) $(libempeg) \
		$(libdbsteam) $(libdb) $(liboutput) $(libuiqt) \
		$(CHORALECD_RESOURCES)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CXX) -o $@ $(LDFLAGS) \
		$(TOP)$(CHORALECD_DIR)$(TARGETDIR)cloud.lo \
		$(CHORALECD_RESOURCES) \
		$(libchoralecd) $(libuiqt) \
		$(libmediatree) $(libdbupnp) $(libimport) $(libdbempeg) \
		$(libdbreceiver) $(liboutput) $(libupnp) $(libmediadb) \
		$(libdbsteam) $(libdb) $(libreceiver) $(libempeg) $(libupnp) \
		$(libutil) \
		-Wl,--as-needed $(PARANOIA_LIBS) \
		$(LIBCDIOP_LIBS) $(LIBCDDB_LIBS) \
		$(LIBFLAC_LIBS) $(TAGLIB_LIBS) $(BOOST_LIBS) $(QT_LIBS) \
		$(LAME_LIBS) \
		$(GSTREAMER_LIBS) $(HAL_LIBS) $(LIBS)

$(tageditor): $(libchoralecd) $(libutil) $(libimport) $(libdblocal) $(libdb) \
		$(libuiqt) $(libmediatree) $(libdbsteam) $(CHORALECD_RESOURCES)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CXX) -o $@ $(LDFLAGS) \
		$(TOP)$(CHORALECD_DIR)$(TARGETDIR)tageditor.lo \
		$(CHORALECD_RESOURCES) \
		$(libchoralecd) $(libuiqt) \
		$(libmediatree) \
		$(libmediadb) $(libdblocal) \
		$(libdbsteam) $(libdb) $(libimport) \
		$(libutil) \
		$(LIBCDIOP_LIBS) $(LIBCDDB_LIBS) \
		$(TAGLIB_LIBS) $(BOOST_LIBS) $(QT_LIBS) \
                $(AVFORMAT_LIBS) $(LAME_LIBS) $(LIBFLAC_LIBS) \
		$(LIBS)

else
$(choralecd):
	@echo "Ignoring choralecd (dependencies not met)"
endif
