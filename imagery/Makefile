TOP?=../
DIR:=imagery/
SRCS:=

imagery:=yes

all-imagery: .force-header-generation

include $(TOP)Make.common

CHORALE_PNGS:= icon16.png icon16s.png icon32.png icon32s.png \
	icon48.png \
	icon48s.png \
	icon60.png \
	320x480.png \
	noart32.png \
	noart48.png \
	noart100.png \
	cd.png \
	cddrive.png \
	close.png \
	dir.png \
	empeg.png \
	ffwd.png \
	file.png \
	film.png \
	folder.png \
	image.png \
	minimise.png \
	network.png \
	norepeat.png \
	output.png \
	pause.png \
	play.png \
	portable.png \
	portcullis.png \
	query.png \
	repeatall.png \
	repeatone.png \
	resize.png \
	rew.png \
	settings.png \
	shuffle.png \
	stop.png \
	tv.png \
	vibez.png

CHORALE_PNGS:=$(CHORALE_PNGS:%.png=$(TOP)$(DIR)%.png)
CHORALE_XPMS:=$(CHORALE_PNGS:%.png=%.xpm)

DO_INKSCAPE:=$(INKSCAPE) -z >/dev/null -b white

ifeq ($(HAVE_INKSCAPE)$(HAVE_CONVERT),yesyes)

$(TOP)$(DIR)icon16.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(INKSCAPE) -z -e $@ -a 60:660:150:750 -w16 -h16 $< >/dev/null
$(TOP)$(DIR)icon32.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(INKSCAPE) -z -e $@ -a 60:660:150:750 -w32 -h32 $< >/dev/null
$(TOP)$(DIR)icon48.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(INKSCAPE) -z -e $@ -a 60:660:150:750 -w48 -h48 $< >/dev/null
$(TOP)$(DIR)icon60.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(INKSCAPE) -z -e $@ -a 60:660:150:750 -w60 -h60 $< >/dev/null
$(TOP)$(DIR)320x480.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(INKSCAPE) -z -e $@ -a 170:770:260:900 -w320 -h480 -y 255 $< >/dev/null

$(TOP)$(DIR)icon16s.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(INKSCAPE) -z -e $@ -a 170:660:260:750 -w16 -h16 $< >/dev/null
$(TOP)$(DIR)icon32s.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(INKSCAPE) -z -e $@ -a 170:660:260:750 -w32 -h32 $< >/dev/null
$(TOP)$(DIR)icon48s.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(INKSCAPE) -z -e $@ -a 170:660:260:750 -w48 -h48 $< >/dev/null

$(TOP)$(DIR)cd.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 295:450:395:550 -w32 -h32 $<
$(TOP)$(DIR)cddrive.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 775:450:875:550 -w32 -h32 $<
$(TOP)$(DIR)portcullis.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 775:570:875:670 -w32 -h32 $<
$(TOP)$(DIR)empeg.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 535:450:635:550 -w32 -h32 $<
$(TOP)$(DIR)folder.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 415:450:515:550 -w32 -h32 $<
$(TOP)$(DIR)network.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 425:580:505:660 -w32 -h32 $<
$(TOP)$(DIR)output.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 295:570:395:670 -w32 -h32 $<
$(TOP)$(DIR)vibez.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 175:450:275:550 -w32 -h32 $<
$(TOP)$(DIR)portable.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 55:450:155:550 -w32 -h32 $<
$(TOP)$(DIR)tv.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 655:570:755:670 -w32 -h32 $<
$(TOP)$(DIR)noart32.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 535:570:635:670 -w32 -h32 $<
$(TOP)$(DIR)noart48.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 535:570:635:670 -w48 -h48 $<
$(TOP)$(DIR)noart100.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 535:570:635:670 -w100 -h100 $<

$(TOP)$(DIR)resize.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 655:450:755:550 -w32 -h32 $<
$(TOP)$(DIR)close.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 490:320:540:370 -w16 -h16 $<
$(TOP)$(DIR)minimise.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 430:320:480:370 -w16 -h16 $<

$(TOP)$(DIR)dir.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 10:320:60:370 -w12 -h12 $<
$(TOP)$(DIR)ffwd.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 370:320:420:370 -w16 -h16 $<
$(TOP)$(DIR)file.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 70:320:120:370 -w12 -h12 $<
$(TOP)$(DIR)query.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 70:260:120:310 -w12 -h12 $<
$(TOP)$(DIR)pause.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 130:320:180:370 -w16 -h16 $<
$(TOP)$(DIR)image.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 130:260:180:310 -w12 -h12 $<
$(TOP)$(DIR)play.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 190:320:240:370 -w16 -h16 $<
$(TOP)$(DIR)film.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 190:260:240:310 -w12 -h12 $<
$(TOP)$(DIR)rew.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 310:320:360:370 -w16 -h16 $<
$(TOP)$(DIR)stop.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 250:320:300:370 -w16 -h16 $<
$(TOP)$(DIR)settings.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 550:320:600:370 -w16 -h16 $<
$(TOP)$(DIR)shuffle.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 610:320:660:370 -w16 -h16 $<
$(TOP)$(DIR)repeatall.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 670:320:720:370 -w16 -h16 $<
$(TOP)$(DIR)norepeat.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 730:320:780:370 -w16 -h16 $<
$(TOP)$(DIR)repeatone.png: $(TOP)$(DIR)chorale.svg
	$(GEN_PRE)$(DO_INKSCAPE) -e $@ -a 790:320:840:370 -w16 -h16 $<

$(TOP)$(DIR)%.xpm: $(TOP)$(DIR)%.png
	$(GEN_PRE)$(CONVERT) -transparent white $< $*.0.xpm
	@sed -e "s/static.*\[]/static const char *const $*_xpm[]/" < $*.0.xpm > $*.1.xpm
	@mv $*.1.xpm $@
	@rm $*.0.xpm

$(TOP)$(DIR)icon16_16.gif: $(TOP)$(DIR)icon16.png
	$(GEN_PRE)$(CONVERT) -colors 16 $^ $@
$(TOP)$(DIR)icon16_256.gif: $(TOP)$(DIR)icon16.png
	$(GEN_PRE)$(CONVERT) -map netscape: $^ $@
$(TOP)$(DIR)icon32_16.gif: $(TOP)$(DIR)icon32.png
	$(GEN_PRE)$(CONVERT) -colors 16 $^ $@
$(TOP)$(DIR)icon32_256.gif: $(TOP)$(DIR)icon32.png
	$(GEN_PRE)$(CONVERT) -map netscape: $^ $@
$(TOP)$(DIR)icon48_16.gif: $(TOP)$(DIR)icon48.png
	$(GEN_PRE)$(CONVERT) -colors 16 $^ $@
$(TOP)$(DIR)icon48_256.gif: $(TOP)$(DIR)icon48.png
	$(GEN_PRE)$(CONVERT) -map netscape: $^ $@

$(TOP)$(DIR)icon.ico: \
	$(TOP)$(DIR)icon16.png $(TOP)$(DIR)icon32.png $(TOP)$(DIR)icon48.png \
	$(TOP)$(DIR)icon16_16.gif $(TOP)$(DIR)icon16_256.gif \
	$(TOP)$(DIR)icon32_16.gif $(TOP)$(DIR)icon32_256.gif \
	$(TOP)$(DIR)icon48_16.gif $(TOP)$(DIR)icon48_256.gif
	$(GEN_PRE)$(CONVERT) $^ $@

else

$(CHORALE_XPMS) $(TOP)$(DIR)icon.ico: $(TOP)$(DIR)chorale.svg
	@echo "** chorale.svg modified but Inkscape and/or convert not found"
	@echo "   Install them and re-run \"configure\" to enact changes"
	@echo

endif

.force-header-generation: $(CHORALE_XPMS) $(TOP)$(DIR)icon.ico

CLEANS += $(CHORALE_PNGS) \
	$(TOP)$(DIR)icon*_*.gif

MAINTAINERCLEANS += $(CHORALE_XPMS) $(TOP)$(DIR)icon.ico
