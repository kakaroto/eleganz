EDJE_CC = @edje_cc@
THEME_NAME = default
THEME_DIR = $(top_srcdir)/data/themes/$(THEME_NAME)
EDJE_FLAGS = -v -id $(THEME_DIR)/images -fd $(THEME_DIR)/fonts

EXTRA_DIST = \
$(THEME_DIR)/$(THEME_NAME).edc \
$(THEME_DIR)/edc/*.edc \
$(THEME_DIR)/images/*.png \
$(THEME_DIR)/fonts/*.ttf

filesdir = $(datadir)/themes/$(THEME_NAME)
files_DATA = $(THEME_NAME).edj

$(THEME_NAME).edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(THEME_DIR)/$(THEME_NAME).edc \
	$(THEME_NAME).edj

clean-local:
	rm -f *.edj
