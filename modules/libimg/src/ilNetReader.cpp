/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include "if.h"
#if defined (COLORSYNC)
#include "icc_profile.h"
#endif /* (COLORSYNC) */
#include "ilINetReader.h"

static NS_DEFINE_IID(kINetReaderIID, IL_INETREADER_IID);

class NetReaderImpl : public ilINetReader {
public:

#if defined (COLORSYNC)
  NetReaderImpl(il_container *aContainer, ip_container *iccContainer);
#else
  NetReaderImpl(il_container *aContainer);
#endif /* (COLORSYNC) */

  ~NetReaderImpl();

  NS_DECL_ISUPPORTS

  virtual unsigned int WriteReady();
  
  virtual int FirstWrite(const unsigned char *str, int32 len);

  virtual int Write(const unsigned char *str, int32 len);

  virtual void StreamAbort(int status);

  virtual void StreamComplete(PRBool is_multipart);

  virtual void NetRequestDone(ilIURL *urls, int status);
  
  virtual PRBool StreamCreated(ilIURL *urls, int type);
  
  virtual PRBool IsMulti();

  NetReaderContainerType GetContainer();

private:
  il_container *mContainer;
#if defined (COLORSYNC)
  ip_container *mICCContainer;
#endif /* (COLORSYNC) */
};

#if defined (COLORSYNC)
NetReaderImpl::NetReaderImpl(il_container *aContainer, ip_container *iccContainer)
#else
NetReaderImpl::NetReaderImpl(il_container *aContainer)
#endif /* (COLORSYNC) */
{
    NS_INIT_REFCNT();
    mContainer		= aContainer;
#if defined (COLORSYNC)
    mICCContainer	= iccContainer;
#endif /* (COLORSYNC) */
}

NetReaderImpl::~NetReaderImpl()
{
}

NS_IMPL_ISUPPORTS(NetReaderImpl, kINetReaderIID)

unsigned int 
NetReaderImpl::WriteReady()
{
    if (mContainer != NULL) {
        return IL_StreamWriteReady(mContainer);
    }
#if defined (COLORSYNC)
    else if (mICCContainer != NULL) {
	/*
		Profile stream doesn't have custom function.
		Just return a std size.  We don't want this to
		be small, because we're going to allocate
		a pointer the first time through, and we
		don't want to have to do alot of reallocs.
	*/
		#define	kProfileChunkSize	8192
		return kProfileChunkSize;
    }
#endif /* (COLORSYNC) */
	return 0;
}
  
int 
NetReaderImpl::FirstWrite(const unsigned char *str, int32 len)
{
    if (mContainer != NULL) {
        return IL_StreamFirstWrite(mContainer, str, len);
    }
#if defined (COLORSYNC)
	else if (mICCContainer != NULL) {
		return IL_ProfileStreamFirstWrite(mICCContainer, str, len);
	}
#endif /* (COLORSYNC) */
	
	return 0;
}

int 
NetReaderImpl::Write(const unsigned char *str, int32 len)
{
    if (mContainer != NULL) {
        return IL_StreamWrite(mContainer, str, len);
    }
#if defined (COLORSYNC)
    else if (mICCContainer != NULL) {
        return IL_ProfileStreamWrite(mICCContainer, str, len);
    }
#endif /* (COLORSYNC) */

	return 0;
}

void 
NetReaderImpl::StreamAbort(int status)
{
    if (mContainer != NULL) {
        IL_StreamAbort(mContainer, status);
    }
#if defined (COLORSYNC)
    else if (mICCContainer != NULL) {
        IL_ProfileStreamAbort(mICCContainer, status);
    }
#endif /* (COLORSYNC) */
}

void 
NetReaderImpl::StreamComplete(PRBool is_multipart)
{
    if (mContainer != NULL) {
        IL_StreamComplete(mContainer, is_multipart);
    }
#if defined (COLORSYNC)
    else if (mICCContainer != NULL) {
        IL_ProfileStreamComplete(mICCContainer);
    }
#endif /* (COLORSYNC) */
}

void 
NetReaderImpl::NetRequestDone(ilIURL *urls, int status)
{
    if (mContainer != NULL) {
        IL_NetRequestDone(mContainer, urls, status);
    }
#if defined (COLORSYNC)
    else if (mICCContainer != NULL) {
        IL_NetProfileRequestDone(mICCContainer, urls, status);
    }
#endif /* (COLORSYNC) */
}
  
PRBool 
NetReaderImpl::StreamCreated(ilIURL *urls, int type)
{
    if (mContainer != NULL) {
        return IL_StreamCreated(mContainer, urls, type);
    }
#if defined (COLORSYNC)
    else if (mICCContainer != NULL) {
        return IL_ProfileStreamCreated(mICCContainer, urls, type);
    }
#endif /* (COLORSYNC) */

	return PR_FALSE;
}
  
PRBool 
NetReaderImpl::IsMulti()
{
    if (mContainer != NULL) {
        return (PRBool)(mContainer->multi > 0);
    }
    else {
        return PR_FALSE;
    }
}

NetReaderContainerType
NetReaderImpl::GetContainer()
{
	NetReaderContainerType	container = NULL;
	
	if (mContainer != NULL)
		container = (NetReaderContainerType) mContainer;
#if defined (COLORSYNC)
	else if (mICCContainer != NULL)
		container = (NetReaderContainerType) mICCContainer;
#endif /* (COLORSYNC) */
	return container;
}

ilINetReader *
#if defined (COLORSYNC)
IL_NewNetReader(il_container *ic, ip_container *ip)
#else
IL_NewNetReader(il_container *ic)
#endif /* (COLORSYNC) */
{
#if defined (COLORSYNC)
    ilINetReader *reader = new NetReaderImpl(ic,ip);
#else
    ilINetReader *reader = new NetReaderImpl(ic);
#endif /* (COLORSYNC) */

    if (reader != NULL) {
        NS_ADDREF(reader);
    }

    return reader;
}

NetReaderContainerType
IL_GetNetReaderContainer(ilINetReader *reader)
{
    if (reader != NULL) {
        return ((NetReaderImpl *)reader)->GetContainer();
    }
    else {
        return NULL;
    }
}
