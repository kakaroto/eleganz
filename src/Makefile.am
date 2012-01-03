
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
	$(ERROR_CFLAGS)

bin_PROGRAMS = homebrew_center

homebrew_center_SOURCES =  types.h \
		tools.c \
		tools.h \
		keys.c \
		keys.h \
		aes.c \
		aes.h \
		sha1.c \
		sha1.h \
		homebrew_center.c \
		pkg.c \
		paged_file.c

homebrew_center_LDADD = @EFL_LIBS@