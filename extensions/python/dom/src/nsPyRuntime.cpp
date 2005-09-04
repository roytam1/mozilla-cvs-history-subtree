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
