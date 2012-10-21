TOP?=

all:

include $(TOP)Make.common

ifeq ($(libdb),)
include libdb/Makefile
endif
ifeq ($(libimport),)
include libimport/Makefile
endif
ifeq ($(libreceiver),)
include libreceiver/Makefile
endif
ifeq ($(libreceiverd),)
include libreceiverd/Makefile
endif
ifeq ($(libupnp),)
include libupnp/Makefile
endif
ifeq ($(libupnpd),)
include libupnpd/Makefile
endif
ifeq ($(liboutput),)
include liboutput/Makefile
endif
ifeq ($(libdbsteam),)
include libdbsteam/Makefile
endif
ifeq ($(libdbupnp),)
include libdbupnp/Makefile
endif
ifeq ($(libutil),)
include libutil/Makefile
endif
ifeq ($(choraleutil),)
include choraleutil/Makefile
endif
ifeq ($(choraled),)
include choraled/Makefile
endif
ifeq ($(choralecd),)
include choralecd/Makefile
endif

# Autoconf stuff for remaking 'configure' and 'Make.config'

configure: configure.ac aclocal.m4
	autoconf

configure:=configure

# autoheader might not change config.h.in, so touch a stamp file.
config.h.in: stamp-h.in

stamp-h.in: configure.ac aclocal.m4
	autoheader
	echo timestamp > stamp-h.in

config.$(TARGET).h: stamp-h
stamp-h: config.h.in config.status
	./config.status

Make.config.$(TARGET): Make.config.in config.status
	./config.status

config.status: configure stamp-h.in
	[ -x config.status ] || ./configure
	./config.status --recheck

aclocal.m4: configure.ac
	aclocal -I autotools

doc:
	doxygen Doxyfile

.PHONY: distclean clean release doc

distclean: clean
	rm -rf config.*.h Make.config.*-* stamp-h config.log config.status \
		autom4te.cache libtool.*
	find . -name diff.txt -exec rm -f \{} \;
	find . -name map.txt -exec rm -f \{} \;
	find . -name '*.dep' -exec rm -f \{} \;
	find . -name '*.o' -exec rm -f \{} \;
	find . -name '*~' -exec rm -f \{} \;

install: $(choraled)
	$(INSTALL) -d $(datadir)/chorale/upnp
	$(INSTALL) libupnp/AVTransport2.xml $(datadir)/chorale/upnp/AVTransport.xml
	$(INSTALL) -d $(localstatedir)/chorale
	$(INSTALL) -d $(bindir)
	$(INSTALL) -s $(choraled) $(bindir)

install-choralecd: $(choralecd)
	$(INSTALL) -d $(bindir)
	$(INSTALL) -s $(choralecd) $(bindir)

CHORALE:=chorale-$(CHORALE_VERSION)

release: distclean
	rm -f chorale*.tar.bz2
	tar cf $(CHORALE).tar --transform=s,./,$(CHORALE)/, \
		--exclude=$(CHORALE).tar --exclude=CVS --exclude=doc \
		--exclude=.libs --exclude=.cvsignore \
		 .
	bzip2 -9 $(CHORALE).tar

SUBDIRS:= libdb libdbreceiver libimport libmediatree libreceiver \
	libdbsteam libutil choraled choralecd libupnpd \
	libmediadb libreceiverd liboutput choraleutil libupnp libdbupnp libtv

libdeps.dot: Makefile
	echo "digraph G {" > $@
	for i in $(SUBDIRS) ; do \
		grep "^#include.*\".*/" $$i/*.{h,cpp} \
			| sed -e 's,/[^\"]*\", -> ,'  -e s,/[^/]*\",, ; \
	done | sort | uniq >> $@
	echo "libdb -> libutil" >> $@
	echo "}" >> $@

filedeps.dot: Makefile
	echo "digraph G {" > $@
	for i in `find . -name '*.h'` ; do \
		g++ -MP -M $$i -I. 2>/dev/null | grep '^[a-z].*:$$' \
			| sed -e 's,^,"'$$i'" -> ",' -e 's,:$$,",' \
				-e 's,^"./,",' ; \
	done >> $@
	echo "}" >> $@

%.png: %.dot
	tred $< | dot -Tpng -o $@

CLEANS += $(TOP)filedeps.dot $(TOP)libdeps.dot

#  -e s,/[^/]*\",, ; \
