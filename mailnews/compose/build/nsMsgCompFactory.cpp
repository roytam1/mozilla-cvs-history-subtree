/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include "msgCore.h"

#include "nsISupports.h"
#include "nsCOMPtr.h"

#include "nsIFactory.h"
#include "nsIGenericFactory.h"
#include "nsIServiceManager.h"
#include "nsIModule.h"

#include "pratom.h"
#include "nsMsgCompCID.h"



/* Include all of the interfaces our factory can generate components for */
#include "nsMsgSendFact.h"
#include "nsMsgCompFieldsFact.h"
#include "nsMsgSendLaterFact.h"

#include "nsMsgComposeFact.h"
#include "nsMsgSendLater.h"
#include "nsSmtpUrl.h"
#include "nsISmtpService.h"
#include "nsSmtpService.h"
#include "nsMsgComposeService.h"
#include "nsMsgCompose.h"
#include "nsMsgSend.h"
#include "nsMsgQuote.h"
#include "nsIMsgDraft.h"
#include "nsMsgCreate.h"    // For drafts...I know, awful file name...
#include "nsSmtpServer.h"




static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);
static NS_DEFINE_CID(kCMsgComposeCID, NS_MSGCOMPOSE_CID);
static NS_DEFINE_CID(kCMsgCompFieldsCID, NS_MSGCOMPFIELDS_CID);
static NS_DEFINE_CID(kCMsgSendCID, NS_MSGSEND_CID);
static NS_DEFINE_CID(kCMsgSendLaterCID, NS_MSGSENDLATER_CID);
static NS_DEFINE_CID(kCSmtpServiceCID, NS_SMTPSERVICE_CID);
static NS_DEFINE_CID(kSmtpServerCID, NS_SMTPSERVER_CID);
static NS_DEFINE_CID(kCMsgComposeServiceCID, NS_MSGCOMPOSESERVICE_CID);
static NS_DEFINE_CID(kCMsgQuoteCID, NS_MSGQUOTE_CID);
static NS_DEFINE_CID(kCSmtpUrlCID, NS_SMTPURL_CID);
static NS_DEFINE_CID(kMsgDraftCID, NS_MSGDRAFT_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(nsSmtpService);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSmtpServer);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMsgCompose);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMsgCompFields);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMsgComposeAndSend);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMsgSendLater);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMsgDraft)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMsgComposeService);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMsgQuote);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSmtpUrl);

////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////
static PRInt32 g_InstanceCount = 0;
static PRInt32 g_LockCount = 0;

struct components_t {
  nsCID cid;
  nsIGenericFactory::ConstructorProcPtr constructor;
  const char *progid;
  const char *description;
};

static components_t components[] =
{
  { NS_MSGCOMPOSE_CID,        &nsMsgComposeConstructor,    },
  { NS_MSGCOMPOSESERVICE_CID, &nsMsgComposeServiceConstructor, },
  { NS_MSGCOMPFIELDS_CID,     &nsMsgCompFieldsConstructor, },
  { NS_MSGDRAFT_CID,          &nsMsgDraftConstructor, },
  { NS_MSGSEND_CID,           &nsMsgComposeAndSendConstructor, },
  { NS_MSGSENDLATER_CID,      &nsMsgSendLaterConstructor, },
  { NS_SMTPSERVICE_CID,       &nsSmtpServiceConstructor,  },
  { NS_SMTPSERVER_CID,        &nsSmtpServerConstructor,    },
  { NS_SMTPURL_CID,           &nsSmtpUrlConstructor, },
  { NS_MSGQUOTE_CID,          &nsMsgQuoteConstructor, },
};

NS_DECL_MODULE(nsModule)
NS_IMPL_MODULE_CORE(nsModule)
NS_IMPL_ISUPPORTS1(nsModule, nsIModule)
NS_IMPL_MODULE_GETCLASSOBJECT(nsModule, components)
NS_IMPL_MODULE_REGISTERSELF(nsModule, components)
NS_IMPL_MODULE_UNREGISTERSELF(nsModule, components)
NS_IMPL_NSGETMODULE(nsModule)
NS_IMPL_MODULE_CANUNLOAD(nsModule)
  
#if 0
	nsCOMPtr<nsIServiceManager> servMgr(do_QueryInterface(aServMgr, &rv));
	if (NS_FAILED(rv)) return rv;

	NS_WITH_SERVICE1(nsIComponentManager, compMgr, aServMgr, kComponentManagerCID, &rv);
	if (NS_FAILED(rv)) return rv;

	// register the message compose factory
	rv = compMgr->RegisterComponent(kCSmtpServiceCID,
										"SMTP Service", nsnull,
										path, PR_TRUE, PR_TRUE);
	if (NS_FAILED(rv)) finalResult = rv;
  
	rv = compMgr->RegisterComponent(kSmtpServerCID,
                                  "SMTP Server",
                                  NS_SMTPSERVER_PROGID,
                                  path, PR_TRUE, PR_TRUE);
	if (NS_FAILED(rv)) finalResult = rv;
	
	rv = compMgr->RegisterComponent(kCSmtpUrlCID,
										"Smtp url",
										nsnull,
										path, PR_TRUE, PR_TRUE);
	if (NS_FAILED(rv)) finalResult = rv;

	rv = compMgr->RegisterComponent(kCMsgComposeServiceCID,
                                  "Message Compose Service",
                                  NS_MSGCOMPOSESERVICE_PROGID,
                                  path, PR_TRUE, PR_TRUE);
	if (NS_FAILED(rv)) finalResult = rv;

	rv = compMgr->RegisterComponent(kCMsgComposeCID,
										"Message Compose",
										"component://netscape/messengercompose/compose",
										path, PR_TRUE, PR_TRUE);
	if (NS_FAILED(rv)) finalResult = rv;

	rv = compMgr->RegisterComponent(kCMsgCompFieldsCID,
										"Message Compose Fields",
										"component://netscape/messengercompose/composefields",
										path, PR_TRUE, PR_TRUE);
	if (NS_FAILED(rv)) finalResult = rv;

	rv = compMgr->RegisterComponent(kCMsgSendCID,
										"Message Compose Send",
										"component://netscape/messengercompose/send",
										path, PR_TRUE, PR_TRUE);
	if (NS_FAILED(rv)) finalResult = rv;

    rv = compMgr->RegisterComponent(kCMsgSendLaterCID,
										"Message Compose Send Later",
										"component://netscape/messengercompose/sendlater",
										path, PR_TRUE, PR_TRUE);
	if (NS_FAILED(rv)) finalResult = rv;

    rv = compMgr->RegisterComponent(kCSmtpServiceCID,
										"Message Compose SMTP Service",
										"component://netscape/messengercompose/smtp",
										path, PR_TRUE, PR_TRUE);
	if (NS_FAILED(rv)) finalResult = rv;

	rv = compMgr->RegisterComponent(kCSmtpServiceCID,  
                                    "SMTP Protocol Handler",
                                    NS_NETWORK_PROTOCOL_PROGID_PREFIX "mailto",
                                    path, PR_TRUE, PR_TRUE);

	if (NS_FAILED(rv)) finalResult = rv;

  
    rv = compMgr->RegisterComponent(kCMsgQuoteCID,
										"Message Quoting",
										"component://netscape/messengercompose/quoting",
										path, PR_TRUE, PR_TRUE);
	if (NS_FAILED(rv)) finalResult = rv;

  // For Drafts...
  rv = compMgr->RegisterComponent(kMsgDraftCID,
										"Message Drafts",
										"component://netscape/messengercompose/drafts",
										path, PR_TRUE, PR_TRUE);
	if (NS_FAILED(rv)) finalResult = rv;

  return finalResult;
}

extern "C" NS_EXPORT nsresult
NSUnregisterSelf(nsISupports* aServMgr, const char* path)
{
	nsresult finalResult = NS_OK;
	nsresult rv = NS_OK;

	NS_WITH_SERVICE1(nsIComponentManager, compMgr, aServMgr, kComponentManagerCID, &rv);
	if (NS_FAILED(rv)) return rv;

	rv = compMgr->UnregisterComponent(kCMsgComposeServiceCID, path);
	if (NS_FAILED(rv))finalResult = rv;

	rv = compMgr->UnregisterComponent(kCMsgComposeCID, path);
	if (NS_FAILED(rv))finalResult = rv;

	rv = compMgr->UnregisterComponent(kCMsgCompFieldsCID, path);
	if (NS_FAILED(rv)) finalResult = rv;

	rv = compMgr->UnregisterComponent(kCMsgSendCID, path);
	if (NS_FAILED(rv)) finalResult = rv;

	rv = compMgr->UnregisterComponent(kCMsgSendLaterCID, path);
	if (NS_FAILED(rv)) finalResult = rv;

	rv = compMgr->UnregisterComponent(kCSmtpServiceCID, path);
	if (NS_FAILED(rv)) finalResult = rv;

	rv = compMgr->UnregisterComponent(kSmtpServerCID, path);
	if (NS_FAILED(rv)) finalResult = rv;

	rv = compMgr->UnregisterComponent(kCSmtpUrlCID, path);
	if (NS_FAILED(rv)) finalResult = rv;

	rv = compMgr->UnregisterComponent(kCMsgQuoteCID, path);
	if (NS_FAILED(rv)) finalResult = rv;

  rv = compMgr->UnregisterComponent(kMsgDraftCID, path);
	if (NS_FAILED(rv)) finalResult = rv;

	return finalResult;
}
#endif
