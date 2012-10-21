TOP?=../
DIR:=libutil/
SRCS:= \
	async_write_buffer.cpp \
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
	memory_stream.cpp \
	multi_stream.cpp \
	partial_stream.cpp \
	peeking_line_reader.cpp \
	poll.cpp \
	poll_posix.cpp \
	poll_win32.cpp \
	pollable_task.cpp \
	poll_thread.cpp \
	quote_escape.cpp \
	scanf64.cpp \
	sliding_buffer_stream.cpp \
	socket.cpp \
	stream.cpp \
	stream_test.cpp \
	string_stream.cpp \
	task.cpp \
	tcp_wrappers.cpp \
	tcp_wrappers_c.c \
	trace.cpp \
	urlescape.cpp \
	utf8.cpp \
	walker.cpp \
	worker_thread_pool.cpp \
	xml.cpp \
	xmlescape.cpp

TEST_SRCS:= \
	async_write_buffer.cpp \
	dbus.cpp \
	diff.cpp \
	file.cpp \
	file_stream_posix.cpp \
	formatter.cpp \
	hal.cpp \
	http_client.cpp \
	http_fetcher.cpp \
	http_server.cpp \
	http_stream.cpp \
	ip_config.cpp \
	line_reader.cpp \
	memory_stream.cpp \
	partial_stream.cpp \
	poll.cpp \
	pollable_task.cpp \
	quote_escape.cpp \
	scanf64.cpp \
	sliding_buffer_stream.cpp \
	stream.cpp \
	string_stream.cpp \
	tcp_wrappers.cpp \
	urlescape.cpp \
	utf8.cpp \
	walker.cpp \
	xml.cpp \
	xmlescape.cpp

all-util:

all: all-util

include $(TOP)Make.common

libutil:=$(TOP)$(DIR)$(TARGETDIR)libutil.la

all-util: $(libutil)

UTIL_OBJS:=$(LOBJS)
UTIL_TESTS:=$(TESTS)

$(TOP)$(DIR)% : CXXFLAGS += $(BOOST_CXXFLAGS) $(DBUS_CXXFLAGS) $(HAL_CXXFLAGS)

$(UTIL_TESTS): TEST_LIBS+= $(BOOST_LIBS) $(DBUS_LIBS) $(HAL_LIBS) $(LIBS) $(LIBWRAP)

$(UTIL_TESTS): $(libutil)

$(libutil): $(UTIL_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(UTIL_OBJS)

CLEANS += $(libutil)
