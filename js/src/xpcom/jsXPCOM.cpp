extern "C" {
#include <stdio.h>
#include "jsapi.h"
#include "../jstypes.h"
#include "../jsassert.h"
#include "../jslink.h"
extern PR_IMPORT_DATA(JSObjectOps) js_ObjectOps;
}

#include "jsXPCOMWrapper.h"

struct FakeRegistry {
	nsIID& clsid;
	nsISupport* comp;
};

int uid_length = 2;				// for now
FakeRegistry uid_table[] = {{vanilla_iid,NULL}, {factory_iid,NULL}};

static JSBool
getGUID(JSString *guid_str, nsIID& guid)
{
	char* idstr = JS_GetStringBytes(guid_str);
    if (!guid.Parse(idstr))
		return JS_FALSE;
	else
		return JS_TRUE;
}

static JSBool
xpcom_create(JSContext *cx,JSObject *obj,uintN argc,jsval *argv,jsval *rval)
{
	// look up clsid, and call its queryinterface to resolve iid.
	// the two arguments passed in are two JS strings representing
	// guids.
	nsIID clsid;
	nsIID iid;
	JSString *clsid_str, *iid_str;

	if (!JSVAL_IS_STRING(argv[0]))
		return JS_FALSE;
	if (!JSVAL_IS_STRING(argv[1]))
		return JS_FALSE;
	clsid_str = JSVAL_TO_STRING(argv[0]);
	if (!getGUID(clsid_str, clsid))
		return JS_FALSE;
	iid_str = JSVAL_TO_STRING(argv[1]);
	if (!getGUID(iid_str, iid))
		return JS_FALSE;
	// at this point we are ready to call CoCreateInstance
	// which returns the component as the interface requested.
	// For now, we will fake this, using a builtin table of
	// clsid:s -> components.
	int i;
	for (i=0; i<uid_length; i++)
		if (uid_table[i].clsid.Equals(clsid))
			break;
	if (i == uid_length)
		return JS_FALSE;
	nsISupports *nsi;
	if (uid_table[i].comp->QueryInterface(iid, &nsi) != NS_OK)
		return JS_FALSE;
	// nsi is now the interface pointer to be sent back to JS
	return (jsXPCOM::ToJS(cx,nsi,rval) == NS_OK ? JS_TRUE : JS_FALSE);
}

static JSBool
factory_create(JSContext *cx,JSObject *obj,uintN argc,jsval *argv,jsval *rval)
{
	nsIID iid;
	JSString *iid_str;
	jsval v;
	nsIFactory *nsf;
	nsISupports *nsi;
	// creates a component from factory, and returns an iid pointer to it.
	if (!JSVAL_IS_STRING(argv[0]))
		return JS_FALSE;
	iid_str = JSVAL_TO_STRING(argv[0]);
	if (!getGUID(iid_str, iid))
		return JS_FALSE;
	nsf = JS_GetPrivate(cx,obj);
	if (nsf == NULL)
		return JS_FALSE;
	if (nsf->CreateInstance(NULL,iid,&nsi) != NS_OK)
		return JS_FALSE;
	return (jsXPCOM::ToJS(cx,nsi,rval) == NS_OK ? JS_TRUE : JS_FALSE);
}

static JSBool
xpcom_create_factory(JSContext *cx,JSObject *obj,uintN argc,jsval *argv,jsval *rval)
{
	// Look up clsid, and find its factory interface.
	// The one argument passed in is a JS string representing
	// a guid.
	nsIID clsid;
	JSString *clsid_str;

	if (!JSVAL_IS_STRING(argv[0]))
		return JS_FALSE;
	clsid_str = JSVAL_TO_STRING(argv[0]);
	if (!getGUID(clsid_str, clsid))
		return JS_FALSE;
	int i;
	for (i=0; i<uid_length; i++)
		if (uid_table[i].clsid.Equals(clsid))
			break;
	if (i == uid_length)
		return JS_FALSE;
	nsISupports *nsi;
	if (uid_table[i].comp->QueryInterface(factory_iid, &nsi) != NS_OK)
		return JS_FALSE;
	// nsi is now the interface pointer to be sent back to JS
	if (jsXPCOM::ToJS(cx,nsi,rval) != NS_OK)
		return JS_FALSE;
	JSObject *obj = JSVAL_TO_OBJECT(*rval);
	return (JS_DefineFunction(cx, obj, "create", factory_create, 1, 0) != NULL);
}

/* XXX replace everything below with shell-compat extension mechanism */
static JSBool
Print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	uintN i, n;
	JSString *str;

	for (i = n = 0; i < argc; i++) {
		str = JS_ValueToString(cx, argv[i]);
		if (!str)
			return JS_FALSE;
		printf("%s%s", i ? " " : "", JS_GetStringBytes(str));
		n++;
	}
	if (n)
		putchar('\n');
	return JS_TRUE;
}

static void
Process(JSContext *cx, JSObject *obj, const char *filename)
{
	JSScript *jsrc = JS_CompileFile(cx,obj,filename);
	jsval rval;

	if (jsrc == NULL) {
#ifdef DEBUG
		printf("Failed compilation of %s\n",filename);
#endif
		return;
	}
	if (!JS_ExecuteScript(cx,obj,jsrc,&rval)) {
#ifdef DEBUG
		printf("Failed execution of %s\n",filename);
#endif
		return;
	}
}

static JSFunctionSpec helper_functions[] = {
    /*    name          native          nargs    */
    {"print",           Print,          0},
    {0}
};

static JSClass global_class = {
    "global", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   JS_FinalizeStub
};

static void
my_ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report)
{
    int i, j, k, n;

    fputs("js: ", stderr);
    if (!report) {
		fprintf(stderr, "%s\n", message);
		return;
    }

    if (report->filename)
		fprintf(stderr, "%s, ", report->filename);
    if (report->lineno)
		fprintf(stderr, "line %u: ", report->lineno);
    fputs(message, stderr);
    if (!report->linebuf) {
		putc('\n', stderr);
		return;
    }

    fprintf(stderr, ":\n%s\n", report->linebuf);
    n = report->tokenptr - report->linebuf;
    for (i = j = 0; i < n; i++) {
		if (report->linebuf[i] == '\t') {
			for (k = (j + 8) & ~7; j < k; j++)
				putc('.', stderr);
			continue;
		}
		putc('.', stderr);
		j++;
    }
    fputs("^\n", stderr);
}

int main(int argc, char* const* argv)
{
	JSRuntime *rt;
	JSContext *cx;
	JSObject *glob, *xpcom;
	JSFunction *fun, *ffun;
	const char *filename;

	if (argc >= 2)
		filename = (const char *) argv[1];
	else
		filename = (const char *) "test.js";
	// JS init
	rt = JS_Init(8L * 1024L * 1024L);
	if (!rt)
		return 0;
	cx = JS_NewContext(rt, 8192);
	PR_ASSERT(cx);
	JS_SetErrorReporter(cx, my_ErrorReporter);
	glob = JS_NewObject(cx, &global_class, NULL, NULL);
	PR_ASSERT(glob);
	if (!JS_InitStandardClasses(cx, glob))
		return -1;
	if (!JS_DefineFunctions(cx, glob, helper_functions))
		return -1;
	xpcom = JS_DefineObject(cx, glob, "xpcom", &global_class, NULL, 0);
	if (!xpcom)
		return -1;
	fun = JS_DefineFunction(cx, xpcom, "create", xpcom_create, 2, 0);
	ffun = JS_DefineFunction(cx, xpcom, "create_factory", xpcom_create_factory, 1, 0);
	XPCOM_ops.newObjectMap = js_ObjectOps.newObjectMap;
	XPCOM_ops.destroyObjectMap = js_ObjectOps.destroyObjectMap;
	// fill in uid_table
	Process(cx, glob, filename);
	// destroy fun and ffun??
	JS_DestroyContext(cx);
	JS_DestroyRuntime(rt);
	JS_ShutDown();
	return 1;
}
