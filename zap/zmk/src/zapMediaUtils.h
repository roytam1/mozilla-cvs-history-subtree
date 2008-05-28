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
 * The Initial Developer of the Original Code is 8x8 Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
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

#ifndef __ZAP_MEDIAUTILS_H__
#define __ZAP_MEDIAUTILS_H__

#include "zapIMediaNode.h"
#include "zapIMediaNodeContainer.h"
#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsXPCOMCIDInternal.h"

#define ZMK_VERIFY_STREAM_TYPE(streamInfo, type)      \
  if (!streamInfo) {                                  \
    NS_ERROR("Missing streamInfo");                   \
    return NS_ERROR_FAILURE;                          \
  }                                                   \
  {                                                   \
    nsCString stype;                                  \
    if (NS_FAILED(streamInfo->GetPropertyAsACString(  \
                    NS_LITERAL_STRING("type"),        \
                    stype)) ||                        \
        stype != NS_LITERAL_CSTRING(type)) {          \
      NS_ERROR("Stream is not of type " type);        \
      return NS_ERROR_FAILURE;                        \
    }                                                 \
  }


#define ZMK_CREATE_STREAM_INFO(streamInfo, type)                    \
  streamInfo = do_CreateInstance(NS_HASH_PROPERTY_BAG_CONTRACTID);  \
  streamInfo->SetPropertyAsACString(NS_LITERAL_STRING("type"),      \
                                    NS_LITERAL_CSTRING(type));


////////////////////////////////////////////////////////////////////////
// A simple autolock guarding a node against modifications

class zapMediaNodeContainerAutoLock
{
public:
  zapMediaNodeContainerAutoLock(zapIMediaNodeContainer* container,
                                zapIMediaNode* node)
      : mContainer(container),
        mNode(node) {
    if (mContainer)
      mContainer->Lock(mNode);
  }

  ~zapMediaNodeContainerAutoLock() {
    if (mContainer)
      mContainer->Unlock(mNode);
  }
  
private:
  nsCOMPtr<zapIMediaNodeContainer> mContainer;
  nsCOMPtr<zapIMediaNode> mNode;
};

#endif // __ZAP_MEDIAUTILS_H__
