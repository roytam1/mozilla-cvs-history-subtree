# This is vaguely related to an nsIScriptContext.  The pydom C code
# just calls back into this class - but that C code still retains some of the
# functionality.

import types, new, logging

import domcompile

from xpcom.client import Component
from xpcom import components, primitives, COMException, nsError, _xpcom

import _nsdom

IID_nsIScriptGlobalObject = _nsdom.IID_nsIScriptGlobalObject
IID_nsIDOMXULElement = components.interfaces.nsIDOMXULElement

# Borrow the xpcom logger (but use a new sub-logger)
logger = logging.getLogger("xpcom.nsdom")
# Note that may calls to logger.debug are prefixed with if __debug__.
# This is to prevent *any* penalty in non-debug mode for logging in perf
# critical areas.

# Also note that assert statements have no cost in non-debug builds.

# XUL Elements don't expose the XBL implemented interfaces via classinfo.
# So we cheat :)
xul_element_interfaces = {
    # tagName:      [IID, ...]
    'textbox':      [components.interfaces.nsIDOMXULTextBoxElement],
    'button':       [components.interfaces.nsIDOMXULButtonElement],
    'checkbox':     [components.interfaces.nsIDOMXULCheckboxElement],
    'image':        [components.interfaces.nsIDOMXULImageElement],
}

# The event listener class we attach to an object for addEventListener
class EventListener:
    _com_interfaces_ = components.interfaces.nsIDOMEventListener
    def __init__(self, evt_name, handler, globs = None):
        if callable(handler):
            self.func = handler
        else:
            # event name for addEventListener has no 'on' prefix
            func_name = "on" + evt_name
            # xxx - what to do about arg names?  It looks like onerror
            # still only gets one arg here anyway - handleEvent only takes
            # one!
            arg_names = ('event',)
            co = domcompile.compile_function(handler,
                                             "inline event '%s'" % evt_name,
                                             func_name,
                                             arg_names)
            g = {}
            exec co in g
            self.func = g[func_name]
        self.globals = globs or globals()

    def handleEvent(self, event):
        # Although handler is already a function object, we must re-bind to
        # new globals
        f = new.function(self.func.func_code, self.globals, self.func.func_name)
        args = (event,)
        # We support having less args declared than supplied, a-la JS.
        # (This can only happen when passed a function object - we always
        # compile a string handler into a function with 1 arg
        args = args[:f.func_code.co_argcount]
        return f(*args)

class WrappedNative(Component):
    """Implements the xpconnect concept of 'wrapped natives' and 'expandos'.

    DOM objects can have arbitrary values set on them.  Once this is done for
    the first time, it gets stored in a map in the context.  This leads to
    cycles, which must be cleaned up when the context is closed.
    """
    def __init__(self, context, obj):
        # Store our context - but our context doesn't keep a reference
        # to us until we call _remember_object() on the context.
        self.__dict__['_context_'] = context
        # We store expandos in a seperate dict rather than directly in our
        # __dict__.  No real need for this other than to prevent these
        # attributes clobbering ones we need to work!
        self.__dict__['_expandos_'] = {}
        Component.__init__(self, obj)

    def __repr__(self):
        iface_desc = self._get_classinfo_repr_()
        return "<DOM object '%s' (%s)>" % (self._object_name_,iface_desc)
        
    def __getattr__(self, attr):
        # If it exists in expandos, always return it.
        if attr.startswith("__"):
            raise AttributeError, attr
        # expandos come before interface attributes (which may be wrong.  If
        # we do this, why not just store it in __dict__?)
        expandos = self._expandos_
        if expandos.has_key(attr):
            return expandos[attr]
        return Component.__getattr__(self, attr)

    def __setattr__(self, attr, value):
        try:
            Component.__setattr__(self, attr, value)
        except AttributeError:
            # Set it as an 'expando'.  It looks like we should delegate *all*
            # to the outside object.
            logger.debug("%s set expando property %r=%r for object %r",
                         self, attr, value, self._comobj_)
            # and register if an event.
            if attr.startswith("on"):
                # I'm quite confused by this :(
                target = self._comobj_
                if _nsdom.IsOuterWindow(target):
                    target = _nsdom.GetCurrentInnerWindow(target)
                go = self._context_.globalObject
                scope = self._context_.GetNativeGlobal()
                if callable(value):
                    # no idea if this is right - set the compiled object ourselves.
                    self._expandos_[attr] = value
                    _nsdom.RegisterScriptEventListener(go, scope, target, attr)
                else:
                    _nsdom.AddScriptEventListener(target, attr, value, False, False)
                    _nsdom.CompileScriptEventListener(go, scope, target, attr)
            else:
                self._expandos_[attr] = value
            self._context_._remember_object(self)

    def _build_all_supported_interfaces_(self):
        # Generally called as pyxpcom is finding an attribute.
        Component._build_all_supported_interfaces_(self)
        # Now hard-code certain element names we know about, as the XBL
        # implemented interfaces are not exposed by this.
        ii = self.__dict__['_interface_infos_']
        if ii.has_key(IID_nsIDOMXULElement):
            # Is a DOM element - see if in our map.
            tagName = self.tagName
            interfaces = xul_element_interfaces.get(tagName, [])
            for interface in interfaces:
                if not ii.has_key(interface):
                        try:
                            self._remember_interface_info(interface)
                        except COMException, why:
                            # eeek - if we fail to build an interface we will just
                            # try and fail again next time.  We need None in the map
                            logger.warning("Failed to build interface info for %s: %s" \
                                           % (interface, why))
        else:
            logger.info("Not a DOM element - not hooking extra interfaces")

    def addEventListener(self, event, handler, useCapture=False):
        # We need to transform string or function objects into
        # nsIDOMEventListener interfaces.
        if not hasattr(handler, "handleEvent"): # may already be a handler instance.
            # Wrap it in our instance, which knows how to convert
            handler = EventListener(event, handler, self._context_.GetNativeGlobal())
        
        base = self.__getattr__('addEventListener')
        base(event, handler, useCapture)

class WrappedNativeGlobal(WrappedNative):
    # Special support for our global.
    def __repr__(self):
        iface_desc = self._get_classinfo_repr_()
        outer = _nsdom.IsOuterWindow(self._comobj_)
        return "<GlobalWindow outer=%s %s>" % (outer, iface_desc)

class ScriptContext:
    def __init__(self):
        self.globalNamespace = {} # must not change identity!
        self._reset()

    def _reset(self):
        self._remembered_objects_ = {}
        self.globalObject = None
        self.globalNamespace.clear()

    def __repr__(self):
        return "<ScriptContext at %d>" % id(self)

    # Called by the _nsdom C++ support to wrap the DOM objects.
    def MakeInterfaceResult(self, object, iid):
        if __debug__:
            logger.debug("MakeInterfaceResult for %r (remembered=%s)",
                         object,
                         self._remembered_objects_.has_key(object))
        assert not hasattr(object, "_comobj_"), "should not be wrapped!"
        # If it is remembered, just return that object.
        try:
            return self._remembered_objects_[object]
        except KeyError:
            # Make a new wrapper - but don't remember the wrapper until
            # we need to, when a property is set on it.

            # We should probably QI for nsIClassInfo, and only do this special
            # wrapping for objects with the DOM flag set.
        
            if iid == IID_nsIScriptGlobalObject:
                klass = WrappedNativeGlobal
            else:
                klass = WrappedNative
            return klass(self, object)

    def _remember_object(self, object):
        # You must only try and remember a wrapped object.
        # Once an object has been wrapped once, all further requests must
        # be identical
        assert self._remembered_objects_.get(object._comobj_, object)==object, \
               "Previously remembered object is not this object!"
        self._remembered_objects_[object._comobj_] = object
        logger.debug("%s remembering object %r - now %d items", self, object,
                     len(self._remembered_objects_))

    def _fixArg(self, arg):
        try:
            argv = arg.QueryInterface(components.interfaces.nsIArray)
        except COMException, why:
            if why.errno != nsError.NS_NOINTERFACE:
                raise
            # This is not an array - see if it is a variant or primitive.
            try:
                var = arg.queryInterface(components.interfaces.nsIVariant)
                parent = None
                if self.globalObject is not None:
                    parent = self.globalObject._comobj_
                if parent is None:
                    logger.warning("_fixArg for context with no global??")
                return _xpcom.GetVariantValue(var, parent)
            except COMException, why:
                if why.errno != nsError.NS_NOINTERFACE:
                    raise
                try:
                    return primitives.GetPrimitive(arg)
                except COMException, why:
                    if why.errno != nsError.NS_NOINTERFACE:
                        raise
                return arg
        # Its an array - do each item
        ret = []
        for i in range(argv.length):
            val = argv.queryElementAt(i, components.interfaces.nsISupports)
            ret.append(self._fixArg(val))
        return ret

    def GetNativeGlobal(self):
        return self.globalNamespace

    def InitContext(self, globalObject):
        self._reset()
        self.globalObject = globalObject
        if globalObject is None:
            logger.debug("%r initializing with NULL global, ns=%d", self,
                         id(self.globalNamespace))
        else:
            assert isinstance(globalObject, WrappedNativeGlobal), \
                   "Out global should have been wrapped in WrappedNativeGlobal"
            if __debug__:
                logger.debug("%r initializing (outer=%s), ns=%d", self,
                             _nsdom.IsOuterWindow(globalObject),
                             id(self.globalNamespace))
            ns = globalObject.__dict__['_expandos_'] = self.globalNamespace
            # Do an optimized 'setattr' for the global
            self._remember_object(globalObject)
            ns['this'] = globalObject
            # How is this magic supposed to work?  What others are there?
            ns['window'] = globalObject
            try:
                ns['document'] = globalObject.document
            except AttributeError:
                logger.warning("'this' object has no document global")

    def FinalizeClasses(self, globalObject):
        self._reset()

    def ClearScope(self, globalObject):
        if __debug__:
            logger.debug("%s.ClearScope %r (%d)", self, globalObject, id(globalObject))
        assert globalObject is self.GetNativeGlobal(), "Wrong global being cleared?"
        self._reset()

    def FinalizeContext(self):
        if __debug__:
            logger.debug("%s.FinalizeContext", self)
        self._reset()

    def ExecuteScript(self, scriptObject, scopeObject):
        if __debug__:
            logger.debug("%s.ExecuteScript %r in scope %s",
                         self, scriptObject, id(scopeObject))
        if scopeObject is None:
            scopeObject = self.GetNativeGlobal()
        assert scopeObject is self.GetNativeGlobal(), "Global was changed??"
        assert type(scriptObject) == types.CodeType, \
               "Script object should be a code object (got %r)" % (scriptObject,)
        exec scriptObject in scopeObject

    def CompileScript(self, text, scopeObject, principal, url, lineno, version):
        # The line number passed is the first; offset is -1
        return domcompile.compile(text, url, lineno=lineno-1)

    def CompileEventHandler(self, principal, name, argNames, body, url, lineno):
        co = domcompile.compile_function(body, url, name, argNames,
                                         lineno=lineno-1)
        g = {}
        exec co in g
        return g[name]

    def CallEventHandler(self, target, scope, handler, argv):
        if __debug__:
            logger.debug("CallEventHandler %r on target %s in scope %s",
                         handler, target, id(scope))
        assert scope is self.GetNativeGlobal(), "scope was changed??"
        globs = self.globalObject._expandos_.copy()
        globs['this'] = target # XXX - what should this be exposed as?
        # Although handler is already a function object, we must re-bind to
        # new globals
        f = new.function(handler.func_code, globs, handler.func_name,
                         handler.func_defaults)
        args = tuple(self._fixArg(argv))
        # We support having less args declared than supplied, a-la JS.
        args = args[:handler.func_code.co_argcount]
        return f(*args)

    def BindCompiledEventHandler(self, target, scope, name, handler):
        if __debug__:
            logger.debug("%s.BindCompiledEventHandler (%s=%r) on target %s in scope %s",
                         self, name, handler, target, id(scope))
        assert scope is self.GetNativeGlobal(), "scope was changed??"
        # Keeps a ref to both the target and handler.
        ns = target._expandos_
        ns[name] = handler
        self._remember_object(target)

    def GetBoundEventHandler(self, target, scope, name):
        if __debug__:
            logger.debug("%s.GetBoundEventHandler for '%s' on target %s in scope %s",
                         self, name, target, id(scope))
        assert scope is self.GetNativeGlobal(), "scope was changed??"
        # No handler by that name?  Just let the py exception propogate - it
        # really should already have been bound, so we don't want it silent.
        # EEEK - this has something to do with inner vs outer windows
        # and is almost certainly not correct
        ns = target._expandos_
        try:
            return ns[name]
        except KeyError:
            logger.info("Event handler %s not found in my namespace - checking scope",
                        name)
            return scope[name]

    def SetProperty(self, target, name, value):
        if __debug__:
            logger.debug("%s.SetProperty for %s=%r", self, name, value)
        target[name] = self._fixArg(value)
