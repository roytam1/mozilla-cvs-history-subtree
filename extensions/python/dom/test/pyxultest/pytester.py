# Runs test code in a UI environment.  Generally anything which can be tested
# without visual confirmation is suitable for running here.
# Note too that the simple fact we *can* display a UI and run tests is
# exercising lots of things.
import sys
from xpcom import components
import xpcom

close_on_success = True

# Utility functions
# Write something to the textbox - eg, error or trace messages.
def write( msg, *args):
    tb = this.document.getElementById("output-box")
    val = tb.getAttribute("value")
    val += (msg % args) + "\n"
    tb.setAttribute("value", val)

def success():
    # This gets called if the test works.  If it is not called, it must not
    # have worked :)
    write("The test worked!")
    if close_on_success:
        print "Closing the window - nothing interesting to see here!"
        window.close()

def cause_error():
    write("Raising an error")
    raise "Testing Testing"

def handle_error_with_cancel(errMsg, source, lineno):
    success()

def test_error_explicit():
    "Test an explicit assignment of a function object to onerror"
    # Assign a function to an onerror attribute
    write("Assigning a function to window.onerror")
    window.onerror = handle_error_with_cancel
    cause_error()

def test_error_explicit_string():
    "Test an explicit assignment of a string to onerror"
    # Assign a string to an onerror attribute
    write("Assigning a string to window.onerror")
    window.onerror = "handle_error_with_cancel(event, source, lineno)"
    cause_error()

class EventListener:
    _com_interfaces_ = components.interfaces.nsIDOMEventListener
    def handleEvent(self, event):
        write("nsIDOMEventListener caught the event")
        success()

def test_error_eventhandler():
    """Test we can explicitly hook the error with our own nsIDOMEventListener"""
    write("Creating an nsIDOMEventListener to listen for the event")
    window.addEventListener("onerror", EventListener(), False)
    write("For some reason 'onerror' is not working.  Onload appears to?")
    write("I need to check out if this works from JS first")
    cause_error()
    
# The general event handlers and test runner code.
def do_onload(event):
    global close_on_success
    klasses = []
    if len(this.arguments) and this.arguments[0]:
        try:
            func = globals()[this.arguments[0]]
        except KeyError:
            print "*** No such test", this.arguments[0]
            func = None
    else:
        func = test_error_explicit_string
        close_on_success = False
    if len(this.arguments)>1 and this.arguments[1]=="-k":
        close_on_success = False
    if func:
        write(func.__doc__ or func.__name__)
        func()
