/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *      Christopher Seawood <cls@seawood.org>
 */

#define XPCOM_TRANSLATE_NSGM_ENTRY_POINT 1

#include "nsIGenericFactory.h" 

#include "nsStaticComponent.h"

#define MODULE(name) { #name "_NSGetModule", NSGETMODULE_ENTRY_POINT(name) }
#define DECL_MODULE(name)                                      \
extern "C" nsresult                                            \
NSGETMODULE_ENTRY_POINT(name) (nsIComponentManager *aCompMgr,  \
                               nsIFile *aLocation,             \
                               nsIModule **aResult)

//@DECLARE_COMPONENTS@
DECL_MODULE(xpconnect);
DECL_MODULE(JS_component_loader);
//DECL_MODULE(nsGtkTimerModule);
DECL_MODULE(cacheservice);
DECL_MODULE(necko_core_and_primary_protocols);
DECL_MODULE(necko_secondary_protocols);
DECL_MODULE(nsURILoaderModule);
DECL_MODULE(nsUConvModule);
DECL_MODULE(nsUCvJAModule);
DECL_MODULE(nsUCvCnModule);
DECL_MODULE(nsUCvLatinModule);
DECL_MODULE(nsUCvTWModule);
DECL_MODULE(nsUCvTW2Module);
DECL_MODULE(nsUCvKoModule);
DECL_MODULE(nsUCvIBMModule);
DECL_MODULE(UcharUtil);
DECL_MODULE(nsLocaleModule);
DECL_MODULE(nsStringBundleModule);
DECL_MODULE(nsLWBrkModule);
DECL_MODULE(nsCharDetModule);
DECL_MODULE(nsPrefModule);
DECL_MODULE(nsGIFModule);
DECL_MODULE(nsPNGModule);
DECL_MODULE(nsMNGModule);
DECL_MODULE(nsJPGModule);
DECL_MODULE(nsCJVMManagerModule);
DECL_MODULE(nsJarModule);
DECL_MODULE(nsSecurityManagerModule);
DECL_MODULE(nsChromeModule);
DECL_MODULE(nsRDFModule);
DECL_MODULE(nsParserModule);
//DECL_MODULE(nsGfxPSModule);
//DECL_MODULE(nsGfxGTKModule);
DECL_MODULE(nsGfxModule);
DECL_MODULE(nsGfx2Module);
DECL_MODULE(nsImageLib2Module);
// DECL_MODULE(nsPPMDecoderModule);
DECL_MODULE(nsPNGDecoderModule);
DECL_MODULE(nsGIFModule2);
DECL_MODULE(nsJPEGDecoderModule);
DECL_MODULE(nsPluginModule);
DECL_MODULE(javascript__protocol);
DECL_MODULE(DOM_components);
DECL_MODULE(nsViewModule);
DECL_MODULE(nsWidgetModule);
//DECL_MODULE(nsWidgetGTKModule);
//DECL_MODULE(XRemoteClientModule);
DECL_MODULE(nsContentModule);
DECL_MODULE(nsLayoutModule);
DECL_MODULE(nsMorkModule);
DECL_MODULE(docshell_provider);
DECL_MODULE(embedcomponents);
DECL_MODULE(Browser_Embedding_Module);
DECL_MODULE(nsTransactionManagerModule);
DECL_MODULE(nsEditorModule);
DECL_MODULE(nsTextServicesModule);
DECL_MODULE(nsProfileModule);
DECL_MODULE(nsPrefMigrationModule);
DECL_MODULE(nsAccessibilityModule);
DECL_MODULE(appshell);
DECL_MODULE(nsFindComponent);
DECL_MODULE(nsRegistryViewerModule);
DECL_MODULE(nsStreamTransferModule);
DECL_MODULE(Session_History_Module);
DECL_MODULE(application);
DECL_MODULE(nsBrowserModule);
DECL_MODULE(nsSoftwareUpdate);
DECL_MODULE(nsCookieModule);
DECL_MODULE(nsWalletModule);
DECL_MODULE(nsWalletViewerModule);
DECL_MODULE(nsXMLExtrasModule);
DECL_MODULE(nsMsgBaseModule);
DECL_MODULE(nsMsgDBModule);
DECL_MODULE(nsMsgNewsModule);
DECL_MODULE(local_mail_services);
DECL_MODULE(nsMimeEmitterModule);
DECL_MODULE(nsVCardModule);
DECL_MODULE(nsSMIMEModule);
DECL_MODULE(mime_services);
DECL_MODULE(nsMsgComposeModule);
DECL_MODULE(IMAP_factory);
DECL_MODULE(nsAbModule);
DECL_MODULE(nsImportServiceModule);
DECL_MODULE(nsTextImportModule);
DECL_MODULE(nsAbSyncModule);
DECL_MODULE(nsLDAPProtocolModule);

static nsStaticModuleInfo StaticModuleInfo[] = {
//      @COMPONENT_LIST@
    MODULE(xpconnect),
    MODULE(JS_component_loader),
//    MODULE(nsGtkTimerModule),
    MODULE(cacheservice),
    MODULE(necko_core_and_primary_protocols),
    MODULE(necko_secondary_protocols),
    MODULE(nsURILoaderModule),
    MODULE(nsUConvModule),
    MODULE(nsUCvJAModule),
    MODULE(nsUCvCnModule),
    MODULE(nsUCvLatinModule),
    MODULE(nsUCvTWModule),
    MODULE(nsUCvTW2Module),
    MODULE(nsUCvKoModule),
    MODULE(nsUCvIBMModule),
    MODULE(UcharUtil),
    MODULE(nsLocaleModule),
    MODULE(nsStringBundleModule),
    MODULE(nsLWBrkModule),
    MODULE(nsCharDetModule),
    MODULE(nsPrefModule),
    MODULE(nsGIFModule),
    MODULE(nsPNGModule),
    MODULE(nsMNGModule),
    MODULE(nsJPGModule),
    MODULE(nsCJVMManagerModule),
    MODULE(nsJarModule),
    MODULE(nsSecurityManagerModule),
    MODULE(nsChromeModule),
    MODULE(nsRDFModule),
    MODULE(nsParserModule),
//  MODULE(nsGfxPSModule),
    MODULE(nsGfxModule),
    MODULE(nsGfx2Module),
    MODULE(nsImageLib2Module),
//    MODULE(nsPPMDecoderModule),
    MODULE(nsPNGDecoderModule),
    MODULE(nsGIFModule2),
    MODULE(nsJPEGDecoderModule),
    MODULE(nsPluginModule),
    MODULE(javascript__protocol),
    MODULE(DOM_components),
    MODULE(nsViewModule),
    MODULE(nsWidgetModule),
//  MODULE(nsWidgetGTKModule),
//    MODULE(XRemoteClientModule),
    MODULE(nsContentModule),
    MODULE(nsLayoutModule),
    MODULE(nsMorkModule),
    MODULE(docshell_provider),
    MODULE(embedcomponents),
    MODULE(Browser_Embedding_Module),
    MODULE(nsTransactionManagerModule),
    MODULE(nsEditorModule),
    MODULE(nsTextServicesModule),
    MODULE(nsProfileModule),
    MODULE(nsPrefMigrationModule),
    MODULE(nsAccessibilityModule),
    MODULE(appshell),
    MODULE(nsFindComponent),
    MODULE(nsRegistryViewerModule),
    MODULE(nsStreamTransferModule),
    MODULE(Session_History_Module),
    MODULE(application),
    MODULE(nsBrowserModule),
    MODULE(nsSoftwareUpdate),
    MODULE(nsCookieModule),
    MODULE(nsWalletModule),
    MODULE(nsWalletViewerModule),
    MODULE(nsXMLExtrasModule),
    MODULE(nsMsgBaseModule),
    MODULE(nsMsgDBModule),
    MODULE(nsMsgNewsModule),
    MODULE(local_mail_services),
    MODULE(nsMimeEmitterModule),
    MODULE(nsVCardModule),
    MODULE(nsSMIMEModule),
    MODULE(mime_services),
    MODULE(nsMsgComposeModule),
    MODULE(IMAP_factory),
    MODULE(nsAbModule),
    MODULE(nsImportServiceModule),
    MODULE(nsTextImportModule),
    MODULE(nsAbSyncModule),
    MODULE(nsLDAPProtocolModule),

};

nsresult
apprunner_getModuleInfo(nsStaticModuleInfo **info, PRUint32 *count)
{
  *info = StaticModuleInfo;
  *count = sizeof(StaticModuleInfo) / sizeof(StaticModuleInfo[0]);
  return NS_OK;
}
