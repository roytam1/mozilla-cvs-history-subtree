#include "jsXPCOM.h"

NS_DEFINE_IID(factory_iid,NS_IFACTORY_IID);
NS_IFACTORY_IID(vanilla_iid, NS_ISUPPORTS_IID);
NS_DEFINE_IID(jsinxpcom_clsid, JS_IN_XPCOM_CLSID);
NS_DEFINE_IID(jsinxpcom_iid, JS_IN_XPCOM_IID);

NS_IMPL_ISUPPORTS(JSinXPCOM, jsinxpcom_iid);

static JSBool
Method_call(JSContext *cx,JSObject *obj,uintN argc,jsval *argv,jsval *vp)
{
    JSObject* meth_obj;
    int dispid;
    
    meth_obj = JSVAL_TO_OBJECT(argv[-2]);
    // resolve method name into dispid,
    // store dispid in private slot,
    // marshal arguments,
    // and call invoke.
    dispid = (int)JS_GetPrivate(cx,meth_obj);
    // ...
}

static JSBool
Method_convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
	*vp = OBJECT_TO_JSVAL(obj);
	return JS_TRUE;
}

static JSBool
Method_lookupProperty(JSContext *cx, JSObject *obj, jsid id,
                         JSObject **objp, JSProperty **propp
#if defined JS_THREADSAFE && defined DEBUG
                            , const char *file, uintN line
#endif
			    )
{
    return JS_TRUE;
}

static JSBool
Method_defineProperty(JSContext *cx, JSObject *obj, jsid id, jsval value,
                         JSPropertyOp getter, JSPropertyOp setter,
                         uintN attrs, JSProperty **propp)
{
    return JS_FALSE;
}

static JSBool
Method_getAttributes(JSContext *cx, JSObject *obj, jsid id,
                        JSProperty *prop, uintN *attrsp)
{
    return JS_FALSE;
}

static JSBool
Method_setAttributes(JSContext *cx, JSObject *obj, jsid id,
                        JSProperty *prop, uintN *attrsp)
{
    return JS_TRUE;
}

static JSBool
Method_deleteProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    return JS_FALSE;
}

static JSBool
Method_defaultValue(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
    return JS_TRUE;
}

PR_STATIC_CALLBACK(JSBool)
Method_getProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
	return JS_TRUE;
}

PR_STATIC_CALLBACK(JSBool)
Method_setProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
	return JS_TRUE;
}

static JSBool
Method_newEnumerate(JSContext *cx,JSObject *obj,JSIterateOp enum_op,jsval *statep,jsid *idp)
{
	return JS_TRUE;
}

static JSBool
Method_checkAccess(JSContext *cx, JSObject *obj, jsid id,
		       JSAccessMode mode, jsval *vp, uintN *attrsp)
{
	return JS_TRUE;
}

JSBool MethodCall(JSContext*,JSObject*,uintN,jsval*,jsval*);

JSObjectOps Method_ops = {
    /* Mandatory non-null function pointer members. */
    NULL,     /* newObjectMap */
    NULL,     /* destroyObjectMap */
    Method_lookupProperty,
    Method_defineProperty,
    Method_getProperty,  
    Method_setProperty,
    Method_getAttributes,
    Method_setAttributes,
    Method_deleteProperty,
    Method_defaultValue,
    Method_newEnumerate,
    Method_checkAccess,
    /* Optionally non-null members start here. */
    NULL,                       /* thisObject */
    NULL,                       /* dropProperty */
    Method_call,                /* call */
    NULL,                       /* construct */
    NULL,                       /* xdrObject */
    NULL,                       /* hasInstance */
};

extern "C" PR_IMPORT_DATA(JSObjectOps) js_ObjectOps;

JSObjectOps *
Method_getObjectOps(JSContext *cx, JSClass *clazz)
{
	if (Method_ops.newObjectMap == NULL) {
		Method_ops.newObjectMap = js_ObjectOps.newObjectMap;
		Method_ops.destroyObjectMap = js_ObjectOps.destroyObjectMap;
	}
    return &Method_ops;
}

static JSClass Method_class = {
    "Method", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   Method_Convert,   JS_FinalizeStub,
    Method_getObjectOps,
};

static JSBool
XPCOM_getProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (!JSVAL_IS_STRING(id))
		return JS_FALSE;
	// use typeinfo here!!!
}

JSBool
XPCOM_getProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if (!JSVAL_IS_STRING(id))
		return JS_FALSE;
	// use typeinfo here!!!
}

// resolving o.m(), where o is a wrapped xpcom pointer, 
// by assigning it to an object which relays its calls 
// using the typeinfo invoke.
static JSBool
XPCOM_resolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
	       JSObject **objp)
{
	char *str;
	JSObject *meth_obj;

	if (!JSVAL_IS_STRING(id))
		return JS_TRUE;
	str = JS_GetStringBytes(JSVAL_TO_STRING(id));
	meth_obj = JS_DefineObject(cx, obj, str, &Method_class, NULL, 0);
	// resolve str to dispid
	int dispid;
	JS_SetPrivate(cx,meth_obj,(void*)dispid);
	*objp = obj;
	return JS_TRUE;
}

static void
XPCOM_finalize(JSContext *cx, JSObject *obj)
{
	nsISupports* nsi;

	nsi = (nsISupports*)JS_GetPrivate(cx,obj);
	if (nsi)
		nsi->Release();
}

static JSClass XPCOMWrapper_class = {
    "XPCOMWrapper",
    JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE,
    JS_PropertyStub,JS_PropertyStub,  
	JS_XPCOMGetProperty,JS_XPCOMSetProperty,
    JS_EnumerateStub,(JSResolveOp)XPCOM_Resolve, 
	JS_ConvertStub,XPCOM_Finalize
};

JS_PUBLIC_API(nsresult)
jsXPCOMWrapper::ToJS(JSContext* cx, double d, jsval* v)
{
    return (JS_NewDoubleValue(cx,d,v) == JS_TRUE ?
	    NS_OK : NS_ERROR_FAILURE);
}

// here, use wide strings rather.
JS_PUBLIC_API(nsresult)
jsXPCOMWrapper::ToJS(JSContext* cx, char* s, jsval* v)
{
	JSString* jstr = JS_NewStringCopyZ(cx, s);
	if (jstr == NULL)
		return NS_ERROR_FAILURE;
	*v = STRING_TO_JSVAL(jstr);
	return NS_OK;
}

JS_PUBLIC_API(nsresult)
jsXPCOMWrapper::ToJS(JSContext* cx, nsISupports* nsi, jsval* v)
{
	jsXPCOM* jsx;
	JSObject* jobj;
	jsWrapper* jsw;

	if (nsi->QueryInterface(jsinxpcom_iid,&jsw) == NS_OK) 
		jobj = jsw->getJS();
	else {
		jobj = JS_NewObject(cx,&XPCOMWrapper_class,NULL,NULL);
		if (jobj == NULL)
			return NS_ERROR_FAILURE;
		nsi->AddRef();
		JS_SetPrivate(cx,jobj,(void*)nsi);
	}
	*v = OBJECT_TO_JSVAL(jobj);
	return NS_OK;
}

JS_PUBLIC_API(nsresult)
jsXPCOMWrapper::FromJS(JSContext* cx, jsval v, double* d)
{
	if (JSVAL_IS_DOUBLE(v)) {
		*d = *JSVAL_TO_DOUBLE(v);
		return NS_OK;
	}
	else if (JSVAL_IS_INT(v)) {
		int i = JSVAL_TO_INT(v);
		*d = (double)i;
		return NS_OK;
	}
	else
		return NS_ERROR_FAILURE;
}

// here, use wide strings rather.
JS_PUBLIC_API(nsresult)
jsXPCOMWrapper::FromJS(JSContext* cx, jsval v, char** s, size_t n)
{
	if (JSVAL_IS_STRING(v)) {
		JSString *jstr = JSVAL_TO_STRING(v);
		char* str = JS_GetStringBytes(jstr);
		strncpy(*s, str, n);
		return NS_OK;
	}
	else
		return NS_ERROR_FAILURE;
}

JS_PUBLIC_API(nsresult)
jsXPCOMWrapper::FromJS(JSContext* cx, jsval v, nsISupports** nsi)
{
	if (JSVAL_IS_OBJECT(v)) {
		JSObject* jobj = JSVAL_TO_OBJECT(v);
		if (JS_GetClass(cx, jobj) == &XPCOM_Class) {
			*nsi = (nsISupports*)JS_GetPrivate(cx,jobj);
			return NS_OK;
		}
		else {
			*nsi = (nsISupports*)new JSinXPCOM(cx,jobj);
			if (*nsi == NULL)
				return NS_ERROR_FAILURE;
			else {
				nsi->AddRef();
				return NS_OK;
			}
		}
	}
	else
		return NS_ERROR_FAILURE;
}

// here add global methods for invoke and typeinfo of JSinXPCOM,
// to be exported by the DLL into which this class gets compiled.

