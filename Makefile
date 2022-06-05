TOP?=

all:

include $(TOP)Make.common

ifeq ($(choralecd),)
include $(TOP)choralecd/Makefile
endif
ifeq ($(choraleutil),)
include $(TOP)choraleutil/Makefile
endif
ifeq ($(choraled),)
include $(TOP)choraled/Makefile
endif

# Autoconf stuff for remaking 'configure' and 'Make.config'

$(TOP)configure: $(TOP)configure.ac $(TOP)aclocal.m4
	(cd ./$(TOP) && autoconf)

configure:=configure

# autoheader might not change config.h.in, so touch a stamp file.
$(TOP)config.h.in: $(TOP)stamp-h.in

$(TOP)stamp-h.in: $(TOP)configure.ac $(TOP)aclocal.m4
	(cd ./$(TOP) \
		&& autoheader \
		&& echo timestamp > stamp-h.in)

config.$(TARGET).h: stamp-h
stamp-h: $(TOP)config.h.in config.status
	./config.status

Make.config.$(TARGET) $(LIBTOOL): $(TOP)Make.config.in config.status
	./config.status

config.status: $(TOP)configure $(TOP)stamp-h.in
	[ -x config.status ] || ./configure
	./config.status --recheck

$(TOP)aclocal.m4: $(TOP)configure.ac
	(cd ./$(TOP) && libtoolize -c)
	(cd ./$(TOP) && aclocal -I autotools)

doc:
	doxygen Doxyfile

.PHONY: distclean clean release doc

distclean: clean
	rm -rf config.*.h Make.config.*-* stamp-h config.log config.status \
		autom4te.cache libtool.* libtool
	find . -name diff.txt -exec rm -f \{} \;
	find . -name 'map*.txt' -exec rm -f \{} \;
	find . -name '*.dep' -exec rm -f \{} \;
	find . -name '*.o' -exec rm -f \{} \;
	find . -name '*~' -exec rm -f \{} \;

maintainerclean: distclean
	rm -f $(MAINTAINERCLEANS)

install: $(choraled)
	$(INSTALL) -d $(datadir)/chorale/upnp
	$(INSTALL) libupnp/AVTransport.xml2 $(datadir)/chorale/upnp/AVTransport.xml
	$(INSTALL) libupnp/RenderingControl.xml2 $(datadir)/chorale/upnp/RenderingControl.xml
	$(INSTALL) libupnp/ContentDirectory.xml2 $(datadir)/chorale/upnp/ContentDirectory.xml
	$(INSTALL) libupnp/ConnectionManager.xml2 $(datadir)/chorale/upnp/ConnectionManager.xml
	$(INSTALL) libupnp/OpticalDrive.xml2 $(datadir)/chorale/upnp/OpticalDrive.xml
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
	$(INSTALL) -s choraleutil/protocoltool $(bindir)
	$(INSTALL) -d $(mandir)/man1
	$(INSTALL) -m644 choraleutil/protocoltool.1 $(mandir)/man1

install-choralecd: $(choralecd)
	$(INSTALL) -d $(bindir)
	$(INSTALL) -s $(choralecd) $(bindir)
	$(INSTALL) -s $(tageditor) $(bindir)

CHORALE:=chorale-$(CHORALE_VERSION)

release: distclean
	rm -f chorale*.tar.bz2
	tar cf $(CHORALE).tar --transform=s,./,$(CHORALE)/, \
		--exclude=$(CHORALE).tar --exclude=CVS --exclude=doc \
		--exclude=.libs --exclude=.cvsignore --exclude=.svn \
		--exclude=.git --exclude='*.flac' --exclude='*.mp3' \
		 .
	bzip2 -9 $(CHORALE).tar

# "check" is the GNU Coding Standards word for "tests"
check: tests headercheck

winrelease: all
	rm -f $(CHORALE)-win32.zip
	rm -rf wintemp
	mkdir -p wintemp/web/upnp wintemp/web/layout
	cp $(TOP)choraled/choralesvc.exe wintemp
	cp $(TOP)choraleutil/protocoltool wintemp/protocoltool.exe
	cp /usr/i586-mingw32/bin/mingwm10.dll wintemp
	cp $(TOP)libupnp/AVTransport2.xml      wintemp/web/upnp/AVTransport.xml
	cp $(TOP)libupnp/ContentDirectory3.xml wintemp/web/upnp/ContentDirectory.xml
	cp $(TOP)libupnp/OpticalDrive.xml      wintemp/web/upnp/OpticalDrive.xml
	cp $(TOP)libupnp/RenderingControl2.xml wintemp/web/upnp/RenderingControl.xml
	cp $(TOP)imagery/icon32.png            wintemp/web/layout/icon.png
	cp $(TOP)imagery/icon32s.png           wintemp/web/layout/iconsel.png
	cp $(TOP)imagery/icon.ico              wintemp/web/layout/icon.ico
	cd wintemp && zip -m -r ../$(CHORALE)-win32.zip .
	mkdir -p wintemp
	cp $(TOP)README       wintemp/README.TXT
	cp $(TOP)INSTALL.WIN32 wintemp/INSTALL.TXT
	cp $(TOP)COPYING      wintemp/COPYING.TXT
	cp $(TOP)COPYING.GPL  wintemp/COPYING.GPL.TXT
	cp $(TOP)COPYING.LGPL wintemp/COPYING.LGPL.TXT
	cp $(TOP)libreceiverd/README wintemp/README.receiver.TXT
	GROFF_NO_SGR=please man $(TOP)choraleutil/protocoltool.1 \
		| col -b > wintemp/README.protocoltool.TXT
	cd wintemp && zip -m -r -l ../$(CHORALE)-win32.zip .

SUBDIRS:= \
	choralecd \
	choraled \
	choraleutil \
	libdav \
	libdb \
	libdbempeg \
	libdblocal \
	libdbmerge \
	libdbreceiver \
	libdbsteam \
	libdbupnp \
	libempeg \
	libimport \
	libisam \
	libmediadb \
	libmediatree \
	liboutput \
	libreceiver \
	libreceiverd \
	libtv \
	libui \
	libuiqt \
	libupnp \
	libupnpd \
	libutil

libdeps.dot: Makefile
	echo "digraph G {" > $@
	for j in $(SUBDIRS) ; do \
		for i in `find $$j -maxdepth 2 -name '*.h' -o -name '*.cpp'` ; do \
			grep -H "^#include.*\".*/" $$i \
				| fgrep -v all- \
				| grep -v ".*include.*$$j" \
				| fgrep -v ".." \
				| sed -e 's,/[^\"]*\", -> ,'  -e s,/[^/]*\",, ; \
		done \
	done | sort | uniq >> $@
	echo "}" >> $@

# Dependencies among Chorale headers
headerdeps.dot: Makefile
	echo "digraph G {" > $@
	for i in `find . -maxdepth 2 -name '*.h'` ; do \
		g++ -MP -M $$i -I. 2>/dev/null $(QT_CXXFLAGS) \
			$(TAGLIB_CXXFLAGS) $(HAL_CXXFLAGS) \
			$(GSTREAMER_CXXFLAGS) $(CXXFLAGS) $(CAIRO_CXXFLAGS) \
			| grep '^[a-z].*:$$' \
			| sed -e 's,^,"'$$i'" -> ",' -e 's,:$$,",' \
				-e 's,^"./,",' ; \
	done >> $@
	echo "}" >> $@

# Dependencies among all Chorale files
filedeps.dot:
	echo "digraph G {" > $@
	for i in `find . -maxdepth 2 -name '*.h' -o -name '*.cpp'` ; do \
		g++ -MP -M $$i -I.  $(QT_CXXFLAGS) \
			$(TAGLIB_CXXFLAGS) $(HAL_CXXFLAGS) \
			$(GSTREAMER_CXXFLAGS) $(CXXFLAGS) $(CAIRO_CXXFLAGS) \
			| grep '^[a-z].*:$$' \
			| sed -e 's,^,"'$$i'" -> ",' -e 's,:$$,",' \
				-e 's,^"./,",' ; \
	done >> $@
	echo "}" >> $@
	scripts/fan $@

# Headers such that whenever A.h is included, B.h is too
coheaders.dot: filedeps.dot Makefile scripts/coheaders.gvpr
	gvpr -f scripts/coheaders.gvpr < filedeps.dot > coheaders.dot

coheaders.svg: coheaders.dot
	tred coheaders.dot | fdp -Tsvg -o $@

# Dependencies among "components" (Lakos's definition)
# Allow for foo, foo_posix, and foo_win32 to be one component not three.
# Allow for bar and bar_internal to be one component not two.
# Allow for Baz, Baz_client, and Baz_server to be one component not three
compdeps.dot: filedeps.dot Makefile
	sed -e 's/[.]cpp//g' -e 's/[.]h//g' -e 's/_posix//g'  -e 's/_linux//g' -e 's/_win32//g' \
	    -e 's,libupnp/\([A-Z][a-zA-Z0-9]*\)_client,libupnp/\1,g' \
	    -e 's,libupnp/\([A-Z][a-zA-Z0-9]*\)_server,libupnp/\1,g' \
	    -e 's/_internal//g' < filedeps.dot > compdeps.dot

compdeps2.dot: compdeps.dot
	echo "strict digraph {" > compdeps2.dot
	for i in $(SUBDIRS) ; do \
		echo "subgraph cluster_$$i {" ; \
		echo "label=$$i" ; \
		grep "$$i/.*->.*$$i/" compdeps.dot ;  \
		echo "}" ; \
	done >> compdeps2.dot
	sed -e 's, -> ,,' \
		-e 's,"\([a-z]*\)/\([a-zA-Z_0-9]*\)","\1/\2" [label="\2"]\n,g' \
		< compdeps.dot | grep label | sort | uniq >> compdeps2.dot
	echo "}" >> compdeps2.dot

cycles.png: compdeps.dot
	echo "strict digraph {" > cycles.dot
	sccmap compdeps.dot \
		| sed -e 's/digraph cluster_/subgraph sg/' \
		      -e 's/scc_map/cluster/' \
		| awk -F '[; \t]' '{ if ($$2 != $$4 || $$2 == "") print }' \
		| grep -v cluster >> cycles.dot
	circo -Tsvg -Nfontname="Luxi Sans" -Gfontnames=gd -o compdeps.svg cycles.dot
	inkscape -z -b white -y 1.0 -e $@ compdeps.svg -d60 > /dev/null

# All dependencies including on system headers
alldeps.dot: Makefile
	echo "digraph G {" > $@
	for i in `find . -maxdepth 2 -name '*.h' -o -name '*.cpp'` ; do \
		g++ -MP -M $$i -I. $(QT_CXXFLAGS) \
			$(TAGLIB_CXXFLAGS) $(HAL_CXXFLAGS) \
			$(GSTREAMER_CXXFLAGS) $(CXXFLAGS) $(CAIRO_CXXFLAGS) \
			| grep '^[a-z/].*:$$' \
			| sed -e 's,:$$,,' \
			| sed -e 's,^,"'$$i'" -> ",' -e 's,$$,",' \
				-e 's,^"./,",' ; \
	done >> $@
	echo "}" >> $@
	scripts/fan $@

%.gif: %.dot
	tred $< | dot -Tgif -o $@

%.png: %.dot
	tred $< | dot -Tsvg -Nfontname="Luxi Sans" -Gfontnames=gd -o $*.svg
	inkscape -z -b white -y 1.0 -e $@ $*.svg -d60 > /dev/null

CLEANS += $(TOP)filedeps.dot $(TOP)libdeps.dot

ALL_HEADERS:=$(shell ls $(TOP)*/*.h | fgrep -v all- )

#$(filter-out %/%.all-%, $(wildcard $(TOP)*/*.h))

CHECKED_HEADERS:=$(ALL_HEADERS:$(TOP)%=$(TOP).headercheck/%)

$(TOP).headercheck/%: $(TOP)% scripts/headercheck Makefile
	@scripts/headercheck $< $(CXXFLAGS) $(QT_CXXFLAGS)
	@mkdir -p `dirname $@`
	@touch $@

ALL_CPPS:=$(wildcard $(TOP)*/*.cpp)

CHECKED_CPPS:=$(ALL_CPPS:$(TOP)%=$(TOP).cppcheck/%)

$(TOP).cppcheck/%: $(TOP)% scripts/cppcheck Makefile
	@scripts/cppcheck $< $(CXXFLAGS) $(QT_CXXFLAGS)
	@mkdir -p `dirname $@`
	@touch $@

headercheck: $(CHECKED_HEADERS)

cppcheck: $(CHECKED_CPPS)

headercount:
	grep 'TOP.*:' */$(TARGETDIR)/*.lo.dep | wc -l

tests:
	@find $(TOP) -name '*.cpp.gcov' | $(TOP)scripts/topten
