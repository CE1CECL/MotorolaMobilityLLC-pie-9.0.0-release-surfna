OPT_SUBDIRS ?= dbus gui mcstrans python restorecond sandbox semodule-utils
SUBDIRS=libsepol libselinux libsemanage checkpolicy secilc policycoreutils $(OPT_SUBDIRS)
PYSUBDIRS=libselinux libsemanage
DISTCLEANSUBDIRS=libselinux libsemanage

ifeq ($(DEBUG),1)
	export CFLAGS = -g3 -O0 -gdwarf-2 -fno-strict-aliasing -Wall -Wshadow 
	export LDFLAGS = -g
else
	export CFLAGS ?= -O2  -Wall -Wextra \
		-Wmissing-format-attribute \
		-Wmissing-noreturn \
		-Wpointer-arith \
		-Wshadow \
		-Wstrict-prototypes \
		-Wundef \
		-Wunused \
		-Wwrite-strings
endif

ifneq ($(DESTDIR),)
	CFLAGS += -I$(DESTDIR)/usr/include
	LDFLAGS += -L$(DESTDIR)/usr/lib
	export CFLAGS
	export LDFLAGS
endif

all install relabel clean test indent:
	@for subdir in $(SUBDIRS); do \
		(cd $$subdir && $(MAKE) $@) || exit 1; \
	done

install-pywrap install-rubywrap swigify:
	@for subdir in $(PYSUBDIRS); do \
		(cd $$subdir && $(MAKE) $@) || exit 1; \
	done

distclean:
	@for subdir in $(DISTCLEANSUBDIRS); do \
		(cd $$subdir && $(MAKE) $@) || exit 1; \
	done
