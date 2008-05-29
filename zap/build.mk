# We build a modified version of xulrunner with the patches mentioned
# on bug #317491. In future, when zap works with stock xulrunner
# versions, we can separate xulrunner and zap builds, as e.g. described here:
# http://developer.mozilla.org/en/docs/Creating_XULRunner_Apps_with_the_Mozilla_Build_System

include $(topsrcdir)/xulrunner/build.mk

######################################################################
# zap api ('zapi') buid:

# We put this in the toolkit tier, so that it gets built before
# xulrunner. E.g. on OS X, xulrunner will package dist/bin/components/
# into dist/XUL.framework as part of the 'libs' build phase, so zapi
# libs must be built by then.

tier_toolkit_dirs += zap/base zap/netutils zap/zmk zap/sdp zap/sip 

# xxx might want to split zap/base, zap/netutils and zap/zmk into
# tier_gecko_dirs, if we end up using them in mozilla code.

######################################################################
# zap sip client build:

tier_app_dirs += zap/client
