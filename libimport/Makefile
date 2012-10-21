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
	file_scanner.cpp \
	file_scanner_thread.cpp \
	playlist.cpp \
	playlist_asx.cpp \
	playlist_wpl.cpp \
	remote_cd_drive.cpp \
	ripping_task.cpp \
	tag_rename_task.cpp \
	tags.cpp \
	tags_flac.cpp \
	tags_mp3.cpp \
	vorbis_comment.cpp

TEST_SRCS:=\
	cd_drives.cpp \
	file_notifier.cpp \
	file_scanner.cpp \
	playlist_asx.cpp \
	playlist_wpl.cpp

all-libimport:

all: all-libimport

include $(TOP)Make.common

LIBIMPORT_OBJS:=$(LOBJS)
LIBIMPORT_TESTS:=$(TESTS)

libimport:=$(TOP)$(DIR)$(TARGETDIR)libimport.la

all-libimport: $(libimport)

$(TOP)$(DIR)% : CXXFLAGS += $(TAGLIB_CXXFLAGS) $(BOOST_CXXFLAGS) \
	$(LIBCDIOP_CXXFLAGS)

ifeq ($(libutil),)
include $(TOP)libutil/Makefile
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

$(LIBIMPORT_TESTS) : TEST_LIBS += $(BOOST_LIBS) $(TAGLIB_LIBS) \
	$(HAL_LIBS) $(LIBCDIOP_LIBS)

$(LIBIMPORT_TESTS) : $(libimport) $(libmediadb) $(libdbsteam) \
	$(libdb) $(libutil)

$(libimport): $(LIBIMPORT_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		 $(CC) -o $@ $(LIBIMPORT_OBJS)

CLEANS += $(libimport)
