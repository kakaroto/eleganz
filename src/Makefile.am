
ERROR_CFLAGS = \
	@EFL_CFLAGS@ \
	-fno-strict-aliasing \
	-Wall \
	-Wextra \
	-Wundef \
	-Wnested-externs \
	-Wwrite-strings \
	-Wpointer-arith \
	-Wbad-function-cast \
	-Wmissing-declarations \
	-Wmissing-prototypes \
	-Wstrict-prototypes \
	-Wredundant-decls \
	-Wno-unused-parameter \
	-Wno-missing-field-initializers

AM_CFLAGS = \
	$(ERROR_CFLAGS) \
	-Wno-redundant-decls \
	-g -O0

bin_PROGRAMS = Eleganz

Eleganz_SOURCES = Eleganz.c \
	menu.h menu.c \
	common.h types.h
if ENABLE_COBRA_ODE
Eleganz_SOURCES += manager.h manager.c
else
Eleganz_SOURCES += tools.c tools.h \
	pkg.c pkg.h \
	paged_file.c paged_file.h \
	keys.c keys.h \
	aes.c aes.h \
	sha1.c sha1.h \
	common.h types.h
endif

Eleganz_CFLAGS = $(AM_CFLAGS) @ELEGANZ_CFLAGS@
Eleganz_LDADD = @EFL_LIBS@
