/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
#include "nsCOMPtr.h"
#include "nsIModule.h"
#include "nsIGenericFactory.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIOService.h"
#include "nsNetModuleMgr.h"
#include "nsFileTransportService.h"
#include "nsSocketTransportService.h"
#include "nsSocketProviderService.h"
#include "nscore.h"
#include "nsStdURLParser.h"
#include "nsAuthURLParser.h"
#include "nsNoAuthURLParser.h"
#include "nsStdURL.h"
#include "nsSimpleURI.h"
#include "nsDnsService.h"
#include "nsLoadGroup.h"
#include "nsInputStreamChannel.h"
#include "nsStreamLoader.h"
#include "nsAsyncStreamListener.h"
#include "nsSyncStreamListener.h"
#include "nsFileStreams.h"
#include "nsBufferedStreams.h"
#include "nsProtocolProxyService.h"

///////////////////////////////////////////////////////////////////////////////

#include "nsStreamConverterService.h"

///////////////////////////////////////////////////////////////////////////////

#include "nsINetDataCache.h"
#include "nsINetDataCacheManager.h"
#include "nsMemCacheCID.h"
#include "nsMemCache.h"
#include "nsNetDiskCache.h"
#include "nsNetDiskCacheCID.h"
#include "nsCacheManager.h"

// Factory method to create a new nsMemCache instance.  Used
// by nsNetDataCacheModule
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsMemCache, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsNetDiskCache, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsCacheManager, Init)

///////////////////////////////////////////////////////////////////////////////

#include "nsMIMEService.h"
#include "nsXMLMIMEDataSource.h"
#include "nsMIMEInfoImpl.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsMIMEInfoImpl);

///////////////////////////////////////////////////////////////////////////////

#include "nsIHTTPProtocolHandler.h"
#include "nsHTTPHandler.h"
#include "nsHTTPSHandler.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsHTTPHandler, Init);
//NS_GENERIC_FACTORY_CONSTRUCTOR(nsHTTPSHandler);

#define NS_HTTPS_HANDLER_FACTORY_CID { 0xd2771480, 0xcac4, 0x11d3, { 0x8c, 0xaf, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74 } }

///////////////////////////////////////////////////////////////////////////////

#include "nsFileChannel.h"
#include "nsFileProtocolHandler.h"
#include "nsDataHandler.h"
#include "nsJARProtocolHandler.h"
#include "nsResProtocolHandler.h"

#include "nsAboutProtocolHandler.h"
#include "nsAboutBlank.h"
#include "nsAboutBloat.h"
#include "nsAboutCredits.h"
#include "mzAboutMozilla.h"
#include "nsKeywordProtocolHandler.h"

///////////////////////////////////////////////////////////////////////////////
// Module implementation for the net library

static nsModuleComponentInfo gNetModuleInfo[] = {
    { "I/O Service", 
      NS_IOSERVICE_CID,
      "component://netscape/network/io-service",
      nsIOService::Create },
    { "File Transport Service", 
      NS_FILETRANSPORTSERVICE_CID,
      "component://netscape/network/file-transport-service",
      nsFileTransportService::Create },
    { "Socket Transport Service", 
      NS_SOCKETTRANSPORTSERVICE_CID,
      "component://netscape/network/socket-transport-service", 
      nsSocketTransportService::Create },
    { "Socket Provider Service", 
      NS_SOCKETPROVIDERSERVICE_CID,
      "component://netscape/network/socket-provider-service",
      nsSocketProviderService::Create },
    { "DNS Service", 
      NS_DNSSERVICE_CID,
      "component://netscape/network/dns-service",
      nsDNSService::Create },
    { "Standard URL Implementation",
      NS_STANDARDURL_CID,
      "component://netscape/network/standard-url",
      nsStdURL::Create },
    { "Simple URI Implementation",
      NS_SIMPLEURI_CID,
      "component://netscape/network/simple-uri",
      nsSimpleURI::Create },
    { "External Module Manager", 
      NS_NETMODULEMGR_CID,
      "component://netscape/network/net-extern-mod",
      nsNetModuleMgr::Create },
    { NS_FILEIO_CLASSNAME,
      NS_FILEIO_CID,
      NS_FILEIO_PROGID,
      nsFileIO::Create },
    { NS_INPUTSTREAMIO_CLASSNAME,
      NS_INPUTSTREAMIO_CID,
      NS_INPUTSTREAMIO_PROGID,
      nsInputStreamIO::Create },
    { NS_STREAMIOCHANNEL_CLASSNAME,
      NS_STREAMIOCHANNEL_CID,
      NS_STREAMIOCHANNEL_PROGID,
      nsStreamIOChannel::Create },
    { "Unichar Stream Loader", 
      NS_STREAMLOADER_CID,
      "component://netscape/network/stream-loader",
      nsStreamLoader::Create },
    { "Async Stream Observer",
      NS_ASYNCSTREAMOBSERVER_CID,
      "component://netscape/network/async-stream-observer",
      nsAsyncStreamObserver::Create },
    { "Async Stream Listener",
      NS_ASYNCSTREAMLISTENER_CID,
      "component://netscape/network/async-stream-listener",
      nsAsyncStreamListener::Create },
    { "Sync Stream Listener", 
      NS_SYNCSTREAMLISTENER_CID,
      "component://netscape/network/sync-stream-listener",
      nsSyncStreamListener::Create },
    { "Load Group", 
      NS_LOADGROUP_CID,
      "component://netscape/network/load-group",
      nsLoadGroup::Create },
    { NS_LOCALFILEINPUTSTREAM_CLASSNAME, 
      NS_LOCALFILEINPUTSTREAM_CID,
      NS_LOCALFILEINPUTSTREAM_PROGID,
      nsFileInputStream::Create },
    { NS_LOCALFILEOUTPUTSTREAM_CLASSNAME, 
      NS_LOCALFILEOUTPUTSTREAM_CID,
      NS_LOCALFILEOUTPUTSTREAM_PROGID,
      nsFileOutputStream::Create },
    { "StdURLParser", 
      NS_STANDARDURLPARSER_CID,
      "component://netscape/network/standard-urlparser",
      nsStdURLParser::Create },
    { "AuthURLParser", 
      NS_AUTHORITYURLPARSER_CID,
      "component://netscape/network/authority-urlparser",
      nsAuthURLParser::Create },
    { "NoAuthURLParser", 
      NS_NOAUTHORITYURLPARSER_CID,
      "component://netscape/network/no-authority-urlparser",
      nsNoAuthURLParser::Create },
    { NS_BUFFEREDINPUTSTREAM_CLASSNAME, 
      NS_BUFFEREDINPUTSTREAM_CID,
      NS_BUFFEREDINPUTSTREAM_PROGID,
      nsBufferedInputStream::Create },
    { NS_BUFFEREDOUTPUTSTREAM_CLASSNAME, 
      NS_BUFFEREDOUTPUTSTREAM_CID,
      NS_BUFFEREDOUTPUTSTREAM_PROGID,
      nsBufferedOutputStream::Create },
    { "Protocol Proxy Service",
      NS_PROTOCOLPROXYSERVICE_CID,
      "component::/netscape/network/protocol-proxy-service",
      nsProtocolProxyService::Create },

    // from netwerk/streamconv:
    { "Stream Converter Service", 
      NS_STREAMCONVERTERSERVICE_CID,
      "component:||netscape|streamConverters", 
      nsStreamConverterService::Create },

    // from netwerk/cache:
    { "Memory Cache", NS_MEM_CACHE_FACTORY_CID, NS_NETWORK_MEMORY_CACHE_PROGID, nsMemCacheConstructor },
    { "File Cache",   NS_NETDISKCACHE_CID,      NS_NETWORK_FILE_CACHE_PROGID,   nsNetDiskCacheConstructor },
    { "Cache Manager",NS_CACHE_MANAGER_CID,     NS_NETWORK_CACHE_MANAGER_PROGID,nsCacheManagerConstructor },

    // from netwerk/mime:
    { "The MIME mapping service", 
      NS_MIMESERVICE_CID,
      "component:||netscape|mime",
      nsMIMEService::Create
    },
    { "xml mime datasource", 
      NS_XMLMIMEDATASOURCE_CID,
      NS_XMLMIMEDATASOURCE_PROGID,
      nsXMLMIMEDataSource::Create
    },
    { "xml mime INFO", 
      NS_MIMEINFO_CID,
      NS_MIMEINFO_PROGID,
      nsMIMEInfoImplConstructor
    },

    // from netwerk/protocol/file:
    { "File Protocol Handler", 
      NS_FILEPROTOCOLHANDLER_CID,  
      NS_NETWORK_PROTOCOL_PROGID_PREFIX "file", 
      nsFileProtocolHandler::Create
    },
    { NS_LOCALFILECHANNEL_CLASSNAME,
      NS_LOCALFILECHANNEL_CID,  
      NS_LOCALFILECHANNEL_PROGID, 
      nsFileChannel::Create
    },
    
    // from netwerk/protocol/http:
    { "HTTP Handler",
      NS_IHTTPHANDLER_CID,
      NS_NETWORK_PROTOCOL_PROGID_PREFIX "http",
      nsHTTPHandlerConstructor },
    { "HTTPS Handler",
      NS_HTTPS_HANDLER_FACTORY_CID,
      NS_NETWORK_PROTOCOL_PROGID_PREFIX "https",
      nsHTTPSHandler::Create },

    // from netwerk/protocol/data:
    { "Data Protocol Handler", 
      NS_DATAHANDLER_CID,
      NS_NETWORK_PROTOCOL_PROGID_PREFIX "data", 
      nsDataHandler::Create},

    // from netwerk/protocol/jar:
    { "JAR Protocol Handler", 
       NS_JARPROTOCOLHANDLER_CID,
       NS_NETWORK_PROTOCOL_PROGID_PREFIX "jar", 
       nsJARProtocolHandler::Create
    },

    // from netwerk/protocol/res:
    { "The Resource Protocol Handler", 
      NS_RESPROTOCOLHANDLER_CID,
      NS_NETWORK_PROTOCOL_PROGID_PREFIX "resource",
      nsResProtocolHandler::Create
    },

    // from netwerk/protocol/about:
    { "About Protocol Handler", 
      NS_ABOUTPROTOCOLHANDLER_CID,
      NS_NETWORK_PROTOCOL_PROGID_PREFIX "about", 
      nsAboutProtocolHandler::Create
    },
    { "about:blank", 
      NS_ABOUT_BLANK_MODULE_CID,
      NS_ABOUT_MODULE_PROGID_PREFIX "blank", 
      nsAboutBlank::Create
    },
    { "about:bloat", 
      NS_ABOUT_BLOAT_MODULE_CID,
      NS_ABOUT_MODULE_PROGID_PREFIX "bloat", 
      nsAboutBloat::Create
    },
    { "about:credits",
      NS_ABOUT_CREDITS_MODULE_CID,
      NS_ABOUT_MODULE_PROGID_PREFIX "credits",
      nsAboutCredits::Create
    },
    { "about:mozilla",
      MZ_ABOUT_MOZILLA_MODULE_CID,
      NS_ABOUT_MODULE_PROGID_PREFIX "mozilla",
      mzAboutMozilla::Create
    },

    // from netwerk/protocol/keyword:
    { "The Keyword Protocol Handler", 
      NS_KEYWORDPROTOCOLHANDLER_CID,
      NS_NETWORK_PROTOCOL_PROGID_PREFIX "keyword",
      nsKeywordProtocolHandler::Create
    }
};

NS_IMPL_NSGETMODULE("necko core and primary protocols", gNetModuleInfo)
