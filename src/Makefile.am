
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
	pkg.c pkg.h \
	paged_file.c paged_file.h \
	keys.c keys.h \
	tools.c tools.h \
	aes.c aes.h \
	sha1.c sha1.h \
	common.h types.h

Eleganz_LDADD = @EFL_LIBS@
