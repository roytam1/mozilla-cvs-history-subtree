/* this ALWAYS GENERATED file contains the proxy stub code */


/* File created by MIDL compiler version 5.01.0164 */
/* at Mon Dec 17 16:52:12 2001
 */
/* Compiler settings for msgMapi.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcproxy.h> version is high enough to compile this file*/
#ifndef __REDQ_RPCPROXY_H_VERSION__
#define __REQUIRED_RPCPROXY_H_VERSION__ 440
#endif


#include "rpcproxy.h"
#ifndef __RPCPROXY_H_VERSION__
#error this stub requires an updated version of <rpcproxy.h>
#endif // __RPCPROXY_H_VERSION__


#include "msgMapi.h"

#define TYPE_FORMAT_STRING_SIZE   313                               
#define PROC_FORMAT_STRING_SIZE   67                                

typedef struct _MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } MIDL_TYPE_FORMAT_STRING;

typedef struct _MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } MIDL_PROC_FORMAT_STRING;


extern const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString;
extern const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString;


/* Standard interface: __MIDL_itf_msgMapi_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: nsIMapi, ver. 0.0,
   GUID={0x6EDCD38E,0x8861,0x11d5,{0xA3,0xDD,0x00,0xB0,0xD0,0xF3,0xBA,0xA7}} */


extern const MIDL_STUB_DESC Object_StubDesc;


#pragma code_seg(".orpc")

HRESULT STDMETHODCALLTYPE nsIMapi_Login_Proxy( 
    nsIMapi __RPC_FAR * This,
    unsigned long aUIArg,
    LOGIN_PW_TYPE aLogin,
    LOGIN_PW_TYPE aPassWord,
    unsigned long aFlags,
    /* [out] */ unsigned long __RPC_FAR *aSessionId)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      3);
        
        
        
        if(!aSessionId)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U + 1024U + 1026U + 10U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *(( unsigned long __RPC_FAR * )_StubMsg.Buffer)++ = aUIArg;
            
            NdrFixedArrayMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                   (unsigned char __RPC_FAR *)aLogin,
                                   (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[2] );
            
            NdrFixedArrayMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                   (unsigned char __RPC_FAR *)aPassWord,
                                   (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[2] );
            
            *(( unsigned long __RPC_FAR * )_StubMsg.Buffer)++ = aFlags;
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[0] );
            
            *aSessionId = *(( unsigned long __RPC_FAR * )_StubMsg.Buffer)++;
            
            _RetVal = *(( HRESULT __RPC_FAR * )_StubMsg.Buffer)++;
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        NdrClearOutParameters(
                         ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                         ( PFORMAT_STRING  )&__MIDL_TypeFormatString.Format[8],
                         ( void __RPC_FAR * )aSessionId);
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB nsIMapi_Login_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    unsigned long _M0;
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    unsigned long aFlags;
    LOGIN_PW_TYPE __RPC_FAR *aLogin;
    LOGIN_PW_TYPE __RPC_FAR *aPassWord;
    unsigned long __RPC_FAR *aSessionId;
    unsigned long aUIArg;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    aLogin = 0;
    aPassWord = 0;
    ( unsigned long __RPC_FAR * )aSessionId = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[0] );
        
        aUIArg = *(( unsigned long __RPC_FAR * )_StubMsg.Buffer)++;
        
        NdrFixedArrayUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                 (unsigned char __RPC_FAR * __RPC_FAR *)&aLogin,
                                 (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[2],
                                 (unsigned char)0 );
        
        NdrFixedArrayUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                 (unsigned char __RPC_FAR * __RPC_FAR *)&aPassWord,
                                 (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[2],
                                 (unsigned char)0 );
        
        aFlags = *(( unsigned long __RPC_FAR * )_StubMsg.Buffer)++;
        
        aSessionId = &_M0;
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((nsIMapi*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> Login(
         (nsIMapi *) ((CStdStubBuffer *)This)->pvServerObject,
         aUIArg,
         *aLogin,
         *aPassWord,
         aFlags,
         aSessionId);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U + 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *(( unsigned long __RPC_FAR * )_StubMsg.Buffer)++ = *aSessionId;
        
        *(( HRESULT __RPC_FAR * )_StubMsg.Buffer)++ = _RetVal;
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE nsIMapi_Initialize_Proxy( 
    nsIMapi __RPC_FAR * This)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      4);
        
        
        
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[18] );
            
            _RetVal = *(( HRESULT __RPC_FAR * )_StubMsg.Buffer)++;
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB nsIMapi_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    RpcTryFinally
        {
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((nsIMapi*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> Initialize((nsIMapi *) ((CStdStubBuffer *)This)->pvServerObject);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *(( HRESULT __RPC_FAR * )_StubMsg.Buffer)++ = _RetVal;
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE nsIMapi_IsValid_Proxy( 
    nsIMapi __RPC_FAR * This)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      5);
        
        
        
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[18] );
            
            _RetVal = *(( HRESULT __RPC_FAR * )_StubMsg.Buffer)++;
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB nsIMapi_IsValid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    RpcTryFinally
        {
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((nsIMapi*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> IsValid((nsIMapi *) ((CStdStubBuffer *)This)->pvServerObject);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *(( HRESULT __RPC_FAR * )_StubMsg.Buffer)++ = _RetVal;
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE nsIMapi_IsValidSession_Proxy( 
    nsIMapi __RPC_FAR * This,
    /* [in] */ unsigned long aSession)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      6);
        
        
        
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *(( unsigned long __RPC_FAR * )_StubMsg.Buffer)++ = aSession;
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[20] );
            
            _RetVal = *(( HRESULT __RPC_FAR * )_StubMsg.Buffer)++;
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB nsIMapi_IsValidSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    unsigned long aSession;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[20] );
        
        aSession = *(( unsigned long __RPC_FAR * )_StubMsg.Buffer)++;
        
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((nsIMapi*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> IsValidSession((nsIMapi *) ((CStdStubBuffer *)This)->pvServerObject,aSession);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *(( HRESULT __RPC_FAR * )_StubMsg.Buffer)++ = _RetVal;
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE nsIMapi_SendMail_Proxy( 
    nsIMapi __RPC_FAR * This,
    /* [in] */ unsigned long aSession,
    /* [in] */ lpnsMapiMessage aMessage,
    /* [in] */ short aRecipCount,
    /* [size_is][in] */ lpnsMapiRecipDesc aRecips,
    /* [in] */ short aFileCount,
    /* [size_is][in] */ lpnsMapiFileDesc aFiles,
    /* [in] */ unsigned long aFlags,
    /* [in] */ unsigned long aReserved)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      7);
        
        
        
        if(!aMessage)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!aRecips)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!aFiles)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U + 4U + 5U + 7U + 4U + 7U + 7U + 7U;
            NdrSimpleStructBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR *)aMessage,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[104] );
            
            _StubMsg.MaxCount = aRecipCount;
            
            NdrConformantArrayBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                          (unsigned char __RPC_FAR *)aRecips,
                                          (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[208] );
            
            _StubMsg.MaxCount = aFileCount;
            
            NdrConformantArrayBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                          (unsigned char __RPC_FAR *)aFiles,
                                          (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[260] );
            
            NdrProxyGetBuffer(This, &_StubMsg);
            *(( unsigned long __RPC_FAR * )_StubMsg.Buffer)++ = aSession;
            
            NdrSimpleStructMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                     (unsigned char __RPC_FAR *)aMessage,
                                     (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[104] );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 1) & ~ 0x1);
            *(( short __RPC_FAR * )_StubMsg.Buffer)++ = aRecipCount;
            
            _StubMsg.MaxCount = aRecipCount;
            
            NdrConformantArrayMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                        (unsigned char __RPC_FAR *)aRecips,
                                        (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[208] );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 1) & ~ 0x1);
            *(( short __RPC_FAR * )_StubMsg.Buffer)++ = aFileCount;
            
            _StubMsg.MaxCount = aFileCount;
            
            NdrConformantArrayMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                        (unsigned char __RPC_FAR *)aFiles,
                                        (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[260] );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            *(( unsigned long __RPC_FAR * )_StubMsg.Buffer)++ = aFlags;
            
            *(( unsigned long __RPC_FAR * )_StubMsg.Buffer)++ = aReserved;
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[24] );
            
            _RetVal = *(( HRESULT __RPC_FAR * )_StubMsg.Buffer)++;
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB nsIMapi_SendMail_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    short aFileCount;
    lpnsMapiFileDesc aFiles;
    unsigned long aFlags;
    lpnsMapiMessage aMessage;
    short aRecipCount;
    lpnsMapiRecipDesc aRecips;
    unsigned long aReserved;
    unsigned long aSession;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    ( lpnsMapiMessage  )aMessage = 0;
    ( lpnsMapiRecipDesc  )aRecips = 0;
    ( lpnsMapiFileDesc  )aFiles = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[24] );
        
        aSession = *(( unsigned long __RPC_FAR * )_StubMsg.Buffer)++;
        
        NdrSimpleStructUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR * __RPC_FAR *)&aMessage,
                                   (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[104],
                                   (unsigned char)0 );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 1) & ~ 0x1);
        aRecipCount = *(( short __RPC_FAR * )_StubMsg.Buffer)++;
        
        NdrConformantArrayUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&aRecips,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[208],
                                      (unsigned char)0 );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 1) & ~ 0x1);
        aFileCount = *(( short __RPC_FAR * )_StubMsg.Buffer)++;
        
        NdrConformantArrayUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                      (unsigned char __RPC_FAR * __RPC_FAR *)&aFiles,
                                      (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[260],
                                      (unsigned char)0 );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        aFlags = *(( unsigned long __RPC_FAR * )_StubMsg.Buffer)++;
        
        aReserved = *(( unsigned long __RPC_FAR * )_StubMsg.Buffer)++;
        
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((nsIMapi*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> SendMail(
            (nsIMapi *) ((CStdStubBuffer *)This)->pvServerObject,
            aSession,
            aMessage,
            aRecipCount,
            aRecips,
            aFileCount,
            aFiles,
            aFlags,
            aReserved);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U;
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *(( HRESULT __RPC_FAR * )_StubMsg.Buffer)++ = _RetVal;
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)aMessage,
                        &__MIDL_TypeFormatString.Format[12] );
        
        _StubMsg.MaxCount = aRecipCount;
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)aRecips,
                        &__MIDL_TypeFormatString.Format[204] );
        
        _StubMsg.MaxCount = aFileCount;
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)aFiles,
                        &__MIDL_TypeFormatString.Format[256] );
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE nsIMapi_SendDocuments_Proxy( 
    nsIMapi __RPC_FAR * This,
    /* [in] */ unsigned long aSession,
    /* [in] */ LPTSTR aDelimChar,
    /* [in] */ LPTSTR aFilePaths,
    /* [in] */ LPTSTR aFileNames,
    /* [in] */ ULONG aFlags)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      8);
        
        
        
        if(!aDelimChar)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!aFilePaths)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!aFileNames)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U + 12U + 14U + 14U + 10U;
            NdrConformantStringBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                           (unsigned char __RPC_FAR *)aDelimChar,
                                           (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[310] );
            
            NdrConformantStringBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                           (unsigned char __RPC_FAR *)aFilePaths,
                                           (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[310] );
            
            NdrConformantStringBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                           (unsigned char __RPC_FAR *)aFileNames,
                                           (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[310] );
            
            NdrProxyGetBuffer(This, &_StubMsg);
            *(( unsigned long __RPC_FAR * )_StubMsg.Buffer)++ = aSession;
            
            NdrConformantStringMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                         (unsigned char __RPC_FAR *)aDelimChar,
                                         (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[310] );
            
            NdrConformantStringMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                         (unsigned char __RPC_FAR *)aFilePaths,
                                         (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[310] );
            
            NdrConformantStringMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                         (unsigned char __RPC_FAR *)aFileNames,
                                         (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[310] );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            *(( ULONG __RPC_FAR * )_StubMsg.Buffer)++ = aFlags;
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[48] );
            
            _RetVal = *(( HRESULT __RPC_FAR * )_StubMsg.Buffer)++;
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB nsIMapi_SendDocuments_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    LPTSTR aDelimChar;
    LPTSTR aFileNames;
    LPTSTR aFilePaths;
    ULONG aFlags;
    unsigned long aSession;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    ( LPTSTR  )aDelimChar = 0;
    ( LPTSTR  )aFilePaths = 0;
    ( LPTSTR  )aFileNames = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[48] );
        
        aSession = *(( unsigned long __RPC_FAR * )_StubMsg.Buffer)++;
        
        NdrConformantStringUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR * __RPC_FAR *)&aDelimChar,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[310],
                                       (unsigned char)0 );
        
        NdrConformantStringUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR * __RPC_FAR *)&aFilePaths,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[310],
                                       (unsigned char)0 );
        
        NdrConformantStringUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR * __RPC_FAR *)&aFileNames,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[310],
                                       (unsigned char)0 );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        aFlags = *(( ULONG __RPC_FAR * )_StubMsg.Buffer)++;
        
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((nsIMapi*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> SendDocuments(
                 (nsIMapi *) ((CStdStubBuffer *)This)->pvServerObject,
                 aSession,
                 aDelimChar,
                 aFilePaths,
                 aFileNames,
                 aFlags);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U;
        _StubMsg.BufferLength += 16;
        
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *(( HRESULT __RPC_FAR * )_StubMsg.Buffer)++ = _RetVal;
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE nsIMapi_Logoff_Proxy( 
    nsIMapi __RPC_FAR * This,
    unsigned long aSession)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      9);
        
        
        
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 4U;
            NdrProxyGetBuffer(This, &_StubMsg);
            *(( unsigned long __RPC_FAR * )_StubMsg.Buffer)++ = aSession;
            
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[20] );
            
            _RetVal = *(( HRESULT __RPC_FAR * )_StubMsg.Buffer)++;
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB nsIMapi_Logoff_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    unsigned long aSession;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[20] );
        
        aSession = *(( unsigned long __RPC_FAR * )_StubMsg.Buffer)++;
        
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((nsIMapi*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> Logoff((nsIMapi *) ((CStdStubBuffer *)This)->pvServerObject,aSession);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *(( HRESULT __RPC_FAR * )_StubMsg.Buffer)++ = _RetVal;
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


HRESULT STDMETHODCALLTYPE nsIMapi_CleanUp_Proxy( 
    nsIMapi __RPC_FAR * This)
{

    HRESULT _RetVal;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        NdrProxyInitialize(
                      ( void __RPC_FAR *  )This,
                      ( PRPC_MESSAGE  )&_RpcMessage,
                      ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                      ( PMIDL_STUB_DESC  )&Object_StubDesc,
                      10);
        
        
        
        RpcTryFinally
            {
            
            _StubMsg.BufferLength = 0U;
            NdrProxyGetBuffer(This, &_StubMsg);
            NdrProxySendReceive(This, &_StubMsg);
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[18] );
            
            _RetVal = *(( HRESULT __RPC_FAR * )_StubMsg.Buffer)++;
            
            }
        RpcFinally
            {
            NdrProxyFreeBuffer(This, &_StubMsg);
            
            }
        RpcEndFinally
        
        }
    RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)
        {
        _RetVal = NdrProxyErrorHandler(RpcExceptionCode());
        }
    RpcEndExcept
    return _RetVal;
}

void __RPC_STUB nsIMapi_CleanUp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    HRESULT _RetVal;
    MIDL_STUB_MESSAGE _StubMsg;
    
NdrStubInitialize(
                     _pRpcMessage,
                     &_StubMsg,
                     &Object_StubDesc,
                     _pRpcChannelBuffer);
    RpcTryFinally
        {
        
        *_pdwStubPhase = STUB_CALL_SERVER;
        _RetVal = (((nsIMapi*) ((CStdStubBuffer *)This)->pvServerObject)->lpVtbl) -> CleanUp((nsIMapi *) ((CStdStubBuffer *)This)->pvServerObject);
        
        *_pdwStubPhase = STUB_MARSHAL;
        
        _StubMsg.BufferLength = 4U;
        NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);
        *(( HRESULT __RPC_FAR * )_StubMsg.Buffer)++ = _RetVal;
        
        }
    RpcFinally
        {
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


static const MIDL_STUB_DESC Object_StubDesc = 
    {
    0,
    NdrOleAllocate,
    NdrOleFree,
    0,
    0,
    0,
    0,
    0,
    __MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x10001, /* Ndr library version */
    0,
    0x50100a4, /* MIDL Version 5.1.164 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    1,  /* Flags */
    0,  /* Reserved3 */
    0,  /* Reserved4 */
    0   /* Reserved5 */
    };

const CINTERFACE_PROXY_VTABLE(11) _nsIMapiProxyVtbl = 
{
    &IID_nsIMapi,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    nsIMapi_Login_Proxy ,
    nsIMapi_Initialize_Proxy ,
    nsIMapi_IsValid_Proxy ,
    nsIMapi_IsValidSession_Proxy ,
    nsIMapi_SendMail_Proxy ,
    nsIMapi_SendDocuments_Proxy ,
    nsIMapi_Logoff_Proxy ,
    nsIMapi_CleanUp_Proxy
};


static const PRPC_STUB_FUNCTION nsIMapi_table[] =
{
    nsIMapi_Login_Stub,
    nsIMapi_Initialize_Stub,
    nsIMapi_IsValid_Stub,
    nsIMapi_IsValidSession_Stub,
    nsIMapi_SendMail_Stub,
    nsIMapi_SendDocuments_Stub,
    nsIMapi_Logoff_Stub,
    nsIMapi_CleanUp_Stub
};

const CInterfaceStubVtbl _nsIMapiStubVtbl =
{
    &IID_nsIMapi,
    0,
    11,
    &nsIMapi_table[-3],
    CStdStubBuffer_METHODS
};

#pragma data_seg(".rdata")

#if !defined(__RPC_WIN32__)
#error  Invalid build platform for this stub.
#endif

static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
    {
        0,
        {
			0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/*  2 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/*  4 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */
/*  6 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/*  8 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */
/* 10 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 12 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 14 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */
/* 16 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 18 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 20 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 22 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 24 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 26 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 28 */	NdrFcShort( 0xc ),	/* Type Offset=12 */
/* 30 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x6,		/* FC_SHORT */
/* 32 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 34 */	NdrFcShort( 0xcc ),	/* Type Offset=204 */
/* 36 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x6,		/* FC_SHORT */
/* 38 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 40 */	NdrFcShort( 0x100 ),	/* Type Offset=256 */
/* 42 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 44 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 46 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 48 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 50 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 52 */	NdrFcShort( 0x134 ),	/* Type Offset=308 */
/* 54 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 56 */	NdrFcShort( 0x134 ),	/* Type Offset=308 */
/* 58 */	
			0x4d,		/* FC_IN_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/* 60 */	NdrFcShort( 0x134 ),	/* Type Offset=308 */
/* 62 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 64 */	0x53,		/* FC_RETURN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */

			0x0
        }
    };

static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x1d,		/* FC_SMFARRAY */
			0x1,		/* 1 */
/*  4 */	NdrFcShort( 0x200 ),	/* 512 */
/*  6 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/*  8 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 10 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 12 */	
			0x11, 0x0,	/* FC_RP */
/* 14 */	NdrFcShort( 0x5a ),	/* Offset= 90 (104) */
/* 16 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 18 */	NdrFcShort( 0x18 ),	/* 24 */
/* 20 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 22 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 24 */	NdrFcShort( 0x8 ),	/* 8 */
/* 26 */	NdrFcShort( 0x8 ),	/* 8 */
/* 28 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 30 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 32 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 34 */	NdrFcShort( 0xc ),	/* 12 */
/* 36 */	NdrFcShort( 0xc ),	/* 12 */
/* 38 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 40 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 42 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 44 */	NdrFcShort( 0x14 ),	/* 20 */
/* 46 */	NdrFcShort( 0x14 ),	/* 20 */
/* 48 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 50 */	0x2,		/* FC_CHAR */
			0x5c,		/* FC_PAD */
/* 52 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 54 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 56 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 58 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 60 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 62 */	NdrFcShort( 0x18 ),	/* 24 */
/* 64 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 66 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 68 */	NdrFcShort( 0xc ),	/* 12 */
/* 70 */	NdrFcShort( 0xc ),	/* 12 */
/* 72 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 74 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 76 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 78 */	NdrFcShort( 0x10 ),	/* 16 */
/* 80 */	NdrFcShort( 0x10 ),	/* 16 */
/* 82 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 84 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 86 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 88 */	NdrFcShort( 0x14 ),	/* 20 */
/* 90 */	NdrFcShort( 0x14 ),	/* 20 */
/* 92 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 94 */	0x2,		/* FC_CHAR */
			0x5c,		/* FC_PAD */
/* 96 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 98 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 100 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 102 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 104 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 106 */	NdrFcShort( 0x30 ),	/* 48 */
/* 108 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 110 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 112 */	NdrFcShort( 0x4 ),	/* 4 */
/* 114 */	NdrFcShort( 0x4 ),	/* 4 */
/* 116 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 118 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 120 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 122 */	NdrFcShort( 0x8 ),	/* 8 */
/* 124 */	NdrFcShort( 0x8 ),	/* 8 */
/* 126 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 128 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 130 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 132 */	NdrFcShort( 0xc ),	/* 12 */
/* 134 */	NdrFcShort( 0xc ),	/* 12 */
/* 136 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 138 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 140 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 142 */	NdrFcShort( 0x10 ),	/* 16 */
/* 144 */	NdrFcShort( 0x10 ),	/* 16 */
/* 146 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 148 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 150 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 152 */	NdrFcShort( 0x14 ),	/* 20 */
/* 154 */	NdrFcShort( 0x14 ),	/* 20 */
/* 156 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 158 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 160 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 162 */	NdrFcShort( 0x1c ),	/* 28 */
/* 164 */	NdrFcShort( 0x1c ),	/* 28 */
/* 166 */	0x12, 0x0,	/* FC_UP */
/* 168 */	NdrFcShort( 0xffffff68 ),	/* Offset= -152 (16) */
/* 170 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 172 */	NdrFcShort( 0x24 ),	/* 36 */
/* 174 */	NdrFcShort( 0x24 ),	/* 36 */
/* 176 */	0x12, 0x0,	/* FC_UP */
/* 178 */	NdrFcShort( 0xffffff5e ),	/* Offset= -162 (16) */
/* 180 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 182 */	NdrFcShort( 0x2c ),	/* 44 */
/* 184 */	NdrFcShort( 0x2c ),	/* 44 */
/* 186 */	0x12, 0x0,	/* FC_UP */
/* 188 */	NdrFcShort( 0xffffff80 ),	/* Offset= -128 (60) */
/* 190 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 192 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 194 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 196 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 198 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 200 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 202 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 204 */	
			0x11, 0x0,	/* FC_RP */
/* 206 */	NdrFcShort( 0x2 ),	/* Offset= 2 (208) */
/* 208 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 210 */	NdrFcShort( 0x18 ),	/* 24 */
/* 212 */	0x26,		/* Corr desc:  parameter, FC_SHORT */
			0x0,		/*  */
#ifndef _ALPHA_
/* 214 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 216 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 218 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 220 */	NdrFcShort( 0x18 ),	/* 24 */
/* 222 */	NdrFcShort( 0x0 ),	/* 0 */
/* 224 */	NdrFcShort( 0x3 ),	/* 3 */
/* 226 */	NdrFcShort( 0x8 ),	/* 8 */
/* 228 */	NdrFcShort( 0x8 ),	/* 8 */
/* 230 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 232 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 234 */	NdrFcShort( 0xc ),	/* 12 */
/* 236 */	NdrFcShort( 0xc ),	/* 12 */
/* 238 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 240 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 242 */	NdrFcShort( 0x14 ),	/* 20 */
/* 244 */	NdrFcShort( 0x14 ),	/* 20 */
/* 246 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 248 */	0x2,		/* FC_CHAR */
			0x5c,		/* FC_PAD */
/* 250 */	
			0x5b,		/* FC_END */

			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 252 */	0x0,		/* 0 */
			NdrFcShort( 0xffffff13 ),	/* Offset= -237 (16) */
			0x5b,		/* FC_END */
/* 256 */	
			0x11, 0x0,	/* FC_RP */
/* 258 */	NdrFcShort( 0x2 ),	/* Offset= 2 (260) */
/* 260 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 262 */	NdrFcShort( 0x18 ),	/* 24 */
/* 264 */	0x26,		/* Corr desc:  parameter, FC_SHORT */
			0x0,		/*  */
#ifndef _ALPHA_
/* 266 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 268 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 270 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 272 */	NdrFcShort( 0x18 ),	/* 24 */
/* 274 */	NdrFcShort( 0x0 ),	/* 0 */
/* 276 */	NdrFcShort( 0x3 ),	/* 3 */
/* 278 */	NdrFcShort( 0xc ),	/* 12 */
/* 280 */	NdrFcShort( 0xc ),	/* 12 */
/* 282 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 284 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 286 */	NdrFcShort( 0x10 ),	/* 16 */
/* 288 */	NdrFcShort( 0x10 ),	/* 16 */
/* 290 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 292 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 294 */	NdrFcShort( 0x14 ),	/* 20 */
/* 296 */	NdrFcShort( 0x14 ),	/* 20 */
/* 298 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 300 */	0x2,		/* FC_CHAR */
			0x5c,		/* FC_PAD */
/* 302 */	
			0x5b,		/* FC_END */

			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 304 */	0x0,		/* 0 */
			NdrFcShort( 0xffffff0b ),	/* Offset= -245 (60) */
			0x5b,		/* FC_END */
/* 308 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 310 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */

			0x0
        }
    };

const CInterfaceProxyVtbl * _msgMapi_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_nsIMapiProxyVtbl,
    0
};

const CInterfaceStubVtbl * _msgMapi_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_nsIMapiStubVtbl,
    0
};

PCInterfaceName const _msgMapi_InterfaceNamesList[] = 
{
    "nsIMapi",
    0
};


#define _msgMapi_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _msgMapi, pIID, n)

int __stdcall _msgMapi_IID_Lookup( const IID * pIID, int * pIndex )
{
    
    if(!_msgMapi_CHECK_IID(0))
        {
        *pIndex = 0;
        return 1;
        }

    return 0;
}

const ExtendedProxyFileInfo msgMapi_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _msgMapi_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _msgMapi_StubVtblList,
    (const PCInterfaceName * ) & _msgMapi_InterfaceNamesList,
    0, // no delegation
    & _msgMapi_IID_Lookup, 
    1,
    1,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};
