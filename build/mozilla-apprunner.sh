#!/bin/sh

moz_prefix=/usr/mozilla

MOZILLA_FIVE_HOME=/usr/lib/mozilla

LD_LIBRARY_PATH={LD_LIBRARY_PATH+"$LD_LIBRARY_PATH":}${MOZILLA_FIVE_HOME}

#PATH={PATH+"$PATH":}${moz_prefix}/bin

export LD_LIBRARY_PATH MOZILLA_FIVE_HOME

exec ${MOZILLA_FIVE_HOME}/bin/apprunner ${1+"$@"}
