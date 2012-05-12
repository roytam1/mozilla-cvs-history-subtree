#! /bin/bash

#### LOCAL MACHINE SETTINGS ####
PORT_64_DBG=8543
PORT_64_OPT=8544
PORT_32_DBG=8545
PORT_32_OPT=8546
if [ "${NSS_TESTS}" = "memleak" ]; then
    PORT_64_DBG=8547
    PORT_64_OPT=8548
    PORT_32_DBG=8549
    PORT_32_OPT=8550
fi

# if your machine's IP isn't registered in DNS,
# you must set appropriate environment variables
# that can be resolved locally.
# For example, if localhost.localdomain works on your system, set:
#HOST=localhost
#DOMSUF=localdomain

# example configuration
case ${HOST} in
host1) 
    JAVA_HOME_64=/opt/jdk/1.6.0_01/SunOS64
    JAVA_HOME_32=/opt/jdk/1.6.0_01/SunOS
    ;;
host2)
    run_bits="32"
    export NSS_TESTS=memleak
    NO_JSS=1
    ;;
esac

