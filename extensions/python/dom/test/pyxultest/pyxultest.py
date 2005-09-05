# An out-of-line script for this Python demo

import sys
import xpcom
from xpcom import components

# This EventListener class is to work around the fact that
# addEventListener can not automagically wrap a string or function like
# JS can.
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
    input = this.document.getElementById("input-box")
    val = "This is the Python on XUL demo using\nPython " + sys.version
    input.setAttribute("value", val)
    
    # Sadly no inline event handlers yet - hook up a click event.
    button = this.document.getElementById("button1")
    button.addEventListener('click', EventListener('print "hello from the click event"'), False)

# Add the event listener.
this.window.addEventListener('load', EventListener(do_load), False)
# Add another one just to test passing a string instead of a function.
this.window.addEventListener('load', EventListener('print "hello from the event " + str(this.document)'), False)

# Some other little functions called by the chrome
def on_button_click():
    print "Button clicked from", this.window.location.href
    # window.open doesn't work as JS has special arg handling :(
    # for now, use our hacky (but quite wonderful) JSExec function.
    import nsdom
    nsdom.JSExec(this, 'this.window.open("chrome://pyxultest/content/dialog.xul", "my-dialog", "chrome")')
    #new = this.window.open("chrome://pyxultest/content/dialog.xul", "my-dialog", "chrome")
    #print "The new window is", new
