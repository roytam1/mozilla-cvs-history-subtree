# This is vaguely related to an nsIScriptContext.  The pydom C code
# just calls back into this class - but that C code still retains some of the
# functionality.

import types, new

import domcompile

from xpcom.client import Component
from xpcom import components, primitives, COMException

class ScriptContext:
    def __init__(self):
        self._reset()

    def _reset(self):
        self._object_namespaces = {}
        self.globalObject = None
        self.globals = None

    def _getNamespaceFor(self, target, create = False):
        # Gets a 'namespace' for a DOM object.  Used as a placeholder when
        # dynamic attributes are set on the object.
        try:
            return self._object_namespaces[target]
        except KeyError:
            if create:
                ret = {}
                self._object_namespaces[target] = ret
                return ret
            return None

    def GetNativeGlobal(self):
        return self.globals

    def InitContext(self, globalObject):
        self.globalObject = globalObject
        self.globals = {}
        if globalObject is not None:
            self.globals['this'] = Component(globalObject)
        self._object_namespaces = {}

    def FinalizeClasses(self, globalObject):
        self._reset()

    def ExecuteScript(self, scriptObject, scopeObject):
        assert scopeObject is None or scopeObject is self.globals, \
               "Global was changed??"
        if scopeObject is None:
            scopeObject = self.globals
        assert type(scopeObject)==dict, "expecting scope to be a dict!"
        assert type(scriptObject) == types.CodeType, \
               "Script object should be a code object (got %r)" % (scriptObject,)
        exec scriptObject in scopeObject

    def CompileScript(self, text, scopeObject, principal, url, lineno, version):
        return domcompile.compile(text, url, lineno=lineno)

    def CompileEventHandler(self, principal, name, argNames, body, url, lineno):
        co = domcompile.compile_function(body, url, name, argNames,
                                         lineno=lineno)
        g = {}
        exec co in g
        return g[name]

    def CallEventHandler(self, target, scope, handler, argv):
        target = Component(target)
        globs = self.globals.copy()
        globs['this'] = target # XXX - what should this be exposed as?
        # Although handler is already a function object, we must re-bind to
        # new globals
        f = new.function(handler.func_code, globs, handler.func_name)
        args = []
        for i in range(argv.length):
            val = argv.queryElementAt(i, components.interfaces.nsISupports)
            try:
                val = primitives.GetPrimitive(val)
            except COMException:
                pass
            args.append(val)
        args = tuple(args)
        return f(*args)

    def BindCompiledEventHandler(self, target, scope, name, handler):
        ns = self._getNamespaceFor(target, True)
        # Keeps a ref to both the target and handler!
        ns[name] = handler

    def GetBoundEventHandler(self, target, scope, name):
        ns = self._getNamespaceFor(target, True)
        if ns is None:
            return None
        return ns[name]
