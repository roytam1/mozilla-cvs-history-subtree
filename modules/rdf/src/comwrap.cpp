/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include "rdf.h"

PR_BEGIN_EXTERN_C
PR_PUBLIC_API(void)
_comwrap_NotificationCB(RDF_Event event, void* pdata);
PR_END_EXTERN_C

class rdfDataBaseWrapper;
class rdfCursorWrapper;
class rdfServiceWrapper;


class rdfDatabaseWrapper : public nsIRDFDatabase {
public:
  NS_DECL_ISUPPORTS

  rdfDatabaseWrapper(RDF r);
  virtual ~rdfDatabaseWrapper();

  /* nsIRDFDataSource methods:  */

  NS_METHOD GetName(const RDF_String const * name /* out */ );

  NS_METHOD GetSource(RDF_Node target,
		      RDF_Resource arcLabel,
		      RDF_Resource *source /* out */);

  NS_METHOD GetSource(RDF_Node target,
		      RDF_Resource arcLabel,
		      PRBool tv,
		      RDF_Resource *source /* out */);

  NS_METHOD GetSources(RDF_Node target,
		       RDF_Resource arcLabel,
		       nsIRDFCursor **sources /* out */);

  NS_METHOD GetSources(RDF_Node target,
		       RDF_Resource arcLabel,
		       PRBool tv,
		       nsIRDFCursor **sources /* out */);

  NS_METHOD GetTarget(RDF_Resource source,
		      RDF_Resource arcLabel,
		      RDF_ValueType targetType,
		      RDF_NodeStruct& target /* in/out */);

  NS_METHOD GetTarget(RDF_Resource source,
		      RDF_Resource arcLabel,
		      RDF_ValueType targetType,
		      PRBool tv,
		      RDF_NodeStruct& target /* in/out */);

  NS_METHOD GetTargets(RDF_Resource source,
		       RDF_Resource arcLabel,
		       RDF_ValueType targetType,
		       nsIRDFCursor **targets /* out */);

  NS_METHOD GetTargets(RDF_Resource source,
		       RDF_Resource arcLabel,
		       PRBool tv,
		       RDF_ValueType targetType,
		       nsIRDFCursor **targets /* out */);

  NS_METHOD Assert(RDF_Resource source, 
		   RDF_Resource arcLabel, 
		   RDF_Node target,
		   PRBool tv = PR_TRUE);

  NS_METHOD Unassert(RDF_Resource source,
		     RDF_Resource arcLabel,
		     RDF_Node target);

  NS_METHOD HasAssertion(RDF_Resource source,
			 RDF_Resource arcLabel,
			 RDF_Node target,
			 PRBool* hasAssertion /* out */);

  NS_METHOD HasAssertion(RDF_Resource source,
			 RDF_Resource arcLabel,
			 RDF_Node target,
			 PRBool& truthValue,
			 PRBool* hasAssertion /* out */);

  NS_METHOD AddObserver(nsIRDFObserver *n,
                         RDF_EventMask type = RDF_ANY_NOTIFY);

  NS_METHOD RemoveObserver(nsIRDFObserver *n,
			   RDF_EventMask = RDF_ANY_NOTIFY);

  NS_METHOD ArcLabelsIn(RDF_Node node,
			nsIRDFCursor **labels /* out */);

  NS_METHOD ArcLabelsOut(RDF_Resource source,
			 nsIRDFCursor **labels /* out */);

  NS_METHOD Flush();

  /* nsIRDFDataBase methods:   */
  NS_METHOD AddDataSource(nsIRDFDataSource* dataSource);

  NS_METHOD RemoveDataSource(nsIRDFDataSource* dataSource);

  NS_METHOD GetDataSource(RDF_String url,
			  nsIRDFDataSource **source /* out */ );

  NS_METHOD DeleteAllArcs(RDF_Resource resource);

private:
  RDF mRDF;

  PR_HashTable mpObserverMap;
};

class rdfCursorWrapper : public nsIRDFCursor {
public:
  NS_DECL_ISUPPORTS
  
  rdfCursorWrapper(RDF_Cursor*);
  virtual ~rdfCursorWrapper();

  NS_METHOD HasElements(PRBool& hasElements);

  NS_METHOD Next(RDF_NodeStruct& n);

private:
  RDF_Cursor mCursor;
};

class rdfServiceWrapper : public nsIRDFService {
public:
  NS_DECL_ISUPPORTS

  NS_METHOD CreateDatabase(RDF_String** url,
			   nsIRDFDatabase** db);

};

/*
  
  rdfDataBaseWrapper:

*/

NS_IMPL_ISUPPORTS( rdfDatabaseWrapper, NS_IRDFDATABASE_IID )

NS_METHOD
rdfDatabaseWrapper::GetName(const RDF_String* name /* out */ )
{
  PR_ASSERT( PR_FALSE );
  return NS_NOT_IMPLEMENTED; // XXX
}

NS_METHOD
rdfDatabaseWrapper::GetSource(RDF_Node target,
			      RDF_Resource arcLabel,
			      RDF_Resource *source /* out */)
{
  PR_ASSERT( target && source );
  *source = (RDF_Resource) RDF_GetSlotValue( mRDF,
					     target,
					     arcLabel,
					     RDF_RESOURCE_TYPE, // anything else makes no sense
					     PR_TRUE,
					     PR_TRUE );

  return NS_OK;
}

NS_METHOD
rdfDatabaseWrapper::GetSource(RDF_Node target,
                       RDF_Resource arcLabel,
                       PRBool tv,
                       RDF_Resource *source /* out */)
{
  *source = (RDF_Resource) RDF_GetSlotValue( mRDF,
					     target,
					     arcLabel,
					     RDF_RESOURCE_TYPE, // anything else makes no sense
					     PR_TRUE,
					     tv );

  return NS_OK;
}

NS_METHOD
rdfDatabaseWrapper::GetSources(RDF_Node target,
                               RDF_Resource arcLabel,
                               nsIRDFCursor **sources /* out */)
{
  return GetSources(target,arcLabel,PR_TRUE,sources);
}

NS_METHOD
rdfDatabaseWrapper::GetSources(RDF_Node target,
                               RDF_Resource arcLabel,
                               PRBool tv,
                               nsIRDFCursor **sources /* out */)
{
  PR_ASSERT( PR_FALSE ); 
  return NS_NOT_IMPLEMENTED; // XXX
}

NS_METHOD
rdfDatabaseWrapper::GetTarget(RDF_Resource source,
                              RDF_Resource arcLabel,
                              RDF_ValueType targetType,
                              RDF_NodeStruct& target /* in/out */)
{
  return GetTarget(source,arcLabel,targetType,PR_TRUE,target);
}

NS_METHOD
rdfDatabaseWrapper::GetTarget(RDF_Resource source,
                              RDF_Resource arcLabel,
                              RDF_ValueType targetType,
                              PRBool tv,
                              RDF_NodeStruct& target /* in/out */)
{
  PR_ASSERT( targetType != RDF_ANY_TYPE ); // not ready to support this yet

  void* value  = RDF_GetSlotValue( mRDF,
				   target,
				   arcLabel,
				   targetType, // anything else makes no sense
				   PR_FALSE,
				   tv );

  target.type = targetType;
  target.value.r = (RDF_Resource) value; // reasonable? XXX

  return NS_OK;
}

NS_METHOD
rdfDatabaseWrapper::GetTargets(RDF_Resource source,
                               RDF_Resource arcLabel,
                               RDF_ValueType targetType,
                               nsIRDFCursor **targets /* out */)
{
  return GetTargets(source,arcLabel,PR_TRUE,targetType,targets);
}

NS_METHOD
rdfDatabaseWrapper::GetTargets(RDF_Resource source,
                               RDF_Resource arcLabel,
                               PRBool tv,
                               RDF_ValueType targetType,
                               nsIRDFCursor **targets /* out */)
{
  PR_ASSERT( PR_FALSE ); 
  return NS_NOT_IMPLEMENTED; // XXX
}

NS_METHOD
rdfDatabaseWrapper::Assert(RDF_Resource source, 
                           RDF_Resource arcLabel, 
                           RDF_Node target,
                           PRBool tv = PR_TRUE)
{
  PRBool b = tv ? RDF_Assert( mRDF, source, arcLabel, (void*) target->value.r, target->type ) :
    RDF_AssertFalse( mRDF, sourcem arcLabel, (Void*) target->value.r, target->type );

  // XXX
  return NS_OK;
}

NS_METHOD
rdfDatabaseWrapper::Unassert(RDF_Resource source,
                             RDF_Resource arcLabel,
                             RDF_Node target)
{
  PRBool b = RDF_Unassert( mRDF, 
			   source, 
			   arcLabel, 
			   (void*) target->value.r, 
			   target->type ); // XXX

  return NS_OK;
}

NS_METHOD
rdfDatabaseWrapper::HasAssertion(RDF_Resource source,
				 RDF_Resource arcLabel,
				 RDF_Node target,
				 PRBool truthValue,
				 PRBool* hasAssertion /* out */)
{
  *hasAssertion = RDF_HasAssertion( mRDF,
				    source,
				    arcLabel,
				    (void*) target->value.r,
				    target->type,
				    truthValue );
  
  return NS_OK;
}

PR_IMPLEMENT(void)
_comwrap_NotificationCB(RDF_Event event, void* pdata)
{
  nsIRDFObserver* observer = (nsIRDFObserver*) pdata;
  // XXX QueryInterface & release??
  observer->HandleEvent( this, event );
}

NS_METHOD
rdfDatabaseWrapper::AddObserver(nsIRDFObserver *observer,
                                RDF_EventMask type = RDF_ANY_NOTIFY)
{
  // XXX event masking does not currently work

  RDF_Notification notification = PL_HashTableLookup( mpObserverMap, observer );
  if( !nofification ) {
    observer->AddRef();
    notification = RDF_AddNotifiable( mRDF,
                                      _comwrap_NotificationCB,
                                      NULL, // XXX
                                      observer );
    PL_HashTableAdd( mpObserverMap,
                     observer,
                     notification );
  }

  return NS_OK; // XXX
}

NS_METHOD
rdfDatabaseWrapper::RemoveObserver(nsIRDFObserver *n,
                            RDF_EventMask = RDF_ANY_NOTIFY)
{

  RDF_Notification notification = PL_HashTableLookup( mpObserverMap, observer );
  if( !notification )
    return NS_ERROR_INVALILD_PARAMETER;
  
  RDF_Error err = RDF_DeleteNotifiable( notification );
  PR_ASSERT( !err ); // the current implementation never fails!
  PL_HashTableRemove( mpObserverMap, observer );
  observer->AddRelease();
    
  return NS_OK; // XXX
}

NS_METHOD
rdfDatabaseWrapper::ArcLabelsIn(RDF_Node node,
                         nsIRDFCursor **labels /* out */)
{
  PR_ASSERT( PR_FALSE ); 
  return NS_NOT_IMPLEMENTED; // XXX
}

NS_METHOD
rdfDatabaseWrapper::ArcLabelsOut(RDF_Resource source,
                          nsIRDFCursor **labels /* out */)
{
  PR_ASSERT( PR_FALSE ); 
  return NS_NOT_IMPLEMENTED; // XXX
}

NS_METHOD
rdfDatabaseWrapper::Flush()
{
  return NS_NOT_IMPLEMENTED; // XXX
}

/*
  
  rdfServiceWrapper:  the RDF service singleton

*/

NS_IMPL_ISUPPORTS( rdfServiceWrapper, NS_IRDFSERVICE_IID )

NS_METHOD
rdfServiceWrapper::CreateDatabase(const RDF_String* url_ary,
				  nsIRDFDatabase **db)
{
  nsresult r = NS_OK;
  RDF rdf = RDF_GetDB(url_ary);
  if( 0 == rdf )
    r = RDF_ERROR_UNABLE_TO_CREATE_DATASOURCE; // XXX this is too wishy-washy

  return r;
}

/*

  rdfCursorWrapper

*/

NS_IMPL_ISUPPORTS( rdfCursorWrapper, NS_IRDFCURSOR_IID )

NS_METHOD
rdfCursorWrapper::Next(RDF_NodeStruct& next)
{

}
