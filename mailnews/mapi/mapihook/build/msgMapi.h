/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Mon Dec 17 16:52:12 2001
 */
/* Compiler settings for msgMapi.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __msgMapi_h__
#define __msgMapi_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __nsIMapi_FWD_DEFINED__
#define __nsIMapi_FWD_DEFINED__
typedef interface nsIMapi nsIMapi;
#endif 	/* __nsIMapi_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_msgMapi_0000 */
/* [local] */ 

typedef wchar_t __RPC_FAR LOGIN_PW_TYPE[ 256 ];

typedef /* [public] */ struct  __MIDL___MIDL_itf_msgMapi_0000_0001
    {
    unsigned long ulReserved;
    unsigned long flFlags;
    unsigned long nPosition_NotUsed;
    LPTSTR lpszPathName;
    LPTSTR lpszFileName;
    unsigned char __RPC_FAR *lpFileType_NotUsed;
    }	nsMapiFileDesc;

typedef struct __MIDL___MIDL_itf_msgMapi_0000_0001 __RPC_FAR *lpnsMapiFileDesc;

typedef /* [public] */ struct  __MIDL___MIDL_itf_msgMapi_0000_0002
    {
    unsigned long ulReserved;
    unsigned long ulRecipClass;
    LPTSTR lpszName;
    LPTSTR lpszAddress;
    unsigned long ulEIDSize_NotUsed;
    unsigned char __RPC_FAR *lpEntryID_NotUsed;
    }	nsMapiRecipDesc;

typedef struct __MIDL___MIDL_itf_msgMapi_0000_0002 __RPC_FAR *lpnsMapiRecipDesc;

typedef /* [public] */ struct  __MIDL___MIDL_itf_msgMapi_0000_0003
    {
    unsigned long ulReserved;
    LPTSTR lpszSubject;
    LPTSTR lpszNoteText;
    LPTSTR lpszMessageType_NotUsed;
    LPTSTR lpszDateReceived_notUsed;
    LPTSTR lpszConversationID_NotUsed;
    unsigned long flFlags;
    lpnsMapiRecipDesc lpOriginator;
    unsigned long nRecipCount;
    lpnsMapiRecipDesc lpRecips;
    unsigned long nFileCount;
    lpnsMapiFileDesc lpFiles;
    }	nsMapiMessage;

typedef struct __MIDL___MIDL_itf_msgMapi_0000_0003 __RPC_FAR *lpnsMapiMessage;



extern RPC_IF_HANDLE __MIDL_itf_msgMapi_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msgMapi_0000_v0_0_s_ifspec;

#ifndef __nsIMapi_INTERFACE_DEFINED__
#define __nsIMapi_INTERFACE_DEFINED__

/* interface nsIMapi */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_nsIMapi;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6EDCD38E-8861-11d5-A3DD-00B0D0F3BAA7")
    nsIMapi : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Login( 
            unsigned long aUIArg,
            LOGIN_PW_TYPE aLogin,
            LOGIN_PW_TYPE aPassWord,
            unsigned long aFlags,
            /* [out] */ unsigned long __RPC_FAR *aSessionId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Initialize( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsValid( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsValidSession( 
            /* [in] */ unsigned long aSession) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SendMail( 
            /* [in] */ unsigned long aSession,
            /* [in] */ lpnsMapiMessage aMessage,
            /* [in] */ short aRecipCount,
            /* [size_is][in] */ lpnsMapiRecipDesc aRecips,
            /* [in] */ short aFileCount,
            /* [size_is][in] */ lpnsMapiFileDesc aFiles,
            /* [in] */ unsigned long aFlags,
            /* [in] */ unsigned long aReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SendDocuments( 
            /* [in] */ unsigned long aSession,
            /* [in] */ LPTSTR aDelimChar,
            /* [in] */ LPTSTR aFilePaths,
            /* [in] */ LPTSTR aFileNames,
            /* [in] */ ULONG aFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Logoff( 
            unsigned long aSession) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CleanUp( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct nsIMapiVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            nsIMapi __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            nsIMapi __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            nsIMapi __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Login )( 
            nsIMapi __RPC_FAR * This,
            unsigned long aUIArg,
            LOGIN_PW_TYPE aLogin,
            LOGIN_PW_TYPE aPassWord,
            unsigned long aFlags,
            /* [out] */ unsigned long __RPC_FAR *aSessionId);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Initialize )( 
            nsIMapi __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsValid )( 
            nsIMapi __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsValidSession )( 
            nsIMapi __RPC_FAR * This,
            /* [in] */ unsigned long aSession);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendMail )( 
            nsIMapi __RPC_FAR * This,
            /* [in] */ unsigned long aSession,
            /* [in] */ lpnsMapiMessage aMessage,
            /* [in] */ short aRecipCount,
            /* [size_is][in] */ lpnsMapiRecipDesc aRecips,
            /* [in] */ short aFileCount,
            /* [size_is][in] */ lpnsMapiFileDesc aFiles,
            /* [in] */ unsigned long aFlags,
            /* [in] */ unsigned long aReserved);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendDocuments )( 
            nsIMapi __RPC_FAR * This,
            /* [in] */ unsigned long aSession,
            /* [in] */ LPTSTR aDelimChar,
            /* [in] */ LPTSTR aFilePaths,
            /* [in] */ LPTSTR aFileNames,
            /* [in] */ ULONG aFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Logoff )( 
            nsIMapi __RPC_FAR * This,
            unsigned long aSession);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CleanUp )( 
            nsIMapi __RPC_FAR * This);
        
        END_INTERFACE
    } nsIMapiVtbl;

    interface nsIMapi
    {
        CONST_VTBL struct nsIMapiVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define nsIMapi_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define nsIMapi_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define nsIMapi_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define nsIMapi_Login(This,aUIArg,aLogin,aPassWord,aFlags,aSessionId)	\
    (This)->lpVtbl -> Login(This,aUIArg,aLogin,aPassWord,aFlags,aSessionId)

#define nsIMapi_Initialize(This)	\
    (This)->lpVtbl -> Initialize(This)

#define nsIMapi_IsValid(This)	\
    (This)->lpVtbl -> IsValid(This)

#define nsIMapi_IsValidSession(This,aSession)	\
    (This)->lpVtbl -> IsValidSession(This,aSession)

#define nsIMapi_SendMail(This,aSession,aMessage,aRecipCount,aRecips,aFileCount,aFiles,aFlags,aReserved)	\
    (This)->lpVtbl -> SendMail(This,aSession,aMessage,aRecipCount,aRecips,aFileCount,aFiles,aFlags,aReserved)

#define nsIMapi_SendDocuments(This,aSession,aDelimChar,aFilePaths,aFileNames,aFlags)	\
    (This)->lpVtbl -> SendDocuments(This,aSession,aDelimChar,aFilePaths,aFileNames,aFlags)

#define nsIMapi_Logoff(This,aSession)	\
    (This)->lpVtbl -> Logoff(This,aSession)

#define nsIMapi_CleanUp(This)	\
    (This)->lpVtbl -> CleanUp(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE nsIMapi_Login_Proxy( 
    nsIMapi __RPC_FAR * This,
    unsigned long aUIArg,
    LOGIN_PW_TYPE aLogin,
    LOGIN_PW_TYPE aPassWord,
    unsigned long aFlags,
    /* [out] */ unsigned long __RPC_FAR *aSessionId);


void __RPC_STUB nsIMapi_Login_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE nsIMapi_Initialize_Proxy( 
    nsIMapi __RPC_FAR * This);


void __RPC_STUB nsIMapi_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE nsIMapi_IsValid_Proxy( 
    nsIMapi __RPC_FAR * This);


void __RPC_STUB nsIMapi_IsValid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE nsIMapi_IsValidSession_Proxy( 
    nsIMapi __RPC_FAR * This,
    /* [in] */ unsigned long aSession);


void __RPC_STUB nsIMapi_IsValidSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE nsIMapi_SendMail_Proxy( 
    nsIMapi __RPC_FAR * This,
    /* [in] */ unsigned long aSession,
    /* [in] */ lpnsMapiMessage aMessage,
    /* [in] */ short aRecipCount,
    /* [size_is][in] */ lpnsMapiRecipDesc aRecips,
    /* [in] */ short aFileCount,
    /* [size_is][in] */ lpnsMapiFileDesc aFiles,
    /* [in] */ unsigned long aFlags,
    /* [in] */ unsigned long aReserved);


void __RPC_STUB nsIMapi_SendMail_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE nsIMapi_SendDocuments_Proxy( 
    nsIMapi __RPC_FAR * This,
    /* [in] */ unsigned long aSession,
    /* [in] */ LPTSTR aDelimChar,
    /* [in] */ LPTSTR aFilePaths,
    /* [in] */ LPTSTR aFileNames,
    /* [in] */ ULONG aFlags);


void __RPC_STUB nsIMapi_SendDocuments_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE nsIMapi_Logoff_Proxy( 
    nsIMapi __RPC_FAR * This,
    unsigned long aSession);


void __RPC_STUB nsIMapi_Logoff_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE nsIMapi_CleanUp_Proxy( 
    nsIMapi __RPC_FAR * This);


void __RPC_STUB nsIMapi_CleanUp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __nsIMapi_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
