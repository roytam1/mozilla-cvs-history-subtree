#! /bin/bash

HOST=$(hostname | cut -d. -f1)
export HOST

# if your machine's IP isn't registered in DNS,
# you must set appropriate environment variables
# that can be resolved locally.
# For example, if localhost.localdomain works on your system, set:
#HOST=localhost
#DOMSUF=localdomain
#export DOMSUF

ARCH=$(uname -s)

ulimit -c unlimited 2> /dev/null

CVSROOT=":pserver:anonymous@cvs-mirror.mozilla.org:/cvsroot"

CVS_TRUNK="mozilla/nsprpub
mozilla/dbm
mozilla/security/dbm
mozilla/security/coreconf
mozilla/security/nss
mozilla/security/jss
-r:NSS_3_11_1_RTM:mozilla/security/nss/lib/freebl/ecl/ecl-curve.h"

CVS_STABLE="-r:HEAD:mozilla/nsprpub
-r:NSS_3_13_4_BRANCH:mozilla/dbm
-r:NSS_3_13_4_BRANCH:mozilla/security/dbm
-r:NSS_3_13_4_BRANCH:mozilla/security/coreconf
-r:NSS_3_13_4_BRANCH:mozilla/security/nss
-r:HEAD:mozilla/security/jss
-r:NSS_3_11_1_RTM:mozilla/security/nss/lib/freebl/ecl/ecl-curve.h"

export NSS_ENABLE_ECC=1
export NSS_ECC_MORE_THAN_SUITE_B=1
export NSPR_LOG_MODULES="pkix:1"

#enable if you have PKITS data
#export PKITS_DATA=$HOME/pkits/data/

NSS_BUILD_TARGET="clean nss_build_all"
JSS_BUILD_TARGET="clean all"

CVS=cvs
MAKE=gmake
AWK=awk
PATCH=patch

if [ "${ARCH}" = "SunOS" ]; then
    AWK=nawk
    PATCH=gpatch
    ARCH=SunOS/$(uname -p)
fi

if [ "${ARCH}" = "Linux" -a -f /etc/system-release ]; then
   VERSION=`sed -e 's; release ;;' -e 's; (.*)$;;' -e 's;Red Hat Enterprise Linux Server;RHEL;' -e 's;Red Hat Enterprise Linux Workstation;RHEL;' /etc/system-release`
   ARCH=Linux/${VERSION}
   echo ${ARCH}
fi

PROCESSOR=$(uname -p)
if [ "${PROCESSOR}" = "ppc64" ]; then
    ARCH="${ARCH}/ppc64"
fi
if [ "${PROCESSOR}" = "powerpc" ]; then
    ARCH="${ARCH}/ppc"
fi

MAIL=mail
TB_SERVER=tinderbox-daemon@tinderbox.mozilla.org
#For more advanced SMTP configurations, consider to use:
#- heirloom-mailx / nail (unix like platforms)
#  example config: MAIL="mailx -S smtp=smtp://smtp.server.tld:25 -r sender@address.tld"
#  or with SMTP/STARTTLS and authentication:
#  MAIL="mailx -S smtp-use-starttls -S smtp=smtp://smpt.server.tld:25 \
#        -S smtp-auth-user=your-smtp-login -S smtp-auth-password=your-smtp-password \
#        -S nss-config-dir=$HOME/nssdb -r sender@address.tld"
#- blat (windows)
#  example, use "blat -install ..." to configure the default and set
#  MAIL="blat - -to tinderbox-daemon@tinderbox.mozilla.org"
#  TB_SERVER=

# how many cycles to keep locally (use at least 1)
CYCLE_MAX=5
# run at most one build per this amount of minutes
CYCLE_TIME=60

PORT_32_DBG=8111
PORT_32_OPT=8222
PORT_64_DBG=8333
PORT_64_OPT=8444

#### SOME DEFAULTS, CAN CHANGE LATER ####

run_bits="32 64"
run_opt="DBG OPT"
run_branches="stable trunk"

### CONFIG.SH CONTAINS CONFIGURATIONS OF ALL MACHINES ###

. config.sh

RUN_BITS="${RUN_BITS:-$run_bits}"
RUN_OPT="${RUN_OPT:-$run_opt}"
RUN_BRANCHES="${RUN_BRANCHES:-$run_branches}"
