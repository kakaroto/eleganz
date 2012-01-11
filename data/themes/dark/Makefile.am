
EDJE_CC = @edje_cc@
EDJE_FLAGS = -v -id $(top_srcdir)/data/themes/dark/images -fd $(top_srcdir)/data/themes/dark/fonts

EXTRA_DIST = \
dark.edc \
edc/main.edc \
edc/menu.edc \
edc/categories.edc \
edc/items.edc \
images/*.png \
fonts/*.ttf

filesdir = $(datadir)/themes/dark
files_DATA = dark.edj

dark.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/themes/dark/dark.edc \
	$(top_builddir)/data/themes/dark/dark.edj

clean-local:
	rm -f *.edj
