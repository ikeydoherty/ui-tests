AM_CFLAGS = \
	-fstack-protector -Wall -pedantic			\
	-Wstrict-prototypes -Wundef -fno-common 		\
	-Werror-implicit-function-declaration 			\
	-Wformat -Wformat-security -Werror=format-security 	\
	-Wconversion -Wunused-variable -Wunreachable-code 	\
	-Wall -W -D_FORTIFY_SOURCE=2 -std=c11

noinst_PROGRAMS = \
	popover-test

popover_test_SOURCES = \
	src/popover/popover.h \
	src/popover/popover.c \
	src/popover/main.c

popover_test_CFLAGS = \
	$(GTK_CFLAGS) \
	$(AM_CFLAGS)

popover_test_LDADD = \
	$(GTK_LIBS)

