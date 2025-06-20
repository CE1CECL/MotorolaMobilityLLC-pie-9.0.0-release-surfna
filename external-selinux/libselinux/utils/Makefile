# Installation directories.
PREFIX ?= $(DESTDIR)/usr
LIBDIR ?= $(PREFIX)/lib
SBINDIR ?= $(PREFIX)/sbin
INCLUDEDIR ?= $(PREFIX)/include

OS ?= $(shell uname)

ifeq ($(shell $(CC) -v 2>&1 | grep "clang"),)
COMPILER ?= gcc
else
COMPILER ?= clang
endif

ifeq ($(COMPILER), gcc)
EXTRA_CFLAGS = -fipa-pure-const -Wpacked-bitfield-compat -Wsync-nand -Wcoverage-mismatch \
	-Wcpp -Wformat-contains-nul -Wnormalized=nfc -Wsuggest-attribute=const \
	-Wsuggest-attribute=noreturn -Wsuggest-attribute=pure -Wtrampolines -Wjump-misses-init \
	-Wno-suggest-attribute=pure -Wno-suggest-attribute=const
endif

MAX_STACK_SIZE=8192
CFLAGS ?= -O -Wall -W -Wundef -Wformat-y2k -Wformat-security -Winit-self -Wmissing-include-dirs \
          -Wunused -Wunknown-pragmas -Wstrict-aliasing -Wshadow -Wpointer-arith \
          -Wbad-function-cast -Wcast-align -Wwrite-strings -Waggregate-return \
          -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes \
          -Wmissing-declarations -Wmissing-noreturn -Wmissing-format-attribute \
          -Wredundant-decls -Wnested-externs -Winline -Winvalid-pch -Wvolatile-register-var \
          -Wdisabled-optimization -Wbuiltin-macro-redefined \
          -Wattributes -Wmultichar \
          -Wdeprecated-declarations -Wdiv-by-zero -Wdouble-promotion -Wendif-labels -Wextra \
          -Wformat-extra-args -Wformat-zero-length -Wformat=2 -Wmultichar \
          -Woverflow -Wpointer-to-int-cast -Wpragmas \
          -Wno-missing-field-initializers -Wno-sign-compare \
          -Wno-format-nonliteral -Wframe-larger-than=$(MAX_STACK_SIZE) -Wp,-D_FORTIFY_SOURCE \
          -fstack-protector-all --param=ssp-buffer-size=4 -fexceptions \
          -fasynchronous-unwind-tables -fdiagnostics-show-option -funit-at-a-time \
           -Wno-aggregate-return -Wno-redundant-decls \
          $(EXTRA_CFLAGS)

LD_SONAME_FLAGS=-soname,$(LIBSO),-z,defs,-z,relro

ifeq ($(OS), Darwin)
override CFLAGS += -I/opt/local/include -I../../libsepol/include
override LDFLAGS += -L../../libsepol/src -undefined dynamic_lookup
endif

override CFLAGS += -I../include -D_GNU_SOURCE $(DISABLE_FLAGS) $(PCRE_CFLAGS)
override LDFLAGS += -L../src
override LDLIBS += -lselinux
PCRE_LDLIBS ?= -lpcre

ifeq ($(ANDROID_HOST),y)
TARGETS=sefcontext_compile
else
TARGETS=$(patsubst %.c,%,$(sort $(wildcard *.c)))
endif

sefcontext_compile: LDLIBS += $(PCRE_LDLIBS) ../src/libselinux.a -lsepol

sefcontext_compile: sefcontext_compile.o ../src/regex.o

all: $(TARGETS)

install: all
	-mkdir -p $(SBINDIR)
	install -m 755 $(TARGETS) $(SBINDIR)

clean:
	rm -f $(TARGETS) *.o *~

distclean: clean

indent:
	../../scripts/Lindent $(wildcard *.[ch])

relabel:

