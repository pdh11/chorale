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

all-choralecd: $(choralecd)

include $(TOP)Make.common

CLEANS += $(choralecd) $(TOP)$(DIR)*.moc.cpp

CHORALECD_OBJS:=$(LOBJS)

$(TOP)$(DIR)% : CXXFLAGS += $(BOOST_CXXFLAGS) $(QT_CXXFLAGS) $(TAGLIB_CXXFLAGS) -Wno-shadow

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
ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif

ifeq ($(QT),yes)

all: all-choralecd
install: install-choralecd

$(choralecd): $(CHORALECD_OBJS) $(libutil) $(libimport) $(libreceiver) \
		$(libdb) $(libdbreceiver) $(libdbupnp) $(libmediatree) \
		$(libmediadb) $(libupnp) \
		$(liboutput)
	$(LD_PRE)libtool --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(CHORALECD_OBJS) $(LIBCDDB_LIBS) \
		$(libmediatree) $(libdbupnp) \
		$(libdbreceiver) $(liboutput) $(libupnp) $(libmediadb) \
		$(libdb) $(libimport) $(libreceiver) \
		$(libutil) $(LIBCDIOP_LIBS) \
		$(LIBFLAC_LIBS) $(TAGLIB_LIBS) $(BOOST_LIBS) $(QT_LIBS) \
		$(LAME_LIBS) $(LIBXMLPP_LIBS) $(CURL_LIBS) $(UPNP_LIBS) \
		$(GSTREAMER_LIBS) -Wl,-Map,$@.map.txt
	@echo $@: `ldd $@ | wc -l` shared libraries | fmt

else
$(choralecd):
	@echo "Ignoring choralecd (dependencies not met)"
endif
