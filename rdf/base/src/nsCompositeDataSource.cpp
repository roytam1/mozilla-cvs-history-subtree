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

/*

  A simple composite data source implementation. A composit data
  source is just a strategy for combining individual data sources into
  a collective graph.


  1) A composite data source holds a sequence of data sources. The set
     of data sources can be specified during creation of the
     database. Data sources can also be added/deleted from a database
     later.

  2) The aggregation mechanism is based on simple super-positioning of
     the graphs from the datasources. If there is a conflict (i.e., 
     data source A has a true arc from foo to bar while data source B
     has a false arc from foo to bar), the data source that it earlier
     in the sequence wins.

     The implementation below doesn't really do this and needs to be
     fixed.

*/

#include "nsIEnumerator.h"
#include "nsIRDFNode.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFObserver.h"
#include "nsRepository.h"
#include "nsVoidArray.h"
#include "nsISupportsArray.h"
#include "prlog.h"

static NS_DEFINE_IID(kIRDFCompositeDataSourceIID, NS_IRDFCOMPOSITEDATASOURCE_IID);
static NS_DEFINE_IID(kIRDFDataSourceIID,      NS_IRDFDATASOURCE_IID);
static NS_DEFINE_IID(kIRDFObserverIID,        NS_IRDFOBSERVER_IID);
static NS_DEFINE_IID(kISupportsIID,           NS_ISUPPORTS_IID);

////////////////////////////////////////////////////////////////////////
// CompositeDataSourceImpl

class CompositeDataSourceImpl : public nsIRDFCompositeDataSource,
                                public nsIRDFObserver
{
protected:
    nsVoidArray*  mObservers;
        
    virtual ~CompositeDataSourceImpl(void);

public:
    CompositeDataSourceImpl(void);
    CompositeDataSourceImpl(char** dataSources);

    nsVoidArray mDataSources;

    // nsISupports interface
    NS_DECL_ISUPPORTS

    // nsIRDFDataSource interface
    NS_IMETHOD Init(const char* uri);

    NS_IMETHOD GetURI(const char* *uri) const;

    NS_IMETHOD GetSource(nsIRDFResource* property,
                         nsIRDFNode* target,
                         PRBool tv,
                         nsIRDFResource** source);

    NS_IMETHOD GetSources(nsIRDFResource* property,
                          nsIRDFNode* target,
                          PRBool tv,
                          nsIEnumerator/*<nsIRDFResource>*/** sources);

    NS_IMETHOD GetTarget(nsIRDFResource* source,
                         nsIRDFResource* property,
                         PRBool tv,
                         nsIRDFNode** target);

    NS_IMETHOD GetTargets(nsIRDFResource* source,
                          nsIRDFResource* property,
                          PRBool tv,
                          nsIEnumerator/*<nsIRDFNode>*/** targets);

    NS_IMETHOD Assert(nsIRDFResource* source, 
                      nsIRDFResource* property, 
                      nsIRDFNode* target,
                      PRBool tv);

    NS_IMETHOD Unassert(nsIRDFResource* source,
                        nsIRDFResource* property,
                        nsIRDFNode* target);

    NS_IMETHOD HasAssertion(nsIRDFResource* source,
                            nsIRDFResource* property,
                            nsIRDFNode* target,
                            PRBool tv,
                            PRBool* hasAssertion);

    NS_IMETHOD AddObserver(nsIRDFObserver* n);

    NS_IMETHOD RemoveObserver(nsIRDFObserver* n);

    NS_IMETHOD ArcLabelsIn(nsIRDFNode* node,
                           nsIEnumerator/*<nsIRDFResource>*/** labels);

    NS_IMETHOD ArcLabelsOut(nsIRDFResource* source,
                            nsIEnumerator/*<nsIRDFNode>*/** labels);

    NS_IMETHOD GetAllResources(nsIEnumerator/*<nsIRDFResource>*/** aCursor);

    NS_IMETHOD Flush();

    NS_IMETHOD IsCommandEnabled(const char* aCommand,
                                nsIRDFResource* aCommandTarget,
                                PRBool* aResult);

    NS_IMETHOD DoCommand(const char* aCommand,
                         nsIRDFResource* aCommandTarget);


    // nsIRDFCompositeDataSource interface
    NS_IMETHOD AddDataSource(nsIRDFDataSource* source);
    NS_IMETHOD RemoveDataSource(nsIRDFDataSource* source);

    // nsIRDFObserver interface
    NS_IMETHOD OnAssert(nsIRDFResource* subject,
                        nsIRDFResource* predicate,
                        nsIRDFNode* object);

    NS_IMETHOD OnUnassert(nsIRDFResource* subject,
                          nsIRDFResource* predicate,
                          nsIRDFNode* object);

    // Implementation methods
    PRBool HasAssertionN(int n, nsIRDFResource* source,
                            nsIRDFResource* property,
                            nsIRDFNode* target,
                            PRBool tv);
};

////////////////////////////////////////////////////////////////////////////////

/*template<class T>*/
class nsRDFCompositeEnumerator : public nsIEnumerator/*<T>*/
{
protected:
    CompositeDataSourceImpl* mCompositeDataSource;
    PRInt32              mDataSourceIndex;
    nsIEnumerator*       mCurrentCursor;
    PRBool               mDone;

    virtual nsresult GetCursor(nsIRDFDataSource* ds, 
                               nsIEnumerator/*<T>*/** result) = 0;
    virtual nsresult GetElement(nsISupports* *result) = 0;

    nsresult NextCursor(void);

public:
    nsRDFCompositeEnumerator(CompositeDataSourceImpl* db);
    virtual ~nsRDFCompositeEnumerator();
    
    NS_DECL_ISUPPORTS

    // nsIEnumerator methods:
    NS_IMETHOD First(void);
    NS_IMETHOD Next(void);
    NS_IMETHOD CurrentItem(nsISupports **aItem) {
        return mCurrentCursor->CurrentItem(aItem);
    }
    NS_IMETHOD IsDone(void);
};
        
nsRDFCompositeEnumerator::nsRDFCompositeEnumerator(CompositeDataSourceImpl* db)
    : mCompositeDataSource(db), 
	  mDataSourceIndex(0),
	  mCurrentCursor(nsnull),
      mDone(PR_FALSE)
{
	NS_INIT_REFCNT();
    NS_ADDREF(mCompositeDataSource);
}

nsRDFCompositeEnumerator::~nsRDFCompositeEnumerator(void)
{
    NS_IF_RELEASE(mCurrentCursor);
    NS_RELEASE(mCompositeDataSource);
}

NS_IMPL_ISUPPORTS(nsRDFCompositeEnumerator, nsIEnumerator::IID());

NS_IMETHODIMP nsRDFCompositeEnumerator::First(void)
{
    mDataSourceIndex = -1;
    return NextCursor();
}

nsresult nsRDFCompositeEnumerator::NextCursor(void)
{
    mDataSourceIndex++;
    NS_IF_RELEASE(mCurrentCursor);
    if (mDataSourceIndex >= mCompositeDataSource->mDataSources.Count()) {
        mDone = PR_TRUE;
        return NS_ERROR_FAILURE;
    }

    nsIRDFDataSource* ds =
        (nsIRDFDataSource*)mCompositeDataSource->mDataSources[mDataSourceIndex];

    nsresult rv = GetCursor(ds, &mCurrentCursor);
    if (NS_FAILED(rv)) {
        mDone = PR_TRUE;
        return rv;
    }
    return mCurrentCursor->First();
}

NS_IMETHODIMP nsRDFCompositeEnumerator::Next(void)
{
    nsresult rv;
    while (1) {
        rv = mCurrentCursor->Next();
        while (NS_FAILED(rv)) { // cursor at end of current datasource
            rv = NextCursor();
            if (NS_FAILED(rv))
                return rv;      // no more datasources
        }
        nsISupports* item;
        rv = GetElement(&item);
        if (NS_SUCCEEDED(rv)) return rv;        // else continue
    }
    return NS_OK;
}

NS_IMETHODIMP nsRDFCompositeEnumerator::IsDone(void)
{
    return mDone ? NS_OK : NS_COMFALSE;
}

////////////////////////////////////////////////////////////////////////////////

/*template<class T>*/
class DBArcsInOutCursor : public nsRDFCompositeEnumerator/*<T>*/
{
protected:
    nsIRDFResource*      mSource;
    nsIRDFNode*          mTarget;
    nsISupportsArray*    mSeen;

    virtual nsresult GetCursor(nsIRDFDataSource* ds, 
                               nsIEnumerator/*<T>*/** result);
    virtual nsresult GetElement(nsISupports* *result);

public:
    DBArcsInOutCursor(CompositeDataSourceImpl* db, nsIRDFNode* node, PRBool arcsOutp);
    virtual ~DBArcsInOutCursor();

    NS_IMETHOD First(void);
};

        
DBArcsInOutCursor::DBArcsInOutCursor(CompositeDataSourceImpl* db,
                                     nsIRDFNode* node,
                                     PRBool arcsOutp)
    : nsRDFCompositeEnumerator(db),
	  mTarget(nsnull),
	  mSource(nsnull),
      mSeen(nsnull)
{
    if (arcsOutp) {
        mSource = (nsIRDFResource*) node;
    } else {
        mTarget = node;
    }
    NS_IF_ADDREF(node); 
}

DBArcsInOutCursor::~DBArcsInOutCursor(void)
{
    for (PRInt32 i = mSeen->Count() - 1; i >= 0; --i) {
        nsIRDFNode* node = (nsIRDFNode*)(*mSeen)[i];
        NS_RELEASE(node);
    }

    NS_IF_RELEASE(mSource);
    NS_IF_RELEASE(mTarget);
    NS_IF_RELEASE(mSeen);
}

nsresult DBArcsInOutCursor::GetCursor(nsIRDFDataSource* ds,
                                      nsIEnumerator/*<T>*/** result)
{
    if (mTarget)
        return ds->ArcLabelsIn(mTarget, result);
    else 
        return ds->ArcLabelsOut(mSource, result);
}

nsresult DBArcsInOutCursor::GetElement(nsISupports* *result)
{
    nsISupports* item;
    nsresult rv = CurrentItem(&item);
    if (NS_FAILED(rv)) 
        return rv;
    // make sure it's an item we haven't seen before
    if (mSeen->IndexOf(item) < 0) {
        mSeen->AppendElement(item);
        *result = item;
        return NS_OK;
    }
    // returning this will cause us to continue to the Next item
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP DBArcsInOutCursor::First(void)
{
    if (mSeen == nsnull) {
        nsresult rv = NS_NewISupportsArray(&mSeen);
        if (NS_FAILED(rv)) return rv;
    }
    return nsRDFCompositeEnumerator::First();
}

////////////////////////////////////////////////////////////////////////
// DBAssertionCursor
//
//   An assertion cursor implementation for the db.
//
/*template<class T>*/
class DBGetSTCursor : public nsRDFCompositeEnumerator/*<T>*/
{
protected:
    nsIRDFResource* mSource;
    nsIRDFResource* mLabel;    
    nsIRDFNode*     mTarget;
    PRBool          mTruthValue;

    virtual nsresult GetCursor(nsIRDFDataSource* ds, 
                               nsIEnumerator/*<T>*/** result);
    virtual nsresult GetElement(nsISupports* *result);

public:
    DBGetSTCursor(CompositeDataSourceImpl* db, nsIRDFNode* u,  
                  nsIRDFResource* property, PRBool inversep, PRBool tv);

    virtual ~DBGetSTCursor();
};

DBGetSTCursor::DBGetSTCursor(CompositeDataSourceImpl* db,
                             nsIRDFNode* u,
                             nsIRDFResource* property,
                             PRBool inversep, 
                             PRBool tv)
    : nsRDFCompositeEnumerator(db),
      mSource(nsnull),
      mLabel(property),
      mTarget(nsnull),
      mTruthValue(tv)
{
    if (!inversep) {
        mSource = (nsIRDFResource*) u;
    } else {
        mTarget = u;
    }

    NS_IF_ADDREF(mSource);
    NS_IF_ADDREF(mTarget);
    NS_IF_ADDREF(mLabel);
}


DBGetSTCursor::~DBGetSTCursor(void)
{
    NS_IF_RELEASE(mLabel);
    NS_IF_RELEASE(mSource);
    NS_IF_RELEASE(mTarget);
}

nsresult DBGetSTCursor::GetCursor(nsIRDFDataSource* ds,
                                  nsIEnumerator/*<T>*/** result)
{
    if (mSource)
        return ds->GetTargets(mSource, mLabel, mTruthValue, result);
    else 
        return ds->GetSources(mLabel, mTarget, mTruthValue, result);
}

nsresult DBGetSTCursor::GetElement(nsISupports* *result)
{
    nsIRDFResource* src;
    nsIRDFNode*     trg;            
    nsresult rv;
    if (mSource) {
        src = mSource;
        rv = mCurrentCursor->CurrentItem((nsISupports**)&trg);
        if (NS_FAILED(rv)) return rv;
    }
    else {
        trg = mTarget;
        rv = mCurrentCursor->CurrentItem((nsISupports**)&src);
        if (NS_FAILED(rv)) return rv;
    }
    PRBool hasNegation = 
        mCompositeDataSource->HasAssertionN(mDataSourceIndex - 1,
                                            src, mLabel, trg, !mTruthValue);
    NS_RELEASE(src);
    NS_RELEASE(trg);
    if (!hasNegation)
        return NS_OK;
    // returning this will cause us to continue to the Next item
    return NS_ERROR_FAILURE;
}

////////////////////////////////////////////////////////////////////////

nsresult
NS_NewRDFCompositeDataSource(nsIRDFCompositeDataSource** result)
{
    CompositeDataSourceImpl* db = new CompositeDataSourceImpl();
    if (! db)
        return NS_ERROR_OUT_OF_MEMORY;

    *result = db;
    NS_ADDREF(*result);
    return NS_OK;
}


CompositeDataSourceImpl::CompositeDataSourceImpl(void)
    : mObservers(nsnull)
{
    NS_INIT_REFCNT();
}


CompositeDataSourceImpl::~CompositeDataSourceImpl(void)
{
    for (PRInt32 i = mDataSources.Count() - 1; i >= 0; --i) {
        nsIRDFDataSource* ds = NS_STATIC_CAST(nsIRDFDataSource*, mDataSources[i]);
        ds->RemoveObserver(this);
        NS_IF_RELEASE(ds);
    }
    delete mObservers;
}

////////////////////////////////////////////////////////////////////////
// nsISupports interface

NS_IMPL_ADDREF(CompositeDataSourceImpl);
NS_IMPL_RELEASE(CompositeDataSourceImpl);

NS_IMETHODIMP
CompositeDataSourceImpl::QueryInterface(REFNSIID iid, void** result)
{
    if (! result)
        return NS_ERROR_NULL_POINTER;

    if (iid.Equals(kIRDFCompositeDataSourceIID) ||
        iid.Equals(kIRDFDataSourceIID) ||
        iid.Equals(kISupportsIID)) {
        *result = NS_STATIC_CAST(nsIRDFCompositeDataSource*, this);
		NS_ADDREF(this);
        return NS_OK;
    }
    else if (iid.Equals(kIRDFObserverIID)) {
        *result = NS_STATIC_CAST(nsIRDFObserver*, this);
        NS_ADDREF(this);
        return NS_OK;
    }
    else {
        *result = nsnull;
        return NS_NOINTERFACE;
    }
}



////////////////////////////////////////////////////////////////////////
// nsIRDFDataSource interface

NS_IMETHODIMP
CompositeDataSourceImpl::Init(const char* uri)
{
    PR_ASSERT(0);
    return NS_ERROR_UNEXPECTED; // XXX CompositeDataSourceImpl doesn't have a URI?
}

NS_IMETHODIMP
CompositeDataSourceImpl::GetURI(const char* *uri) const
{
    PR_ASSERT(0);
    return NS_ERROR_UNEXPECTED; // XXX CompositeDataSourceImpl doesn't have a URI?
}

NS_IMETHODIMP
CompositeDataSourceImpl::GetSource(nsIRDFResource* property,
                                   nsIRDFNode* target,
                                   PRBool tv,
                                   nsIRDFResource** source)
{
    PRInt32 count = mDataSources.Count();
    for (PRInt32 i = 0; i < count; ++i) {
        nsIRDFDataSource* ds = NS_STATIC_CAST(nsIRDFDataSource*, mDataSources[i]);

        if (NS_FAILED(ds->GetSource(property, target, tv, source)))
            continue;

        // okay, found it. make sure we don't have the opposite
        // asserted in a more local data source
        if (!HasAssertionN(count-1, *source, property, target, !tv)) 
            return NS_OK;

        NS_RELEASE(*source);
        return NS_ERROR_RDF_NO_VALUE;
    }
    return NS_ERROR_RDF_NO_VALUE;
}

NS_IMETHODIMP
CompositeDataSourceImpl::GetSources(nsIRDFResource* property,
                                    nsIRDFNode* target,
                                    PRBool tv,
                                    nsIEnumerator/*<nsIRDFResource>*/** result)
{
    if (! result)
        return NS_ERROR_NULL_POINTER;

    *result = new DBGetSTCursor(this, target, property, 1, tv);
    if (! result)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*result);
    return NS_OK;
}

NS_IMETHODIMP
CompositeDataSourceImpl::GetTarget(nsIRDFResource* source,
                                   nsIRDFResource* property,
                                   PRBool tv,
                                   nsIRDFNode** target)
{
    PRInt32 count = mDataSources.Count();
    for (PRInt32 i = 0; i < count; ++i) {
        nsIRDFDataSource* ds = NS_STATIC_CAST(nsIRDFDataSource*, mDataSources[i]);

        if (NS_FAILED(ds->GetTarget(source, property, tv, target)))
            continue;

        // okay, found it. make sure we don't have the opposite
        // asserted in the "local" data source
        if (!HasAssertionN(count-1, source, property, *target, !tv)) 
            return NS_OK;

        NS_RELEASE(*target);
        return NS_ERROR_RDF_NO_VALUE;
    }

    return NS_ERROR_RDF_NO_VALUE;
}

PRBool
CompositeDataSourceImpl::HasAssertionN(int n,
                                       nsIRDFResource* source,
                                       nsIRDFResource* property,
                                       nsIRDFNode* target,
                                       PRBool tv)
{
    int m = 0;
    PRBool result = 0;
    while (m < n) {
        nsIRDFDataSource* ds = (nsIRDFDataSource*) mDataSources[m];
        ds->HasAssertion(source, property, target, tv, &result);
        if (result) return 1;
        m++;
    }
    return 0;
}
    


NS_IMETHODIMP
CompositeDataSourceImpl::GetTargets(nsIRDFResource* source,
                                    nsIRDFResource* property,
                                    PRBool tv,
                                    nsIEnumerator/*<nsIRDFNode>*/** targets)
{
    if (! targets)
        return NS_ERROR_NULL_POINTER;

    nsIEnumerator/*<nsIRDFNode>*/* result;
    result = new DBGetSTCursor(this, source, property, 0, tv);
    if (! result)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(result);
    *targets = result;
    return NS_OK;
}

NS_IMETHODIMP
CompositeDataSourceImpl::Assert(nsIRDFResource* source, 
                                nsIRDFResource* property, 
                                nsIRDFNode* target,
                                PRBool tv)
{
    // Need to add back the stuff for unblocking ...
    for (PRInt32 i = mDataSources.Count() - 1; i >= 0; --i) {
        nsIRDFDataSource* ds = NS_STATIC_CAST(nsIRDFDataSource*, mDataSources[i]);
        if (NS_SUCCEEDED(ds->Assert(source, property, target, tv)))
            return NS_OK;
    }

    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
CompositeDataSourceImpl::Unassert(nsIRDFResource* source,
                                  nsIRDFResource* property,
                                  nsIRDFNode* target)
{
    nsresult rv;
    PRInt32 count = mDataSources.Count();

    for (PRInt32 i = 0; i < count; ++i) {
        nsIRDFDataSource* ds = NS_STATIC_CAST(nsIRDFDataSource*, mDataSources[i]);
        if (NS_FAILED(rv = ds->Unassert(source, property, target)))
            break;
    }

    if (NS_FAILED(rv)) {
        nsIRDFDataSource* ds0 = NS_STATIC_CAST(nsIRDFDataSource*, mDataSources[0]);
        rv = ds0->Assert(source, property, target, PR_FALSE);
    }
    return rv;
}

NS_IMETHODIMP
CompositeDataSourceImpl::HasAssertion(nsIRDFResource* source,
                                      nsIRDFResource* property,
                                      nsIRDFNode* target,
                                      PRBool tv,
                                      PRBool* hasAssertion)
{
    nsresult rv;


    // Otherwise, look through all the data sources to see if anyone
    // has the positive...
    PRInt32 count = mDataSources.Count();
    PRBool hasNegation = 0;
    for (PRInt32 i = 0; i < count; ++i) {
        nsIRDFDataSource* ds = NS_STATIC_CAST(nsIRDFDataSource*, mDataSources[i]);
        if (NS_FAILED(rv = ds->HasAssertion(source, property, target, tv, hasAssertion)))
            return rv;

        if (*hasAssertion)
            return NS_OK;

        if (NS_FAILED(rv = ds->HasAssertion(source, property, target, !tv, &hasNegation)))
            return rv;

        if (hasNegation) {
            *hasAssertion = 0;
            return NS_OK;
        }
    }

    // If we get here, nobody had the assertion at all
    *hasAssertion = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
CompositeDataSourceImpl::AddObserver(nsIRDFObserver* obs)
{
    if (!mObservers) {
        if ((mObservers = new nsVoidArray()) == nsnull)
            return NS_ERROR_OUT_OF_MEMORY;
    }

    // XXX ensure uniqueness?

    mObservers->AppendElement(obs);
    return NS_OK;
}

NS_IMETHODIMP
CompositeDataSourceImpl::RemoveObserver(nsIRDFObserver* obs)
{
    if (!mObservers)
        return NS_OK;

    mObservers->RemoveElement(obs);
    return NS_OK;
}

NS_IMETHODIMP
CompositeDataSourceImpl::ArcLabelsIn(nsIRDFNode* node,
                                     nsIEnumerator/*<nsIRDFResource>*/** labels)
{
    if (! labels)
        return NS_ERROR_NULL_POINTER;

    nsIEnumerator/*<nsIRDFResource>*/* result = new DBArcsInOutCursor(this, node, 0);
    if (! result)
        return NS_ERROR_NULL_POINTER;

    NS_ADDREF(result);
    *labels = result;
    return NS_OK;
}

NS_IMETHODIMP
CompositeDataSourceImpl::ArcLabelsOut(nsIRDFResource* source,
                                      nsIEnumerator/*<nsIRDFNode>*/** labels)
{
    if (! labels)
        return NS_ERROR_NULL_POINTER;

    nsIEnumerator/*<nsIRDFNode>*/* result = new DBArcsInOutCursor(this, source, 1);
    if (! result)
        return NS_ERROR_NULL_POINTER;

    NS_ADDREF(result);
    *labels = result;
    return NS_OK;
}

NS_IMETHODIMP
CompositeDataSourceImpl::GetAllResources(nsIEnumerator/*<nsIRDFResource>*/** aCursor)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
CompositeDataSourceImpl::Flush()
{
    for (PRInt32 i = mDataSources.Count() - 1; i >= 0; --i) {
        nsIRDFDataSource* ds = NS_STATIC_CAST(nsIRDFDataSource*, mDataSources[i]);
        ds->Flush();
    }
    return NS_OK;
}

NS_IMETHODIMP
CompositeDataSourceImpl::IsCommandEnabled(const char* aCommand,
                                          nsIRDFResource* aCommandTarget,
                                          PRBool* aResult)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
CompositeDataSourceImpl::DoCommand(const char* aCommand,
                                   nsIRDFResource* aCommandTarget)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////
// nsIRDFCompositeDataSource methods
// XXX rvg We should make this take an additional argument specifying where
// in the sequence of data sources (of the db), the new data source should
// fit in. Right now, the new datasource gets stuck at the end.
// need to add the observers of the CompositeDataSourceImpl to the new data source.

NS_IMETHODIMP
CompositeDataSourceImpl::AddDataSource(nsIRDFDataSource* source)
{
    NS_ASSERTION(source != nsnull, "null ptr");
    if (! source)
        return NS_ERROR_NULL_POINTER;

    mDataSources.InsertElementAt(source, 0);
    source->AddObserver(this);
    NS_ADDREF(source);
    return NS_OK;
}



NS_IMETHODIMP
CompositeDataSourceImpl::RemoveDataSource(nsIRDFDataSource* source)
{
    NS_ASSERTION(source != nsnull, "null ptr");
    if (! source)
        return NS_ERROR_NULL_POINTER;


    if (mDataSources.IndexOf(source) >= 0) {
        mDataSources.RemoveElement(source);
        source->RemoveObserver(this);
        NS_RELEASE(source);
    }
    return NS_OK;
}

NS_IMETHODIMP
CompositeDataSourceImpl::OnAssert(nsIRDFResource* subject,
                                  nsIRDFResource* predicate,
                                  nsIRDFNode* object)
{
    if (mObservers) {
        for (PRInt32 i = mObservers->Count() - 1; i >= 0; --i) {
            nsIRDFObserver* obs = (nsIRDFObserver*) mObservers->ElementAt(i);
            obs->OnAssert(subject, predicate, object);
            // XXX ignore return value?
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
CompositeDataSourceImpl::OnUnassert(nsIRDFResource* subject,
                                    nsIRDFResource* predicate,
                                    nsIRDFNode* object)
{
    if (mObservers) {
        for (PRInt32 i = mObservers->Count() - 1; i >= 0; --i) {
            nsIRDFObserver* obs = (nsIRDFObserver*) mObservers->ElementAt(i);
            obs->OnUnassert(subject, predicate, object);
            // XXX ignore return value?
        }
    }
    return NS_OK;
}



