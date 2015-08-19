TOP?=../
DIR:=libimport/
SRCS:=\
	audio_cd.cpp \
	cddb_service.cpp \
	cd_content_factory.cpp \
	cd_drives.cpp \
	cd_toc_task.cpp \
	eject_task.cpp \
	encoding_task.cpp \
	file_notifier.cpp \
	flac_encoder.cpp \
	mp3_encoder.cpp \
	playlist.cpp \
	playlist_io.cpp \
	playlist_asx.cpp \
	playlist_pls.cpp \
	playlist_wpl.cpp \
	remote_cd_drive.cpp \
	ripping_control.cpp \
	ripping_engine.cpp \
	ripping_task.cpp \
	scsi.cpp \
	scsi_linux.cpp \
	scsi_win32.cpp \
	tag_reader.cpp \
	tag_rename_task.cpp \
	tags.cpp \
	tags_flac.cpp \
	tags_mp3.cpp \
	test_cd.cpp \
	vorbis_comment.cpp

TEST_SRCS:=\
	cd_drives.cpp \
	encoding_task.cpp \
	file_notifier.cpp \
	playlist_asx.cpp \
	playlist_pls.cpp \
	playlist_wpl.cpp \
	ripping_control.cpp

all-libimport:

all: all-libimport

include $(TOP)Make.common

LIBIMPORT_OBJS:=$(LOBJS)
LIBIMPORT_TESTS:=$(TESTS)

libimport:=$(TOP)$(DIR)$(TARGETDIR)libimport.a

rip:=$(TOP)$(DIR)rip$(DOTEXE)
RIP_OBJS:=$(TOP)$(DIR)$(TARGETDIR)rip.lo

rip2:=$(TOP)$(DIR)rip2$(DOTEXE)
RIP2_OBJS:=$(TOP)$(DIR)$(TARGETDIR)2rip.lo

all-libimport: $(libimport) $(rip) $(rip2)

$(TOP)$(DIR)% : CXXFLAGS += $(TAGLIB_CXXFLAGS) $(BOOST_CXXFLAGS) \
	$(LIBCDIOP_CXXFLAGS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
endif
ifeq ($(libdb),)
include $(TOP)libdb/Makefile
endif
ifeq ($(libmediadb),)
include $(TOP)libmediadb/Makefile
endif

$(LIBIMPORT_TESTS) : TEST_LIBS += $(libimport) $(libdb) $(libutil) \
	$(BOOST_LIBS) $(TAGLIB_LIBS) $(PARANOIA_LIBS) $(HAL_LIBS) \
	$(LIBCDIOP_LIBS) $(LAME_LIBS) $(LIBFLAC_LIBS) $(LIBS)

$(LIBIMPORT_TESTS) : $(libimport) $(libmediadb) \
	$(libdb) $(libutil)

$(libimport): $(LIBIMPORT_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		 $(CXX) -o $@ $(LIBIMPORT_OBJS)

$(rip): $(RIP_OBJS) $(libimport) $(libutil)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --silent \
		$(CXX) -o $@ $(LDFLAGS) $(RIP_OBJS) \
		$(libimport) $(libutil) \
		$(PARANOIA_LIBS) $(LIBCDIOP_LIBS) $(BOOST_LIBS) $(LIBS)

$(rip2): $(RIP2_OBJS) $(libimport) $(libutil)
	$(LD_PRE)$(LIBTOOL) --tag=CXX --mode=link --silent \
		$(CXX) -o $@ $(LDFLAGS) $(RIP2_OBJS) \
		$(libimport) $(libdb) $(libutil) $(TAGLIB_LIBS) $(HAL_LIBS) \
		$(PARANOIA_LIBS) $(LIBCDIOP_LIBS) $(BOOST_LIBS) \
		$(LAME_LIBS) $(LIBFLAC_LIBS) $(LIBS)

CLEANS += $(libimport) $(rip)
