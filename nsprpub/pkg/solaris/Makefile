#
# Copyright 2002 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#ident	"$Id$"
#

MOD_DEPTH = ../..

%: %.ksh
	$(RM) $@
	cp $< $@
	chmod +x $@

ifeq ($(USE_64), 1)
DIRS = \
	SUNWnsprx
else
DIRS = \
	SUNWnspr
endif

include Makefile.com

awk_pkginfo: bld_awk_pkginfo
	./bld_awk_pkginfo -m $(MACH) -p "$(PRODUCT_VERSION)" -o $@ -v $(PRODUCT_VERSION)

all:: awk_pkginfo PROTO
publish: awk_pkginfo PROTO
	+$(LOOP_OVER_DIRS)

clean clobber::
	$(RM) awk_pkginfo bld_awk_pkginfo
	$(RM) -r $(ROOT)

PROTO: \
	$(ROOT) \
	$(ROOT)/usr \
	$(ROOT)/usr/lib \
	$(ROOT)/usr/lib/mps

$(ROOT) $(ROOT)/%:
	mkdir -p $@

$(ROOT)/usr/lib/mps:
	$(LN) -sf ../../../$(DIST)/lib $@

.PHONY: PROTO
