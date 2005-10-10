# An out-of-line script for this Python demo

import sys
import xpcom
from xpcom import components
import nsdom

# Utility functions
# Write something to the textbox - eg, error or trace messages.
def write( msg, *args):
    tb = this.document.getElementById("output_box")
    val = tb.getAttribute("value")
    val += (msg % args) + "\n"
    tb.setAttribute("value", val)

# An event listener class to test explicit hooking of events via objects.
# Note Python can now just call addEventListener a-la JS
class EventListener:
    _com_interfaces_ = components.interfaces.nsIDOMEventListener
    def __init__(self, handler, globs = None):
        try:
            self.co = handler.func_code
        except AttributeError:
            self.co = compile(handler, "inline script", "exec")
        self.globals = globs or globals()
    
    def handleEvent(self, event):
        exec self.co in self.globals

# An event function to handle onload - but hooked up manually rather than via
# having the magic name 'onload'
def do_load():
    input = this.document.getElementById("output_box")
    # Clear the text in the XUL
    input.setAttribute("value", "")
    write("This is the Python on XUL demo using\nPython " + sys.version)

    # Sadly no inline event handlers yet - hook up a click event.
    button = this.document.getElementById("but_dialog")
    button.addEventListener('click', 'write("hello from the click event for the dialog button")', False)

# Add an event listener as a function
this.window.addEventListener('load', do_load, False)
# Add another one just to test passing a string instead of a function.
this.window.addEventListener('load', "print 'hello from string event handler'", False)
# And yet another with an explicit EventListener instance. (on error tests can't do this?)
this.window.addEventListener('load', EventListener('print "hello from an object event handler"'), False)

# Some other little functions called by the chrome
def on_but_dialog_click():
    write("Button clicked from %s" %  this.window.location.href)
    # window.open doesn't work as JS has special arg handling :(
    # for now, use our hacky (but quite wonderful) JSExec function.
    import nsdom
    nsdom.JSExec(this, 'this.window.open("chrome://pyxultest/content/dialog.xul", "my-dialog", "chrome")')
    #new = this.window.open("chrome://pyxultest/content/dialog.xul", "my-dialog", "chrome")
    #print "The new window is", new

def run_tests():
    # I wish I could reach into the window and get all tests!
    tests = """ test_error_explicit
                test_error_explicit_string
                test_wrong_event_args
                test_error_eventhandler_object
                test_error_eventhandler_function
                test_error_eventhandler_string""".split()
    keep_open = this.document.getElementById("keep_tests_open").getAttribute("checked")
    for test in tests:
        write("Running test %s" % test)
        suffix = ', "%s"' % test
        if keep_open:
            suffix += ', "-k"'
        cmd = 'this.window.openDialog("chrome://pyxultest/content/pytester.xul", "my-dialog", "modal"%s)' % suffix
        print "cmd is", cmd
        nsdom.JSExec(this, cmd)
    if keep_open:
        write("Ran all the tests - the windows told you if the tests worked")
    else:
        write("Ran all the tests - if you saw no window, it worked!")
