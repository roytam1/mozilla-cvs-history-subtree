/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#include "nsIGenericFactory.h"
#include "nsDOMSerializer.h"
#include "nsXMLHttpRequest.h"
#include "nsDOMParser.h"
#include "nsSOAPParameter.h"
#include "nsSOAPCall.h"
#include "nsDefaultSOAPEncoder.h"
#include "nsHTTPSOAPTransport.h"
#include "nsString.h"
#include "prprf.h"
#include "nsIScriptNameSpaceManager.h"


////////////////////////////////////////////////////////////////////////
// Define the contructor function for the objects
//
// NOTE: This creates an instance of objects by using the default constructor
//
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDOMSerializer)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsXMLHttpRequest)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDOMParser)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSOAPCall)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSOAPParameter)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDefaultSOAPEncoder)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsHTTPSOAPTransport)


static NS_METHOD 
RegisterXMLExtras(nsIComponentManager *aCompMgr,
                  nsIFile *aPath,
                  const char *registryLocation,
                  const char *componentType)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsICategoryManager> catman =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);

  if (NS_FAILED(rv))
    return rv;

  nsXPIDLCString previous;
  rv = catman->AddCategoryEntry(JAVASCRIPT_GLOBAL_CONSTRUCTOR_CATEGORY,
                                "XMLSerializer",
                                NS_XMLSERIALIZER_CONTRACTID,
                                PR_TRUE, PR_TRUE, getter_Copies(previous));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = catman->AddCategoryEntry(JAVASCRIPT_GLOBAL_CONSTRUCTOR_CATEGORY,
                                "XMLSerializer",
                                NS_XMLSERIALIZER_CONTRACTID,
                                PR_TRUE, PR_TRUE, getter_Copies(previous));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = catman->AddCategoryEntry(JAVASCRIPT_GLOBAL_CONSTRUCTOR_CATEGORY,
                                "XMLHttpRequest",
                                NS_XMLHTTPREQUEST_CONTRACTID,
                                PR_TRUE, PR_TRUE, getter_Copies(previous));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = catman->AddCategoryEntry(JAVASCRIPT_GLOBAL_CONSTRUCTOR_CATEGORY,
                                "DOMParser",
                                NS_DOMPARSER_CONTRACTID,
                                PR_TRUE, PR_TRUE, getter_Copies(previous));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = catman->AddCategoryEntry(JAVASCRIPT_GLOBAL_CONSTRUCTOR_CATEGORY,
                                "SOAPCall",
                                NS_SOAPCALL_CONTRACTID,
                                PR_TRUE, PR_TRUE, getter_Copies(previous));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = catman->AddCategoryEntry(JAVASCRIPT_GLOBAL_CONSTRUCTOR_CATEGORY,
                                "SOAPParameter",
                                NS_SOAPPARAMETER_CONTRACTID,
                                PR_TRUE, PR_TRUE, getter_Copies(previous));
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}

////////////////////////////////////////////////////////////////////////
// Define a table of CIDs implemented by this module along with other
// information like the function to create an instance, contractid, and
// class name.
//
static nsModuleComponentInfo components[] = {
  { "XML Serializer", NS_XMLSERIALIZER_CID, NS_XMLSERIALIZER_CONTRACTID,
    nsDOMSerializerConstructor, RegisterXMLExtras },
  { "XMLHttpRequest", NS_XMLHTTPREQUEST_CID, NS_XMLHTTPREQUEST_CONTRACTID,
    nsXMLHttpRequestConstructor },
  { "DOM Parser", NS_DOMPARSER_CID, NS_DOMPARSER_CONTRACTID,
    nsDOMParserConstructor },
  { "SOAP Call", NS_SOAPCALL_CID, NS_SOAPCALL_CONTRACTID,
    nsSOAPCallConstructor },
  { "SOAP Parameter", NS_SOAPPARAMETER_CID, NS_SOAPPARAMETER_CONTRACTID,
    nsSOAPParameterConstructor },
  { "Default SOAP Encoder", NS_DEFAULTSOAPENCODER_CID,
    NS_DEFAULTSOAPENCODER_CONTRACTID, nsDefaultSOAPEncoderConstructor },
  { "HTTP SOAP Transport", NS_HTTPSOAPTRANSPORT_CID,
    NS_HTTPSOAPTRANSPORT_CONTRACTID, nsHTTPSOAPTransportConstructor },
};

NS_IMPL_NSGETMODULE("nsXMLExtrasModule", components)
