/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ----- BEGIN LICENSE BLOCK -----
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Mozilla JSCodeLib.
 *
 * The Initial Developer of the Original Code is 
 * Alex Fritze.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Alex Fritze <alex@croczilla.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ----- END LICENSE BLOCK ----- */

#ifndef __MOZ_JSCODELIB_H__
#define __MOZ_JSCODELIB_H__

#include "mozIJSCodeLib.h"
#include "nsDataHashtable.h"
#include "jsapi.h"
#include "nsCOMPtr.h"
#include "nsIXPConnect.h"
#include "nsIJSRuntimeService.h"

#ifndef XPCONNECT_STANDALONE
#include "nsIPrincipal.h"
#endif

////////////////////////////////////////////////////////////////////////
// mozJSCodeLib class

class mozJSCodeLib : public mozIJSCodeLib
{
public:
  mozJSCodeLib();
  ~mozJSCodeLib();
  nsresult Init();

  NS_DECL_ISUPPORTS
  NS_DECL_MOZIJSCODELIB
  
private:
  PRInt32 LoadURL(char **buf, const nsACString &url);
  JSObject *LoadModule(const nsACString &module);
  void UnloadModules();

  class ModulesHash : public nsDataHashtable<nsCStringHashKey,JSObject*>
  {
  public:
    // need this function to be public:
    EntryType *PutEntry(KeyType aKey) {
      return nsDataHashtable<nsCStringHashKey,JSObject*>::PutEntry(aKey);
    }
  };
  ModulesHash mModules;
  
  nsCOMPtr<nsIXPConnect> mXPConnect;
  nsCOMPtr<nsIJSRuntimeService> mRuntimeService;
#ifndef XPCONNECT_STANDALONE
  nsCOMPtr<nsIPrincipal> mSystemPrincipal;
#endif
  
};

#endif // __MOZ_JSCODELIB_H__
