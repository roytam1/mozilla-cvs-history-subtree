/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef nsSOAPTypeRegistry_h__
#define nsSOAPTypeRegistry_h__

#include "nsString.h"
#include "nsISOAPType.h"
#include "nsISOAPTypeRegistry.h"
#include "nsISecurityCheckedComponent.h"
#include "nsIDOMElement.h"
#include "nsISOAPMarshaller.h"
#include "nsISOAPUnmarshaller.h"
#include "nsCOMPtr.h"
#include "nsHashtable.h"

class nsSOAPTypeRegistry : public nsISOAPTypeRegistry,
		    public nsISecurityCheckedComponent
{
public:
  nsSOAPTypeRegistry();
  virtual ~nsSOAPTypeRegistry();

  NS_DECL_ISUPPORTS

  // nsISOAPTypeRegistry
  NS_DECL_NSISOAPTYPEREGISTRY

  // nsISecurityCheckedComponent
  NS_DECL_NSISECURITYCHECKEDCOMPONENT

protected:
  nsSupportsHashtable* mTypeIDs;
  nsSupportsHashtable* mSchemaIDs;
  nsCOMPtr<nsISOAPTypeRegistry> mDefault;

};

class nsSOAPDefaultTypeRegistry : nsSOAPTypeRegistry
{
public:
  nsSOAPDefaultTypeRegistry();
};

#endif
