#
# Copyright 2002 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#ident  "$Id$"
#

MOD_DEPTH = ../..

NAME	= sun-nspr
RELEASE	= 2
VERSION	= `grep PR_VERSION \
	    $(MOD_DEPTH)/../dist/$(OBJDIR_NAME)/include/prinit.h \
		| sed -e 's/"$$//' -e 's/.*"//' -e 's/ .*//'`

include $(MOD_DEPTH)/config/config.mk

publish:
	$(MAKE) clean
	mkdir -p SOURCES SRPMS RPMS BUILD
	(cd $(MOD_DEPTH)/../dist/$(OBJDIR_NAME)/lib && tar cphf - .) \
	 | (mkdir -p opt/sun/private/lib && cd opt/sun/private/lib && tar xvfBp -)
	(cd $(MOD_DEPTH)/../dist/$(OBJDIR_NAME)/include && tar cphf - .) \
	 | (mkdir -p opt/sun/private/include/nspr && cd opt/sun/private/include/nspr && tar xvfBp -)
	tar czvf SOURCES/$(NAME)-$(VERSION).tar.gz opt
	echo "%define name $(NAME)" >$(NAME).spec
	echo "%define version $(VERSION)" >>$(NAME).spec
	echo "%define release $(RELEASE)" >>$(NAME).spec
	echo "%define buildroot `pwd`/$(NAME)-root" >>$(NAME).spec
	echo "%define _topdir `pwd`" >>$(NAME).spec
	cat ../linux/$(NAME).spec >>$(NAME).spec
	rpm -ba $(NAME).spec

clean:
	rm -rf SOURCES SRPMS RPMS BUILD
	rm -rf RPMS SRPMS opt
	rm -f $(NAME)-$(VERSION).tar.gz
