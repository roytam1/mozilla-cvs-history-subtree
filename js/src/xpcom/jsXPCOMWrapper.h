#ifndef _jsIScriptable
#define _jsIScriptable
#include "jsapi.h"
#include "nsISupports.h"
#include "nsIFactory.h"

extern const nsIID factory_iid;
extern const nsIID vanilla_iid;

class JSXPCOM {
public:
	// XPCOM -> JS. more types to come.
	static JS_PUBLIC_API(nsresult) ToJS(JSContext* cx, double d, jsval* v);
	static JS_PUBLIC_API(nsresult) ToJS(JSContext* cx, char* s, jsval* v);
	static JS_PUBLIC_API(nsresult) ToJS(JSContext* cx, nsISupports* o,
					    jsval* v);
	// JS -> XPCOM. more types to come.
	static JS_PUBLIC_API(nsresult) FromJS(JSContext* cx, jsval v,
					      double* d);
	static JS_PUBLIC_API(nsresult) FromJS(JSContext* cx, jsval v,
					      char** s, size_t sz);
	static JS_PUBLIC_API(nsresult) FromJS(JSContext* cx, jsval v,
					      nsISupports** nsi);
};

// clsid
#define JSXPCOM_CLSID \
    { 0, 0, 0, \
	{0, 0, 0, 0, 0, 0, 0, 0}}

// iid
#define JSXPCOM_IID \
    { 0, 0, 0, \
	{0, 0, 0, 0, 0, 0, 0, 0}}

extern const nsIID JSXPCOM_clsid;
extern const nsIID JSXPCOM_iid;

class jsIXPCOMWrapper : public nsISupports {
	virtual JSObject* getJS() =0;
}

// this class makes a JS object be an XPCOM component
class jsXPCOMWrapper : public jsWrapper {
	JSContext* cx;
	JSObject* obj;
public:

	jsXPCOMWrapper(JSContext* acx, JSObject* aobj) : cx(acx), obj(aobj) {
	    jsval v = OBJECT_TO_JSVAL(obj);
	    JS_AddRoot(cx, &v);
	}

	~jsXPCOMWrapper() {
	    jsval v = OBJECT_TO_JSVAL(obj);
	    /*
	     * XXX What if the context we were originally on goes away before
	     * the destructor is called?  Objects aren't per-context, so it'd
	     * be perfectly legal for that to happen.
	     */
	    JS_RemoveRoot(cx, &v);
	}

	JSObject* getJS() { return obj; }

	NS_DECL_ISUPPORTS
};

#endif
