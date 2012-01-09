
EDJE_CC = @edje_cc@
EDJE_FLAGS = -v -id $(top_srcdir)/data/themes/light/images -fd $(top_srcdir)/data/themes/light/fonts

EXTRA_DIST = \
light.edc

filesdir = $(datadir)/themes/light
files_DATA = light.edj

light.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/themes/light/light.edc \
	$(top_builddir)/data/themes/light/light.edj

clean-local:
	rm -f *.edj
