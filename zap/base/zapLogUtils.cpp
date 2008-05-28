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
 * The Original Code is the Mozilla SIP client project.
 *
 * The Initial Developer of the Original Code is SSM
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Allan Beaufour <allan@beaufour.dk> (original author)
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

#include "zapLogUtils.h"
#include "nsStringAPI.h"

#ifdef PR_LOGGING
  PRLogModuleInfo *gLogUtilsLog = PR_NewLogModule("logutils");
  #define LOG(x)  PR_LOG(gLogUtilsLog, PR_LOG_DEBUG, x)
#else
  #define LOG(x)
#endif


////////////////////////////////////////////////////////////////////////
// zapLogUtils

zapLogUtils::zapLogUtils()
{
}

zapLogUtils::~zapLogUtils()
{
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_THREADSAFE_ADDREF(zapLogUtils)
NS_IMPL_THREADSAFE_RELEASE(zapLogUtils)

NS_INTERFACE_MAP_BEGIN(zapLogUtils)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapILogUtils)
  NS_INTERFACE_MAP_ENTRY(zapILogUtils)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapILogUtils methods:

NS_IMETHODIMP
zapLogUtils::LogMessage(const nsACString &aModule,
                        const nsACString &aMessage)
{
#ifdef PR_LOGGING
  /**
   * XXX: Use a hash instead?
   * XXX: use C strings in IDL?
   */

  PRUint32 len = mModules.Length();
  PRLogModuleInfo* mod = nsnull;
  const char* moduleStr = nsPromiseFlatCString(aModule).get();
  for (PRUint32 i = 0; i < len; ++i) {
    if (!strcmp(moduleStr, mModules[i]->name)) {
      mod = mModules[i];
      break;
    }
  }
  if (!mod) {
    LOG(("Creating new module for: %s\n", moduleStr));
    mod = PR_NewLogModule(moduleStr);
    NS_ENSURE_TRUE(mod, NS_ERROR_OUT_OF_MEMORY);
    mModules.AppendElement(mod);
  }
  PR_LOG(mod, PR_LOG_DEBUG, ("%s", nsPromiseFlatCString(aMessage).get()));
#endif

  return NS_OK;
}

NS_IMETHODIMP
zapLogUtils::GetLoggingEnabled(PRBool *aVar)
{
  NS_ENSURE_ARG_POINTER(aVar);
  *aVar =
#ifdef PR_LOGGING
    PR_TRUE
#else
    PR_FALSE
#endif
    ;

  return NS_OK;
}
