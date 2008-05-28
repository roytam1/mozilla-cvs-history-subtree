/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is Joost Technologies B.V. code.
 *
 * The Initial Developer of the Original Code is Joost Technologies B.V.
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Alex Fritze <alex@croczilla.com> (original author)
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

#ifndef __ZAP_MEDIANODEHOLDER_H__
#define __ZAP_MEDIANODEHOLDER_H__

#include "nsCOMPtr.h"
#include "zapIMediaNodeContainer.h"
#include "nsIPropertyBag2.h"
#include "nsAutoPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsStringAPI.h"

////////////////////////////////////////////////////////////////////////
// zapMediaNodeHolder<T>: Helper class taking care of calling
// insertedIntoContainer() and removedFromContainer() on a media node.

template<class T>
class zapMediaNodeHolder
{
public:
  ~zapMediaNodeHolder() { ReleaseNode(); }

  // initialize with a node type:
  nsresult Init(const nsACString& type,
                zapIMediaNodeContainer* container,
                nsIPropertyBag2* node_pars)
  {
    ReleaseNode();
    nsCString clazz =
      NS_LITERAL_CSTRING(ZAP_MEDIANODE_CONTRACTID_PREFIX)+type;
    if (NS_FAILED(CallCreateInstance(clazz.get(), nsnull,
                                     NS_GET_TEMPLATE_IID(T),
                                     getter_AddRefs(mMediaNode)))) {
      mMediaNode = nsnull;
      NS_WARNING("unknown node class");
      return NS_ERROR_FAILURE;
    }
    nsresult rv = mMediaNode->InsertedIntoContainer(container, node_pars);
    if (NS_FAILED(rv)) {
      NS_WARNING("media node initialization failed");
      mMediaNode = nsnull;
    }
    return rv;    
  }

  // initialize with a pointer:
  nsresult Init(T* ptr, zapIMediaNodeContainer* container,
                nsIPropertyBag2* node_pars)
  {
    ReleaseNode();
    mMediaNode = ptr;
    nsresult rv = mMediaNode->InsertedIntoContainer(container, node_pars);
    if (NS_FAILED(rv)) {
      NS_WARNING("media node initialization failed");
      mMediaNode = nsnull;
    }
    return rv;
  }

  void ReleaseNode()
  {
    if (!mMediaNode) return;
    mMediaNode->RemovedFromContainer();
    mMediaNode = nsnull;
  }
  
  PRBool IsEmpty() { return !mMediaNode; }
  
  nsDerivedSafe<T>* operator->() const
  {
    return mMediaNode;
  }

  operator nsDerivedSafe<T>*() const
  {
    return mMediaNode;
  }
  
private:
  nsRefPtr<T> mMediaNode;
};

#endif // __ZAP_MEDIANODEHOLDER_H__

