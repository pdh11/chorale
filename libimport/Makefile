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
	encoding_task_flac.cpp \
	encoding_task_mp3.cpp \
	file_notifier.cpp \
	playlist.cpp \
	playlist_asx.cpp \
	playlist_pls.cpp \
	playlist_wpl.cpp \
	remote_cd_drive.cpp \
	ripping_control.cpp \
	ripping_task.cpp \
	tag_rename_task.cpp \
	tags.cpp \
	tags_flac.cpp \
	tags_mp3.cpp \
	vorbis_comment.cpp

TEST_SRCS:=\
	cd_drives.cpp \
	file_notifier.cpp \
	playlist_asx.cpp \
	playlist_pls.cpp \
	playlist_wpl.cpp

all-libimport:

all: all-libimport

include $(TOP)Make.common

LIBIMPORT_OBJS:=$(LOBJS)
LIBIMPORT_TESTS:=$(TESTS)

libimport:=$(TOP)$(DIR)$(TARGETDIR)libimport.a

all-libimport: $(libimport)

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

$(LIBIMPORT_TESTS) : TEST_LIBS += $(libimport) $(libutil) \
	$(BOOST_LIBS) $(TAGLIB_LIBS) $(PARANOIA_LIBS) $(HAL_LIBS) \
	$(LIBCDIOP_LIBS) $(LIBS)

$(LIBIMPORT_TESTS) : $(libimport) $(libmediadb) \
	$(libdb) $(libutil)

$(libimport): $(LIBIMPORT_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		 $(CXX) -o $@ $(LIBIMPORT_OBJS)

CLEANS += $(libimport)
