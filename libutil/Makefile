TOP?=../
DIR:=libutil/
SRCS:= \
	async_write_buffer.cpp \
	compare.cpp \
	counted_object.cpp \
	cpus.cpp \
	dbus.cpp \
	file.cpp \
	file_posix.cpp \
	file_win32.cpp \
	file_stream.cpp \
	file_stream_posix.cpp \
	file_stream_win32.cpp \
	formatter.cpp \
	hal.cpp \
	http_client.cpp \
	http_fetcher.cpp \
	http_parser.cpp \
	http_server.cpp \
	http_stream.cpp \
	ip_config.cpp \
	line_reader.cpp \
	locking.cpp \
	memory_stream.cpp \
	multi_stream.cpp \
	mutex.cpp \
	partial_stream.cpp \
	peeking_line_reader.cpp \
	poll_posix.cpp \
	poll_win32.cpp \
	quote_escape.cpp \
	scanf64.cpp \
	scheduler.cpp \
	socket.cpp \
	stream.cpp \
	stream_test.cpp \
	string_stream.cpp \
	task.cpp \
	tcp_wrappers.cpp \
	tcp_wrappers_c.c \
	throw_exception.cpp \
	trace.cpp \
	urlescape.cpp \
	utf8.cpp \
	walker.cpp \
	wmemcpy.cpp \
	worker_thread_pool.cpp \
	xml.cpp \
	xmlescape.cpp

TEST_SRCS:= \
	async_write_buffer.cpp \
	bind.cpp \
	compare.cpp \
	counted_object.cpp \
	dbus.cpp \
	diff.cpp \
	file.cpp \
	file_stream.cpp \
	file_stream_posix.cpp \
	file_stream_win32.cpp \
	formatter.cpp \
	hal.cpp \
	http_client.cpp \
	http_fetcher.cpp \
	http_server.cpp \
	http_stream.cpp \
	ip_config.cpp \
	line_reader.cpp \
	memory_stream.cpp \
	mutex.cpp \
	partial_stream.cpp \
	quote_escape.cpp \
	scanf64.cpp \
	scheduler.cpp \
	stream.cpp \
	string_stream.cpp \
	tcp_wrappers.cpp \
	trace.cpp \
	urlescape.cpp \
	utf8.cpp \
	walker.cpp \
	worker_thread_pool.cpp \
	xml.cpp \
	xmlescape.cpp

all-util:

all: all-util

include $(TOP)Make.common

libutil:=$(TOP)$(DIR)$(TARGETDIR)libutil.a

file_win32:=$(TOP)$(DIR)$(TARGETDIR)file_win32.lo

all-util: $(libutil)

UTIL_OBJS:=$(LOBJS)
UTIL_TESTS:=$(TESTS)

$(TOP)$(DIR)% : CXXFLAGS += $(BOOST_CXXFLAGS) $(DBUS_CXXFLAGS) $(HAL_CXXFLAGS)

$(UTIL_TESTS): TEST_LIBS+= $(libutil) $(file_win32) $(BOOST_LIBS) $(DBUS_LIBS) $(HAL_LIBS) $(LIBS)

$(TOP)$(DIR)$(TARGETDIR)tcp_wrappers__%: LIBS += $(LIBWRAP)

ifneq ($(EXCEPTIONAL_boost_thread_mutex_hpp),)
$(TOP)$(DIR)$(TARGETDIR)mutex%: CXXFLAGS += -fexceptions -UBOOST_NO_EXCEPTIONS
endif

ifneq ($(EXCEPTIONAL_boost_thread_tss_hpp),)
$(TOP)$(DIR)$(TARGETDIR)trace%: CXXFLAGS += -fexceptions -UBOOST_NO_EXCEPTIONS
endif

$(UTIL_TESTS): $(libutil)

$(libutil): $(UTIL_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(UTIL_OBJS)

CLEANS += $(libutil)
