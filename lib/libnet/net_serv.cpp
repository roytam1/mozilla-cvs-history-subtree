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

#include "net_serv.h"
#include "net_strm.h"
#include "net.h"
#include "mktrace.h"

/* XXX: Legacy definitions... */
MWContext *new_stub_context();
void free_stub_context(MWContext *window_id);
void bam_exit_routine(URL_Struct *URL_s, int status, MWContext *window_id);

extern "C" {
extern char *XP_AppCodeName;
extern char *XP_AppVersion;
};


nsNetlibService::nsNetlibService()
{
    NS_INIT_REFCNT();

    m_stubContext = new_stub_context();

    /* Initialize netlib with 32 sockets... */
    NET_InitNetLib(0, 32);


    /* XXX: How should the User Agent get initialized? */
    XP_AppCodeName = strdup("Mozilla");
    XP_AppVersion = strdup("1.0");

}


NS_DEFINE_IID(kINetServiceIID, NS_INETSERVICE_IID);
NS_IMPL_ISUPPORTS(nsNetlibService,kINetServiceIID);


nsNetlibService::~nsNetlibService()
{
    TRACEMSG(("nsNetlibService is being destroyed...\n"));

    if (NULL == m_stubContext) {
        free_stub_context((MWContext *)m_stubContext);
    }

    NET_ShutdownNetLib();
}


NS_IMETHODIMP nsNetlibService::OpenStream(const char *aUrl, 
                                          nsIStreamNotification *aConsumer)
{
    URL_Struct *URL_s;
    nsConnectionInfo *pConn;

    /* Create a new dummy context for the URL load... */
    if ((NULL == m_stubContext) || (NULL == aConsumer)) {
        return NS_FALSE;
    }

    /* Create the nsConnectionInfo object... */
    pConn = new nsConnectionInfo(NULL, aConsumer);
    if (NULL == pConn) {
        return NS_FALSE;
    }
    pConn->AddRef();

    /* Create the URLStruct... */
    URL_s = NET_CreateURLStruct(aUrl, NET_NORMAL_RELOAD);
    if (NULL == URL_s) {
        pConn->Release();
        return NS_FALSE;
    }

    /* 
     * Mark the URL as background loading.  This prevents many
     * client upcall notifications...
     */
    URL_s->load_background = TRUE;

    /*
     * Attach the Data Consumer to the URL_Struct.
     *
     * Both the Data Consumer and the URL_Struct are freed in the 
     * bam_exit_routine(...)
     */
    URL_s->fe_data = pConn;
    pConn->AddRef();

    /* Start the URL load... */
    NET_GetURL (URL_s,                      /* URL_Struct      */
                FO_CACHE_AND_PRESENT,       /* FO_Present_type */
                (MWContext *)m_stubContext, /* MWContext       */
                bam_exit_routine);          /* Exit routine... */

    /* Remember, the URL_s may have been freed ! */

    return NS_OK;
}


NS_IMETHODIMP nsNetlibService::OpenBlockingStream(const char *aUrl, 
                                              nsIStreamNotification *aConsumer,
                                              nsIInputStream **aNewStream)
{
    URL_Struct *URL_s;
    nsConnectionInfo *pConn;

    if (NULL != aNewStream) {
        *aNewStream = NULL;

        /* Create the nsConnectionInfo object... */
        pConn = new nsConnectionInfo(NULL, aConsumer);
        if (NULL == pConn) {
            return NS_FALSE;
        }
        pConn->AddRef();

        /* Create the blocking stream... */
        pConn->pNetStream = new nsBlockingStream();
        if (NULL == pConn->pNetStream) {
            pConn->Release();
        }
        pConn->pNetStream->AddRef();

        *aNewStream = pConn->pNetStream;
        pConn->pNetStream->AddRef();

        /* Create the URLStruct... */
        URL_s = NET_CreateURLStruct(aUrl, NET_NORMAL_RELOAD);
        if (NULL == URL_s) {
            pConn->pNetStream->Release();
            pConn->Release();
            return NS_FALSE;
        }

        /* 
         * Mark the URL as background loading.  This prevents many
         * client upcall notifications...
         */
        URL_s->load_background = TRUE;

        /*
         * Attach the Data Consumer to the URL_Struct.
         *
         * Both the Data Consumer and the URL_Struct are freed in the 
         * bam_exit_routine(...)
         */
        URL_s->fe_data = pConn;

        /* Start the URL load... */
        NET_GetURL (URL_s,                      /* URL_Struct      */
                    FO_CACHE_AND_PRESENT,       /* FO_Present_type */
                    (MWContext *)m_stubContext, /* MWContext       */
                    bam_exit_routine);          /* Exit routine... */

        /* Remember, the URL_s may have been freed ! */

        return NS_OK;
    }

    return NS_FALSE;
}


extern "C" {
/*
 * Factory for creating instance of the NetlibService...
 */
/* NS_BASE */ nsresult NS_NewINetService(nsINetService** aInstancePtrResult,
                                         nsISupports* aOuter)
{
    static nsNetlibService *pNetlib = NULL;

    if (NULL != aOuter) {
        return NS_ERROR_NO_AGGREGATION;
    }

    /* XXX: For now only allow a single instance of the Netlib Service */
    if (NULL == pNetlib) {
        pNetlib = new nsNetlibService();
    }

    if (NULL == pNetlib) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    return pNetlib->QueryInterface(kINetServiceIID, (void**)aInstancePtrResult);
}

}; /* extern "C" */


/*
 * This is the generic exit routine for all URLs loaded via the new
 * BAM APIs...
 */
static void bam_exit_routine(URL_Struct *URL_s, int status, MWContext *window_id)
{
    TRACEMSG(("bam_exit_routine was called...\n"));

    if (NULL != URL_s) {
        /* Release the nsIstreamNotification object held in the URL_Struct. */
        nsIStreamNotification *pConsumer = (nsIStreamNotification *)URL_s->fe_data;
        pConsumer->Release();

        /* Delete the URL_Struct... */
        NET_FreeURLStruct(URL_s);
    }
}

