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
	SUNWprx \
	SUNWprdx
else
DIRS = \
	SUNWpr \
	SUNWprd
endif

PROTO = \
	$(ROOT) \
	$(ROOT)/usr \
	$(ROOT)/usr/lib \
	$(ROOT)/usr/lib/mps \
	$(ROOT)/usr/lib/mps/secv1 \
	$(ROOT)/usr/include \
	$(ROOT)/usr/include/mps 

ifdef USE_64
PROTO += $(ROOT)/usr/lib/mps/sparcv9 \
	$(ROOT)/usr/include/mps/sparcv9
endif

include Makefile.com

awk_pkginfo: bld_awk_pkginfo
	./bld_awk_pkginfo -m $(MACH) -p "$(PRODUCT_VERSION)" -o $@ -v $(PRODUCT_VERSION)

all:: awk_pkginfo $(PROTO)
publish: awk_pkginfo $(PROTO)
	+$(LOOP_OVER_DIRS)

clean clobber::
	$(RM) awk_pkginfo bld_awk_pkginfo
	$(RM) -r $(ROOT)

$(ROOT) $(ROOT)/%:
	mkdir -p $@

ifdef USE_64
$(ROOT)/usr/lib/mps/sparcv9: $(ROOT)/usr/lib
	$(LN) -sf ../../../../$(DIST)/lib $@
$(ROOT)/usr/include/mps/sparcv9: $(ROOT)/usr/include
	$(LN) -sf ../../../../$(DIST)/include $@
else
$(ROOT)/usr/lib/mps/secv1: $(ROOT)/usr/lib
	$(LN) -sf ../../../../$(DIST)/lib $@
$(ROOT)/usr/include/mps: $(ROOT)/usr/include
	$(LN) -sf ../../../$(DIST)/include $@
endif
