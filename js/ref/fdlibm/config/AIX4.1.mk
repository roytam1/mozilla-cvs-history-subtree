#
# Config stuff for AIX
#

CC = xlC_r
CCC = xlC_r

RANLIB = ranlib

#.c.o:
#	$(CC) -c -MD $*.d $(CFLAGS) $<
ARCH := aix
CPU_ARCH = rs6000
GFX_ARCH = x
INLINES = js_compare_and_swap:js_fast_lock1:js_fast_unlock1:js_lock_get_slot:js_lock_set_slot:js_lock_scope1

OS_CFLAGS = -qarch=com -qinline+$(INLINES) -DXP_UNIX -DAIX -DAIXV3 -DSYSV
OS_LIBS = -lbsd -lsvld -lm
#-lpthreads -lc_r
