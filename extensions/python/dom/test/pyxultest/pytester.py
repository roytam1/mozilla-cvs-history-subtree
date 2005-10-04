# Runs non-ui related test code.
from xpcom import components
import xpcom

def do_onload():
    # Why can't I get args??
    # Intention is to look at window.arguments and determine what 'test'
    # we are being asked to run.
    print "OnLoad called"
    raise "foo"

def do_onerror(*arguments):
    print "OnError called"
    print "Args are", arguments
