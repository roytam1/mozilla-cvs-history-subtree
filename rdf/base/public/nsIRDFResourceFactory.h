/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef nsIRDFResourceFactory_h__
#define nsIRDFResourceFactory_h__

#include "nsISupports.h"
class nsIRDFResource;

// {8CE57A20-A02C-11d2-8EBF-00805F29F370}
#define NS_IRDFRESOURCEFACTORY_IID \
{ 0x8ce57a20, 0xa02c, 0x11d2, { 0x8e, 0xbf, 0x0, 0x80, 0x5f, 0x29, 0xf3, 0x70 } }


/**
 * A resource factory can be registered with <tt>nsIRDFService</tt> to produce
 * resources with a certain <i>URI prefix</i>. The resource factory will be called
 * upon to create a new resource, which the resource manager will cache.
 *
 * @see nsIRDFService::RegisterResourceFactory
 * @see nsIRDFService::UnRegisterResourceFactory
 */
class nsIRDFResourceFactory : public nsISupports
{
public:
    NS_IMETHOD CreateResource(const char* aURI, nsIRDFResource** aResult) = 0;
};


#endif // nsIRDFResourceFactory_h__

