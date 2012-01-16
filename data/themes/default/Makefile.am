
EDJE_CC = @edje_cc@
EDJE_FLAGS = -v -id $(top_srcdir)/data/themes/default/images -fd $(top_srcdir)/data/themes/default/fonts

EXTRA_DIST = \
default.edc \
edc/*.edc \
images/*.png \
fonts/*.ttf

filesdir = $(datadir)/themes/default
files_DATA = default.edj

default.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/themes/default/default.edc \
	$(top_builddir)/data/themes/default/default.edj

clean-local:
	rm -f *.edj
