
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

bin_PROGRAMS = FLHOC

FLHOC_SOURCES = FLHOC.c \
	FLHOC_menu.h FLHOC_menu.c

FLHOC_LDADD = @EFL_LIBS@