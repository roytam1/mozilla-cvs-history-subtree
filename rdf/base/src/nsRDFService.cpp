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


#include "nsIAtom.h"
#include "nsIRDFNode.h"
#include "nsIRDFService.h"
#include "nsString.h"
#include "plhash.h"
#include "plstr.h"
#include "prlog.h"

////////////////////////////////////////////////////////////////////////

static NS_DEFINE_IID(kIRDFServiceIID,         NS_IRDFSERVICE_IID);
static NS_DEFINE_IID(kIRDFLiteralIID,         NS_IRDFLITERAL_IID);
static NS_DEFINE_IID(kIRDFResourceIID,        NS_IRDFRESOURCE_IID);
static NS_DEFINE_IID(kIRDFNodeIID,            NS_IRDFNODE_IID);
static NS_DEFINE_IID(kISupportsIID,           NS_ISUPPORTS_IID);

////////////////////////////////////////////////////////////////////////

class ResourceImpl;

class ServiceImpl : public nsIRDFService {
protected:
    PLHashTable* mResources;
    virtual ~ServiceImpl(void);

public:
    ServiceImpl(void);

    // nsISupports
    NS_DECL_ISUPPORTS

    // nsIRDFService
    NS_IMETHOD GetResource(const char* uri, nsIRDFResource** resource);
    NS_IMETHOD GetUnicodeResource(const PRUnichar* uri, nsIRDFResource** resource);
    NS_IMETHOD GetLiteral(const PRUnichar* value, nsIRDFLiteral** literal);

    NS_IMETHOD GetDataSource(const char* uri, nsIRDFDataSource** dataSource);
    NS_IMETHOD GetDatabase(const char** uri, nsIRDFDataBase** dataBase);
    NS_IMETHOD GetBrowserDatabase(nsIRDFDataBase** dataBase);

    void ReleaseNode(const ResourceImpl* resource);
};


////////////////////////////////////////////////////////////////////////

class ResourceImpl : public nsIRDFResource {
public:
    ResourceImpl(ServiceImpl* mgr, const char* uri);
    virtual ~ResourceImpl(void);

    // nsISupports
    NS_DECL_ISUPPORTS

    // nsIRDFNode
    NS_IMETHOD EqualsNode(nsIRDFNode* node, PRBool* result) const;

    // nsIRDFResource
    NS_IMETHOD GetValue(const char* *uri) const;
    NS_IMETHOD EqualsResource(const nsIRDFResource* resource, PRBool* result) const;
    NS_IMETHOD EqualsString(const char* uri, PRBool* result) const;

    // Implementation methods
    const char* GetURI(void) const {
        return mURI;
    }

private:
    char*                mURI;
    ServiceImpl* mMgr;
};


ResourceImpl::ResourceImpl(ServiceImpl* mgr, const char* uri)
    : mMgr(mgr)
{
    NS_INIT_REFCNT();
    NS_IF_ADDREF(mMgr);
    mURI = PL_strdup(uri);
}


ResourceImpl::~ResourceImpl(void)
{
    mMgr->ReleaseNode(this);
    PL_strfree(mURI);
    NS_IF_RELEASE(mMgr);
}


NS_IMPL_ADDREF(ResourceImpl);
NS_IMPL_RELEASE(ResourceImpl);

nsresult
ResourceImpl::QueryInterface(REFNSIID iid, void** result)
{
    if (! result)
        return NS_ERROR_NULL_POINTER;

    *result = nsnull;
    if (iid.Equals(kIRDFResourceIID) ||
        iid.Equals(kIRDFNodeIID) ||
        iid.Equals(kISupportsIID)) {
        *result = NS_STATIC_CAST(nsIRDFResource*, this);
        AddRef();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}

NS_IMETHODIMP
ResourceImpl::EqualsNode(nsIRDFNode* node, PRBool* result) const
{
    nsresult rv;
    nsIRDFResource* resource;
    if (NS_SUCCEEDED(node->QueryInterface(kIRDFResourceIID, (void**) &resource))) {
        rv = EqualsResource(resource, result);
        NS_RELEASE(resource);
    }
    else {
        *result = PR_FALSE;
        rv = NS_OK;
    }
    return rv;
}

NS_IMETHODIMP
ResourceImpl::GetValue(const char* *uri) const
{
    if (!uri)
        return NS_ERROR_NULL_POINTER;

    *uri = mURI;
    return NS_OK;
}

NS_IMETHODIMP
ResourceImpl::EqualsResource(const nsIRDFResource* resource, PRBool* result) const
{
    if (!resource || !result)
        return NS_ERROR_NULL_POINTER;

    *result = (resource == this);
    return NS_OK;
}


NS_IMETHODIMP
ResourceImpl::EqualsString(const char* uri, PRBool* result) const
{
    if (!uri || !result)
        return NS_ERROR_NULL_POINTER;

    *result = (PL_strcmp(uri, mURI) == 0);
    return NS_OK;
}


////////////////////////////////////////////////////////////////////////
//

class LiteralImpl : public nsIRDFLiteral {
public:
    LiteralImpl(const PRUnichar* s);
    virtual ~LiteralImpl(void);

    // nsISupports
    NS_DECL_ISUPPORTS

    // nsIRDFNode
    NS_IMETHOD EqualsNode(nsIRDFNode* node, PRBool* result) const;

    // nsIRDFLiteral
    NS_IMETHOD GetValue(const PRUnichar* *value) const;
    NS_IMETHOD EqualsLiteral(const nsIRDFLiteral* literal, PRBool* result) const;

private:
    nsAutoString mValue;
};


LiteralImpl::LiteralImpl(const PRUnichar* s)
    : mValue(s)
{
    NS_INIT_REFCNT();
}

LiteralImpl::~LiteralImpl(void)
{
}

NS_IMPL_ADDREF(LiteralImpl);
NS_IMPL_RELEASE(LiteralImpl);

nsresult
LiteralImpl::QueryInterface(REFNSIID iid, void** result)
{
    if (! result)
        return NS_ERROR_NULL_POINTER;

    *result = nsnull;
    if (iid.Equals(kIRDFLiteralIID) ||
        iid.Equals(kIRDFNodeIID) ||
        iid.Equals(kISupportsIID)) {
        *result = NS_STATIC_CAST(nsIRDFLiteral*, this);
        AddRef();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}

NS_IMETHODIMP
LiteralImpl::EqualsNode(nsIRDFNode* node, PRBool* result) const
{
    nsresult rv;
    nsIRDFLiteral* literal;
    if (NS_SUCCEEDED(node->QueryInterface(kIRDFLiteralIID, (void**) &literal))) {
        rv = EqualsLiteral(literal, result);
        NS_RELEASE(literal);
    }
    else {
        *result = PR_FALSE;
        rv = NS_OK;
    }
    return rv;
}

NS_IMETHODIMP
LiteralImpl::GetValue(const PRUnichar* *value) const
{
    NS_ASSERTION(value, "null ptr");
    if (! value)
        return NS_ERROR_NULL_POINTER;

    *value = mValue.GetUnicode();
    return NS_OK;
}


NS_IMETHODIMP
LiteralImpl::EqualsLiteral(const nsIRDFLiteral* literal, PRBool* result) const
{
    NS_ASSERTION(literal && result, "null ptr");
    if (!literal || !result)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;
    const PRUnichar* p;
    if (NS_FAILED(rv = literal->GetValue(&p)))
        return rv;

    nsAutoString s(p);

    *result = s.Equals(mValue);
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////
// ServiceImpl


ServiceImpl::ServiceImpl(void)
    : mResources(nsnull)
{
    NS_INIT_REFCNT();
    mResources = PL_NewHashTable(1023,              // nbuckets
                                 PL_HashString,     // hash fn
                                 PL_CompareStrings, // key compare fn
                                 PL_CompareValues,  // value compare fn
                                 nsnull, nsnull);   // alloc ops & priv
}


ServiceImpl::~ServiceImpl(void)
{
    PL_HashTableDestroy(mResources);
}


NS_IMPL_ISUPPORTS(ServiceImpl, kIRDFServiceIID);


NS_IMETHODIMP
ServiceImpl::GetResource(const char* uri, nsIRDFResource** resource)
{
    ResourceImpl* result =
        NS_STATIC_CAST(ResourceImpl*, PL_HashTableLookup(mResources, uri));

    if (! result) {
        result = new ResourceImpl(this, uri);
        if (! result)
            return NS_ERROR_OUT_OF_MEMORY;

        // This is a little trick to make storage more efficient. For
        // the "key" in the table, we'll use the string value that's
        // stored as a member variable of the ResourceImpl object.
        PL_HashTableAdd(mResources, result->GetURI(), result);

        // *We* don't AddRef() the resource, because the resource
        // AddRef()s *us*.
    }

    result->AddRef();
    *resource = result;

    return NS_OK;
}


NS_IMETHODIMP
ServiceImpl::GetUnicodeResource(const PRUnichar* uri, nsIRDFResource** resource)
{
    nsString s(uri);
    char* cstr = s.ToNewCString();
    nsresult rv = GetResource(cstr, resource);
    delete[] cstr;
    return rv;
}


NS_IMETHODIMP
ServiceImpl::GetLiteral(const PRUnichar* uri, nsIRDFLiteral** literal)
{
    LiteralImpl* result = new LiteralImpl(uri);
    if (! result)
        return NS_ERROR_OUT_OF_MEMORY;

    *literal = result;
    NS_ADDREF(result);
    return NS_OK;
}

NS_IMETHODIMP
ServiceImpl::GetDataSource(const char* uri, nsIRDFDataSource** dataSource)
{
    PR_ASSERT(0);
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
ServiceImpl::GetDatabase(const char** uri, nsIRDFDataBase** dataBase)
{
    PR_ASSERT(0);
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
ServiceImpl::GetBrowserDatabase(nsIRDFDataBase** dataBase)
{
    PR_ASSERT(0);
    return NS_ERROR_NOT_IMPLEMENTED;
}


void
ServiceImpl::ReleaseNode(const ResourceImpl* resource)
{
    PL_HashTableRemove(mResources, resource->GetURI());
}

////////////////////////////////////////////////////////////////////////

nsresult
NS_NewRDFService(nsIRDFService** mgr)
{
    ServiceImpl* result = new ServiceImpl();
    if (! result)
        return NS_ERROR_OUT_OF_MEMORY;

    *mgr = result;
    NS_ADDREF(result);
    return NS_OK;
}
