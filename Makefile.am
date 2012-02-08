include Makefile.include
include Makefile.tests
include data/Makefile.include
include redis-glib/Makefile.include
include tests/Makefile.include

ACLOCAL_AMFLAGS = -I build/m4 ${ACLOCAL_FLAGS}
DISTCHECK_CONFIGURE_FLAGS = --enable-maintainer-flags --enable-gtk-doc --enable-introspection

redisdocdir = ${prefix}/doc/redis-glib-1.0
redisdoc_DATA =
redisdoc_DATA += README
redisdoc_DATA += COPYING
redisdoc_DATA += AUTHORS
redisdoc_DATA += ChangeLog
redisdoc_DATA += INSTALL
redisdoc_DATA += NEWS

EXTRA_DIST += $(redisdoc_DATA)

dist-hook:
	@if test -d "$(srcdir)/.git"; then                              \
          (cd "$(srcdir)" &&                                            \
           $(top_srcdir)/missing --run git log --stat ) > ChangeLog.tmp \
           && mv -f ChangeLog.tmp $(top_distdir)/ChangeLog              \
           || (rm -f ChangeLog.tmp;                                     \
               echo Failed to generate ChangeLog >&2);                  \
	else                                                            \
	  echo A git checkout is required to generate a ChangeLog >&2;  \
	fi

uninstall-local:
	-rm -r $(redisdocdir)
