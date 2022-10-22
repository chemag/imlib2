AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src data
if BUILD_DOC
SUBDIRS += doc
endif
if BUILD_TEST
SUBDIRS += test
endif

CLEANFILES = $(PACKAGE).spec

MAINTAINERCLEANFILES = aclocal.m4 compile \
		       config.guess config.h.in config.sub configure \
		       depcomp install-sh ltmain.sh missing \
		       Makefile.in */Makefile.in */*/Makefile.in */*/*/Makefile.in \
		       imlib2_docs.tar.gz

EXTRA_DIST = AUTHORS COPYING COPYING-PLAIN \
	     README.in README \
	     $(PACKAGE).pc.in \
	     $(PACKAGE).spec.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(PACKAGE).pc

dist-hook: $(top_builddir)/$(PACKAGE).spec
	if [ -s $(distdir)/test/Makefile.am ]; then \
	  rm -f $(distdir)/test/*.*; \
	  touch $(distdir)/test/Makefile.am; \
	  cd $(top_srcdir) && $(AUTOMAKE) --foreign $(abs_builddir)/$(distdir)/test/Makefile; \
	fi

if SRC_IS_GIT
RPM_RELEASE=`echo -n "1.%(date '+%y%m%d').git"; git rev-parse --short=8 HEAD`
else
RPM_RELEASE=`echo -n "1.%(date '+%y%m%d')"`
endif

$(top_builddir)/$(PACKAGE).spec: FORCE
	sed "s/\@PACKAGE\@/@PACKAGE@/;s/\@VERSION\@/@VERSION@/;s/\@RPM_RELEASE\@/$(RPM_RELEASE)/" \
		$(top_srcdir)/$(PACKAGE).spec.in > $@

.PHONY: doc src test
doc src test:
	$(MAKE) -C $@
test: src

FORCE:
