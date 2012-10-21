TOP?=../
DIR:=choralecd/
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
	poller.cpp \
	poller.moc.cpp \
	resource_widget.cpp \
	resource_widget.moc.cpp \
	setlist_window.cpp \
	setlist_window.moc.cpp \
	settings.cpp \
	settings_entry.cpp \
	settings_entry.moc.cpp \
	settings_window.cpp \
	settings_window.moc.cpp \
	tagtable.cpp \
	tagtable.moc.cpp \
	tree_model.cpp \
	tree_model.moc.cpp

choralecd:=$(TOP)$(DIR)choralecd
cloud:=$(TOP)$(DIR)cloud

all-choralecd: $(choralecd) $(cloud)

include $(TOP)Make.common

libchoralecd:=$(TOP)$(DIR)$(TARGETDIR)libchoralecd.a

CLEANS += $(choralecd) $(cloud) $(TOP)$(DIR)*.moc.cpp $(choralecd).map.txt

CHORALECD_OBJS:=$(LOBJS)
CHORALECD_DIR:=$(DIR)

$(TOP)$(DIR)% : CXXFLAGS += $(BOOST_CXXFLAGS) $(QT_CXXFLAGS) \
		$(TAGLIB_CXXFLAGS) -Wno-shadow

ifeq ($(libdb),)
include $(TOP)libdb/Makefile
endif
ifeq ($(libmediadb),)
include $(TOP)libmediadb/Makefile
endif
ifeq ($(libdbreceiver),)
include $(TOP)libdbreceiver/Makefile
endif
ifeq ($(libdbupnp),)
include $(TOP)libdbupnp/Makefile
endif
ifeq ($(libmediatree),)
include $(TOP)libmediatree/Makefile
endif
ifeq ($(libreceiver),)
include $(TOP)libreceiver/Makefile
endif
ifeq ($(liboutput),)
include $(TOP)liboutput/Makefile
endif
ifeq ($(libimport),)
include $(TOP)libimport/Makefile
endif
ifeq ($(libdbempeg),)
include $(TOP)libdbempeg/Makefile
endif
ifeq ($(libempeg),)
include $(TOP)libempeg/Makefile
endif
ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif
ifeq ($(imagery),)
include $(TOP)imagery/Makefile
endif

ifeq ($(QT),yes)

$(libchoralecd): $(CHORALECD_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(CHORALECD_OBJS)

all: all-choralecd
install: install-choralecd

$(choralecd): $(libchoralecd) $(libutil) $(libimport) $(libdbreceiver) \
		$(libreceiver) $(libdbupnp) $(libmediatree) \
		$(libmediadb) $(libupnp) $(libdbempeg) $(libempeg) \
		$(libdbsteam) $(libdb) $(liboutput)
	$(LD_PRE)libtool --tag=CXX --mode=link --quiet \
		$(CXX) -o $@ $(TOP)$(CHORALECD_DIR)$(TARGETDIR)choralecd.lo \
		$(libchoralecd) \
		$(libmediatree) $(libdbupnp) $(libimport) $(libdbempeg) \
		$(libdbreceiver) $(liboutput) $(libupnp) $(libmediadb) \
		$(libdbsteam) $(libdb) $(libreceiver) $(libempeg) $(libupnp) \
		$(libutil) -Wl,--as-needed $(PARANOIA_LIBS) \
		$(LIBCDIOP_LIBS) $(LIBCDDB_LIBS) \
		$(LIBFLAC_LIBS) $(TAGLIB_LIBS) $(BOOST_LIBS) $(QT_LIBS) \
		$(LAME_LIBS) \
		$(GSTREAMER_LIBS) $(HAL_LIBS) -Wl,-Map,$@.map.txt
	@echo $@: `ldd $@ | wc -l` shared libraries | fmt

$(cloud): $(libchoralecd) $(libutil) $(libimport) $(libdbreceiver) \
		$(libreceiver) $(libdbupnp) $(libmediatree) \
		$(libmediadb) $(libupnp) $(libdbempeg) $(libempeg) \
		$(libdbsteam) $(libdb) $(liboutput)
	$(LD_PRE)libtool --tag=CXX --mode=link --quiet \
		$(CXX) -o $@ $(TOP)$(CHORALECD_DIR)$(TARGETDIR)cloud.lo \
		$(libchoralecd) \
		$(libmediatree) $(libdbupnp) $(libimport) $(libdbempeg) \
		$(libdbreceiver) $(liboutput) $(libupnp) $(libmediadb) \
		$(libdbsteam) $(libdb) $(libreceiver) $(libempeg) $(libupnp) \
		$(libutil) \
		-Wl,--as-needed $(PARANOIA_LIBS) \
		$(LIBCDIOP_LIBS) $(LIBCDDB_LIBS) \
		$(LIBFLAC_LIBS) $(TAGLIB_LIBS) $(BOOST_LIBS) $(QT_LIBS) \
		$(LAME_LIBS) \
		$(GSTREAMER_LIBS) $(HAL_LIBS)

else
$(choralecd):
	@echo "Ignoring choralecd (dependencies not met)"
endif
