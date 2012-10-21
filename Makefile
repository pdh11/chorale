TOP?=

all:

include $(TOP)Make.common

ifeq ($(choraleutil),)
include $(TOP)choraleutil/Makefile
endif
ifeq ($(choraled),)
include $(TOP)choraled/Makefile
endif
ifeq ($(choralecd),)
include $(TOP)choralecd/Makefile
endif

# Autoconf stuff for remaking 'configure' and 'Make.config'

$(TOP)configure: $(TOP)configure.ac $(TOP)aclocal.m4
	autoconf

configure:=configure

# autoheader might not change config.h.in, so touch a stamp file.
$(TOP)config.h.in: $(TOP)stamp-h.in

$(TOP)stamp-h.in: $(TOP)configure.ac $(TOP)aclocal.m4
	autoheader
	echo timestamp > stamp-h.in

config.$(TARGET).h: stamp-h
stamp-h: $(TOP)config.h.in config.status
	./config.status

Make.config.$(TARGET) $(LIBTOOL): $(TOP)Make.config.in config.status
	./config.status

config.status: $(TOP)configure $(TOP)stamp-h.in
	[ -x config.status ] || ./configure
	./config.status --recheck

$(TOP)aclocal.m4: $(TOP)configure.ac
	aclocal -I autotools

doc:
	doxygen Doxyfile

.PHONY: distclean clean release doc

distclean: clean
	rm -rf config.*.h Make.config.*-* stamp-h config.log config.status \
		autom4te.cache libtool.* libtool
	find . -name diff.txt -exec rm -f \{} \;
	find . -name map.txt -exec rm -f \{} \;
	find . -name '*.dep' -exec rm -f \{} \;
	find . -name '*.o' -exec rm -f \{} \;
	find . -name '*~' -exec rm -f \{} \;

maintainerclean: distclean
	rm -f $(MAINTAINERCLEANS)

install: $(choraled)
	$(INSTALL) -d $(datadir)/chorale/upnp
	$(INSTALL) libupnp/AVTransport2.xml $(datadir)/chorale/upnp/AVTransport.xml
	$(INSTALL) libupnp/RenderingControl2.xml $(datadir)/chorale/upnp/RenderingControl.xml
	$(INSTALL) libupnp/ContentDirectory2.xml $(datadir)/chorale/upnp/ContentDirectory.xml
	$(INSTALL) libupnp/OpticalDrive.xml $(datadir)/chorale/upnp/OpticalDrive.xml
	$(INSTALL) -d $(datadir)/chorale/layout
	$(INSTALL) -m644 imagery/default.css $(datadir)/chorale/layout/default.css
	$(INSTALL) -m644 imagery/icon32.png  $(datadir)/chorale/layout/icon.png
	$(INSTALL) -m644 imagery/icon32s.png $(datadir)/chorale/layout/iconsel.png
	$(INSTALL) -m644 imagery/icon.ico    $(datadir)/chorale/layout/icon.ico
	$(INSTALL) -d $(datadir)/icons/hicolor/16x16/apps
	$(INSTALL) -m644 imagery/icon16.png $(datadir)/icons/hicolor/16x16/apps/choralecd.png
	$(INSTALL) -d $(datadir)/icons/hicolor/32x32/apps
	$(INSTALL) -m644 imagery/icon32.png $(datadir)/icons/hicolor/32x32/apps/choralecd.png
	$(INSTALL) -d $(datadir)/icons/hicolor/48x48/apps
	$(INSTALL) -m644 imagery/icon48.png $(datadir)/icons/hicolor/48x48/apps/choralecd.png
	$(INSTALL) -m644 imagery/noart32.png $(datadir)/chorale/layout/
	$(INSTALL) -m644 imagery/noart48.png $(datadir)/chorale/layout/
	$(INSTALL) -m644 imagery/search-amazon.png $(datadir)/chorale/layout/
	$(INSTALL) -m644 imagery/search-imdb.png $(datadir)/chorale/layout/
	$(INSTALL) -m644 imagery/search-google.png $(datadir)/chorale/layout/
	$(INSTALL) -m644 imagery/search-wikipedia.png $(datadir)/chorale/layout/
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

winrelease: all
	rm -f $(CHORALE)-win32.zip
	rm -rf wintemp
	mkdir -p wintemp
	cp $(TOP)choraled/choralesvc.exe wintemp
	cp /usr/i586-mingw32/bin/mingwm10.dll wintemp
	zip -j -m -r $(CHORALE)-win32.zip wintemp
	mkdir -p wintemp
	cp $(TOP)README       wintemp/README.TXT
	cp $(TOP)INSTALL.WIN32 wintemp/INSTALL.TXT
	cp $(TOP)COPYING      wintemp/COPYING.TXT
	cp $(TOP)COPYING.GPL  wintemp/COPYING.GPL.TXT
	cp $(TOP)COPYING.LGPL wintemp/COPYING.LGPL.TXT
	cp $(TOP)libreceiverd/README wintemp/README.receiver.TXT
	zip -j -m -r -l $(CHORALE)-win32.zip wintemp

SUBDIRS:= libdb libdbreceiver libimport libmediatree libreceiver \
	libdbsteam libutil choraled choralecd libupnpd libempeg libdbempeg \
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
