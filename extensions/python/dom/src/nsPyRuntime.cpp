/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is mozilla.org.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Mark Hammond (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsPyRuntime.h"
#include "nsPyContext.h"
#include "nsIServiceManager.h"
#include "nsICategoryManager.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "nsIScriptObjectPrincipal.h"

extern void init_nsdom();

// QueryInterface implementation for nsPythonRuntime
NS_INTERFACE_MAP_BEGIN(nsPythonRuntime)
  NS_INTERFACE_MAP_ENTRY(nsILanguageRuntime)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsPythonRuntime)
NS_IMPL_RELEASE(nsPythonRuntime)

nsresult
nsPythonRuntime::CreateContext(nsIScriptContext **ret)
{
    if (!Py_IsInitialized()) {
        Py_Initialize();
        PyXPCOM_Globals_Ensure();
    }
    init_nsdom();
    *ret = new nsPythonContext();
    if (!ret)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_IF_ADDREF(*ret);
    return NS_OK;
}

NS_METHOD
nsPythonRuntime::RegisterSelf(nsIComponentManager* aCompMgr,
                                   nsIFile* aPath,
                                   const char* aRegistryLocation,
                                   const char* aComponentType,
                                   const nsModuleComponentInfo *info)
{
    nsresult rv = NS_OK;
    nsCOMPtr<nsICategoryManager> catman =
        do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);

    if (NS_FAILED(rv))
        return rv;

    nsXPIDLCString previous;
    rv = catman->AddCategoryEntry(SCRIPT_LANGUAGE_CATEGORY,
                                  "application/x-python",
                                  NS_SCRIPT_LANGUAGE_PYTHON_CONTRACTID,
                                  PR_TRUE, PR_TRUE, getter_Copies(previous));

    if (NS_FAILED(rv))
        return rv;

    // And also register a short name.
    rv = catman->AddCategoryEntry(SCRIPT_LANGUAGE_CATEGORY,
                                  "python",
                                  NS_SCRIPT_LANGUAGE_PYTHON_CONTRACTID,
                                  PR_TRUE, PR_TRUE, getter_Copies(previous));
    return rv;
}


NS_METHOD
nsPythonRuntime::UnregisterSelf(nsIComponentManager* aCompMgr,
                                     nsIFile* aPath,
                                     const char* aRegistryLocation,
                                     const nsModuleComponentInfo *info)
{

    nsresult rv = NS_OK;
    nsCOMPtr<nsICategoryManager> catman =
        do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);

    if (NS_FAILED(rv))
        return rv;

    catman->DeleteCategoryEntry(SCRIPT_LANGUAGE_CATEGORY,
                                "application/x-python", PR_TRUE);
    catman->DeleteCategoryEntry(SCRIPT_LANGUAGE_CATEGORY,
                                "python", PR_TRUE);
    return rv;
}

PyObject *PyJSExec(PyObject *self, PyObject *args)
{
    PyObject *obGlobal;
    char *code;
    char *url = "<python JSExec script>";
    PRUint32 lineNo = 0;
    PRUint32 version = 0;
    if (!PyArg_ParseTuple(args, "Os|sii", &obGlobal, &code, &url, &lineNo, &version))
        return NULL;
    nsCOMPtr<nsISupports> isup;
    if (!Py_nsISupports::InterfaceFromPyObject(
                obGlobal,
                NS_GET_IID(nsISupports),
                getter_AddRefs(isup),
                PR_FALSE, PR_FALSE))
        return NULL;
    nsCOMPtr<nsIScriptGlobalObject> scriptGlobal = do_QueryInterface(isup);
    if (scriptGlobal == nsnull)
        return PyErr_Format(PyExc_TypeError, "Object is not an nsIScriptGlobal");
    nsIScriptContext *scriptContext =
           scriptGlobal->GetLanguageContext(nsIProgrammingLanguage::JAVASCRIPT);
    if (!scriptContext)
        return PyErr_Format(PyExc_RuntimeError, "No javascript context available");

    // get the principal
    nsIPrincipal *principal = nsnull;
    nsCOMPtr<nsIScriptObjectPrincipal> globalData = do_QueryInterface(scriptGlobal);
    if (globalData)
        principal = globalData->GetPrincipal();
    if (!principal)
        return PyErr_Format(PyExc_RuntimeError, "No nsIPrincipal available");

    nsresult rv;
    void *scope = scriptGlobal->GetLanguageGlobal(nsIProgrammingLanguage::JAVASCRIPT);
    void *scriptObject = nsnull;
    nsAutoString str;
    PRBool bIsUndefined;
    Py_BEGIN_ALLOW_THREADS
    rv = scriptContext->CompileScript( NS_ConvertASCIItoUCS2(code).get(),
                                       strlen(code),
                                       scope,
                                       principal, // no principal??
                                       url,
                                       lineNo,
                                       version,
                                       &scriptObject);
    if (NS_SUCCEEDED(rv))
        rv = scriptContext->ExecuteScript(scriptObject, scope, &str,
                                          &bIsUndefined);
    Py_END_ALLOW_THREADS
    if (NS_FAILED(rv))
        return PyXPCOM_BuildPyException(rv);
    return Py_BuildValue("NN", PyObject_FromNSString(str), PyBool_FromLong(bIsUndefined));
    // XXX - cleanup scriptObject???
}
// We also setup a "fake" module called nsdom with a few utility functions
// available to python.
static struct PyMethodDef methods[]=
{
    {"JSExec", PyJSExec, 1},
    { NULL }
};

////////////////////////////////////////////////////////////
// The module init code.
//
// *NOT* called by Python - this is not a regular Python module.
static PRBool have_init = PR_FALSE;

void 
init_nsdom() {
    if (have_init)
        return;
    CEnterLeavePython _celp;
    // Create the module and add the functions
    Py_InitModule("nsdom", methods);
    have_init = PR_TRUE;
}
