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

#ifndef nsRDFCursorUtils_h__
#define nsRDFCursorUtils_h__

#include "nsIRDFCursor.h"
#include "nsSupportsArrayEnumerator.h"
#include "nsIEnumerator.h"

class NS_RDF nsRDFArrayCursor : public nsSupportsArrayEnumerator, 
                                public nsIRDFCursor
{
public:
    NS_DECL_ISUPPORTS_INHERITED

    // nsIRDFCursor methods:
    NS_IMETHOD Advance(void);
    NS_IMETHOD GetDataSource(nsIRDFDataSource** aDataSource);
    NS_IMETHOD GetValue(nsISupports** aValue);

    // nsRDFArrayCursor methods:
    nsRDFArrayCursor(nsIRDFDataSource* aDataSource,
                     nsISupportsArray* valueArray);
    virtual ~nsRDFArrayCursor(void);

protected:
    nsIRDFDataSource* mDataSource;
    PRBool mStarted;
};

////////////////////////////////////////////////////////////////////////////////

class NS_RDF nsRDFArrayAssertionCursor : public nsRDFArrayCursor, 
                                         public nsIRDFAssertionCursor
{
public:
    NS_DECL_ISUPPORTS_INHERITED

    // nsIRDFCursor methods:
    NS_IMETHOD Advance(void) { 
        return nsRDFArrayCursor::Advance();
    }
    NS_IMETHOD GetDataSource(nsIRDFDataSource** aDataSource) {
        return nsRDFArrayCursor::GetDataSource(aDataSource);
    }
    NS_IMETHOD GetValue(nsISupports** aValue) {
        return nsRDFArrayCursor::GetValue(aValue);
    }

    // nsIRDFAssertionCursor methods:
    NS_IMETHOD GetSubject(nsISupports* *aSubject);
    NS_IMETHOD GetPredicate(nsISupports* *aPredicate);
    NS_IMETHOD GetObject(nsISupports* *aObject);
    NS_IMETHOD GetTruthValue(PRBool *aTruthValue);

    // nsRDFArrayAssertionCursor methods:
    nsRDFArrayAssertionCursor(nsIRDFDataSource* aDataSource,
                              nsISupports* subject,
                              nsISupports* predicate,
                              nsISupportsArray* objectsArray,
                              PRBool truthValue = PR_TRUE);
    virtual ~nsRDFArrayAssertionCursor();
    
protected:
    nsISupports* mSubject;
    nsISupports* mPredicate;
    PRBool mTruthValue;
};

////////////////////////////////////////////////////////////////////////////////

class NS_RDF nsRDFSingletonAssertionCursor : public nsIRDFAssertionCursor
{
public:
    NS_DECL_ISUPPORTS

    // nsIRDFCursor methods:
    NS_IMETHOD Advance(void);
    NS_IMETHOD GetDataSource(nsIRDFDataSource** aDataSource);
    NS_IMETHOD GetValue(nsISupports** aValue);

    // nsIRDFAssertionCursor methods:
    NS_IMETHOD GetSubject(nsISupports* *aSubject);
    NS_IMETHOD GetPredicate(nsISupports* *aPredicate);
    NS_IMETHOD GetObject(nsISupports* *aObject);
    NS_IMETHOD GetTruthValue(PRBool *aTruthValue);

    // nsRDFSingletonAssertionCursor methods:

    // node == subject if inverse == false
    // node == target if inverse == true
    // value computed when accessed from datasource
    nsRDFSingletonAssertionCursor(nsIRDFDataSource* aDataSource,
                                  nsISupports* node,
                                  nsISupports* predicate,
                                  PRBool inverse = PR_FALSE,
                                  PRBool truthValue = PR_TRUE);
    virtual ~nsRDFSingletonAssertionCursor();

protected:
    nsIRDFDataSource* mDataSource;
    nsISupports* mNode;
    nsISupports* mPredicate;
    nsISupports* mValue;
    PRBool mInverse;
    PRBool mTruthValue;
    PRBool mConsumed;
};

////////////////////////////////////////////////////////////////////////////////

class NS_RDF nsRDFArrayArcsCursor : public nsRDFArrayCursor
{
public:
    // nsRDFArrayArcsCursor methods:
    nsRDFArrayArcsCursor(nsIRDFDataSource* aDataSource,
                         nsISupports* node,
                         nsISupportsArray* arcs);
    virtual ~nsRDFArrayArcsCursor();

protected:
    nsresult GetPredicate(nsISupports** aPredicate) {
        return GetValue((nsISupports**)aPredicate);
    }

    nsresult GetNode(nsISupports* *result) {
        *result = mNode;
        NS_ADDREF(mNode);
        return NS_OK;
    }
    
    nsISupports* mNode;
};

////////////////////////////////////////////////////////////////////////////////

class NS_RDF nsRDFArrayArcsOutCursor : public nsRDFArrayArcsCursor,
                                       public nsIRDFArcsOutCursor
{
public:
    NS_DECL_ISUPPORTS_INHERITED

    // nsIRDFCursor methods:
    NS_IMETHOD Advance(void) { 
        return nsRDFArrayArcsCursor::Advance();
    }
    NS_IMETHOD GetDataSource(nsIRDFDataSource** aDataSource) {
        return nsRDFArrayArcsCursor::GetDataSource(aDataSource);
    }
    NS_IMETHOD GetValue(nsISupports** aValue) {
        return nsRDFArrayArcsCursor::GetValue(aValue);
    }

    // nsIRDFArcsOutCursor methods:
    NS_IMETHOD GetSubject(nsISupports** aSubject) {
        return GetNode((nsISupports**)aSubject);
    }
    NS_IMETHOD GetPredicate(nsISupports** aPredicate) {
        return nsRDFArrayArcsCursor::GetPredicate(aPredicate);
    }

    // nsRDFArrayArcsOutCursor methods:
    nsRDFArrayArcsOutCursor(nsIRDFDataSource* aDataSource,
                            nsISupports* subject,
                            nsISupportsArray* arcs)
        : nsRDFArrayArcsCursor(aDataSource, subject, arcs) {}
    virtual ~nsRDFArrayArcsOutCursor() {}
};

////////////////////////////////////////////////////////////////////////////////

class NS_RDF nsRDFArrayArcsInCursor : public nsRDFArrayArcsCursor,
                                      public nsIRDFArcsInCursor
{
public:
    NS_DECL_ISUPPORTS_INHERITED

    // nsIRDFCursor methods:
    NS_IMETHOD Advance(void) { 
        return nsRDFArrayArcsCursor::Advance();
    }
    NS_IMETHOD GetDataSource(nsIRDFDataSource** aDataSource) {
        return nsRDFArrayArcsCursor::GetDataSource(aDataSource);
    }
    NS_IMETHOD GetValue(nsISupports** aValue) {
        return nsRDFArrayArcsCursor::GetValue(aValue);
    }

    // nsIRDFArcsInCursor methods:
    NS_IMETHOD GetObject(nsISupports** aObject) {
        return GetNode(aObject);
    }
    NS_IMETHOD GetPredicate(nsISupports** aPredicate) {
        return nsRDFArrayArcsCursor::GetPredicate(aPredicate);
    }

    // nsRDFArrayArcsInCursor methods:
    nsRDFArrayArcsInCursor(nsIRDFDataSource* aDataSource,
                            nsISupports* object,
                            nsISupportsArray* arcs)
        : nsRDFArrayArcsCursor(aDataSource, object, arcs) {}
    virtual ~nsRDFArrayArcsInCursor() {}
};

////////////////////////////////////////////////////////////////////////////////

class NS_RDF nsRDFEnumeratorCursor : public nsIRDFCursor
{
public:
    NS_DECL_ISUPPORTS

    // nsIRDFCursor methods:
    NS_IMETHOD Advance(void);
    NS_IMETHOD GetDataSource(nsIRDFDataSource** aDataSource);
    NS_IMETHOD GetValue(nsISupports** aValue);

    // nsRDFEnumeratorCursor methods:
    nsRDFEnumeratorCursor(nsIRDFDataSource* aDataSource,
                          nsIEnumerator* valueEnumerator);
    virtual ~nsRDFEnumeratorCursor(void);

protected:
    nsIRDFDataSource* mDataSource;
    nsIEnumerator* mEnum;
    PRBool mStarted;
};

////////////////////////////////////////////////////////////////////////////////

class NS_RDF nsRDFEnumeratorAssertionCursor : public nsRDFEnumeratorCursor, 
                                              public nsIRDFAssertionCursor
{
public:
    NS_DECL_ISUPPORTS_INHERITED

    // nsIRDFCursor methods:
    NS_IMETHOD Advance(void) { 
        return nsRDFEnumeratorCursor::Advance();
    }
    NS_IMETHOD GetDataSource(nsIRDFDataSource** aDataSource) {
        return nsRDFEnumeratorCursor::GetDataSource(aDataSource);
    }
    NS_IMETHOD GetValue(nsISupports** aValue) {
        return nsRDFEnumeratorCursor::GetValue(aValue);
    }

    // nsIRDFAssertionCursor methods:
    NS_IMETHOD GetSubject(nsISupports* *aSubject);
    NS_IMETHOD GetPredicate(nsISupports* *aPredicate);
    NS_IMETHOD GetObject(nsISupports* *aObject);
    NS_IMETHOD GetTruthValue(PRBool *aTruthValue);

    // nsRDFEnumeratorAssertionCursor methods:
    nsRDFEnumeratorAssertionCursor(nsIRDFDataSource* aDataSource,
                                   nsISupports* subject,
                                   nsISupports* predicate,
                                   nsIEnumerator* objectsEnumerator,
                                   PRBool truthValue = PR_TRUE);
    virtual ~nsRDFEnumeratorAssertionCursor();
    
protected:
    nsISupports* mSubject;
    nsISupports* mPredicate;
    PRBool mTruthValue;
};

////////////////////////////////////////////////////////////////////////////////

class NS_RDF nsRDFEnumeratorArcsCursor : public nsRDFEnumeratorCursor
{
public:
    // nsRDFEnumeratorArcsOutCursor methods:
    nsRDFEnumeratorArcsCursor(nsIRDFDataSource* aDataSource,
                              nsISupports* node,
                              nsIEnumerator* arcs);
    virtual ~nsRDFEnumeratorArcsCursor();

protected:
    nsresult GetPredicate(nsISupports** aPredicate) {
        return GetValue((nsISupports**)aPredicate);
    }

    nsresult GetNode(nsISupports* *result) {
        *result = mNode;
        NS_ADDREF(mNode);
        return NS_OK;
    }
    
    nsISupports* mNode;
};

////////////////////////////////////////////////////////////////////////////////

class NS_RDF nsRDFEnumeratorArcsOutCursor : public nsRDFEnumeratorArcsCursor,
                                            public nsIRDFArcsOutCursor
{
public:
    NS_DECL_ISUPPORTS_INHERITED

    // nsIRDFCursor methods:
    NS_IMETHOD Advance(void) { 
        return nsRDFEnumeratorArcsCursor::Advance();
    }
    NS_IMETHOD GetDataSource(nsIRDFDataSource** aDataSource) {
        return nsRDFEnumeratorArcsCursor::GetDataSource(aDataSource);
    }
    NS_IMETHOD GetValue(nsISupports** aValue) {
        return nsRDFEnumeratorArcsCursor::GetValue(aValue);
    }

    // nsIRDFArcsOutCursor methods:
    NS_IMETHOD GetSubject(nsISupports** aSubject) {
        return GetNode((nsISupports**)aSubject);
    }
    NS_IMETHOD GetPredicate(nsISupports** aPredicate) {
        return nsRDFEnumeratorArcsCursor::GetPredicate(aPredicate);
    }

    // nsRDFEnumeratorArcsOutCursor methods:
    nsRDFEnumeratorArcsOutCursor(nsIRDFDataSource* aDataSource,
                                 nsISupports* subject,
                                 nsIEnumerator* arcs)
        : nsRDFEnumeratorArcsCursor(aDataSource, subject, arcs) {}
    virtual ~nsRDFEnumeratorArcsOutCursor() {}
};

////////////////////////////////////////////////////////////////////////////////

class NS_RDF nsRDFEnumeratorArcsInCursor : public nsRDFEnumeratorArcsCursor,
                                           public nsIRDFArcsInCursor
{
public:
    NS_DECL_ISUPPORTS_INHERITED

    // nsIRDFCursor methods:
    NS_IMETHOD Advance(void) { 
        return nsRDFEnumeratorArcsCursor::Advance();
    }
    NS_IMETHOD GetDataSource(nsIRDFDataSource** aDataSource) {
        return nsRDFEnumeratorArcsCursor::GetDataSource(aDataSource);
    }
    NS_IMETHOD GetValue(nsISupports** aValue) {
        return nsRDFEnumeratorArcsCursor::GetValue(aValue);
    }

    // nsIRDFArcsInCursor methods:
    NS_IMETHOD GetObject(nsISupports** aObject) {
        return GetNode(aObject);
    }
    NS_IMETHOD GetPredicate(nsISupports** aPredicate) {
        return nsRDFEnumeratorArcsCursor::GetPredicate(aPredicate);
    }

    // nsRDFEnumeratorArcsInCursor methods:
    nsRDFEnumeratorArcsInCursor(nsIRDFDataSource* aDataSource,
                                nsISupports* object,
                                nsIEnumerator* arcs)
        : nsRDFEnumeratorArcsCursor(aDataSource, object, arcs) {}
    virtual ~nsRDFEnumeratorArcsInCursor() {}
};

////////////////////////////////////////////////////////////////////////////////

#endif /* nsRDFCursorUtils_h__ */
