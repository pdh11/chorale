TOP?=../
DIR:=libutil/
SRCS:=	async_write_buffer.cpp \
	counted_object.cpp \
	cpus.cpp \
	dbus.cpp \
	file.cpp \
	file_stream.cpp \
	hal.cpp \
	http_client.cpp \
	http_stream.cpp \
	line_reader.cpp \
	memory_stream.cpp \
	multi_stream.cpp \
	partial_stream.cpp \
	poll.cpp \
	quote_escape.cpp \
	socket.cpp \
	ssdp.cpp \
	stream.cpp \
	stream_test.cpp \
	string_stream.cpp \
	task.cpp \
	trace.cpp \
	upnp.cpp \
	urlescape.cpp \
	walker.cpp \
	web_server.cpp \
	worker_thread.cpp \
	worker_thread_pool.cpp \
	xmlescape.cpp

TEST_SRCS:= \
	async_write_buffer.cpp \
	dbus.cpp \
	file.cpp \
	file_stream.cpp \
	hal.cpp \
	http_client.cpp \
	http_stream.cpp \
	line_reader.cpp \
	memory_stream.cpp \
	partial_stream.cpp \
	poll.cpp \
	quote_escape.cpp \
	ssdp.cpp \
	stream.cpp \
	string_stream.cpp \
	urlescape.cpp \
	walker.cpp \
	web_server.cpp \
	xml.cpp \
	xmlescape.cpp

all-util:

all: all-util

include $(TOP)Make.common

libutil:=$(TOP)$(DIR)$(TARGETDIR)libutil.la

all-util: $(libutil)

UTIL_OBJS:=$(LOBJS)
UTIL_TESTS:=$(TESTS)

$(TOP)$(DIR)% : CXXFLAGS += $(BOOST_CXXFLAGS) $(CURL_CXXFLAGS) \
	$(DBUS_CXXFLAGS) $(HAL_CXXFLAGS)

$(UTIL_TESTS): TEST_LIBS+= $(BOOST_LIBS) $(CURL_LIBS) $(UPNP_LIBS) $(DBUS_LIBS) $(HAL_LIBS)

$(UTIL_TESTS): $(libutil)

$(libutil): $(UTIL_OBJS)
	$(AR_PRE)$(LIBTOOL) --tag=CXX --mode=link --quiet \
		$(CC) -o $@ $(UTIL_OBJS)

CLEANS += $(libutil)
