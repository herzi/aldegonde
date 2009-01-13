SUBDIRS = \
	docs \
	pixmaps \
	src

EXTRA_DIST = \
	@PACKAGE@.spec.in \
	@PACKAGE@.spec \
	configure.ac \
	autogen.sh \
	AUTHORS \
	TODO \
	README \
	INSTALL \
	NEWS \
	ChangeLog \
	COPYING

dist-hook:
	cp @PACKAGE@.spec $(distdir)

rpm: dist
	rpm -ta @PACKAGE@-@VERSION@.tar.gz
