/* -*- Mode: IDL; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Peter Van der Beken <peterv@netscape.com> (original author)
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsCOMArray.h"
#include "nsIDocument.h"
#include "nsIExpatSink.h"
#include "nsINameSpace.h"
#include "nsINameSpaceManager.h"
#include "nsINodeInfo.h"
#include "nsITransformMediator.h"
#include "nsIURI.h"
#include "nsIXMLContentSink.h"
#include "txAtoms.h"
#include "txMozillaXSLTProcessor.h"
#include "txStylesheetCompiler.h"
#include "XMLUtils.h"

static void destroyAttributesArray(txStylesheetAttr* aAtts,
                                   PRUint32 aAttsCount)
{
    PRUint32 counter;
    for (counter = 0; counter < aAttsCount; ++counter) {
        NS_RELEASE(aAtts[counter].mLocalName);
        NS_IF_RELEASE(aAtts[counter].mPrefix);
    }
    delete [] aAtts;
}

class txStylesheetSink : public nsIXMLContentSink,
                         public nsIExpatSink
{
public:
    txStylesheetSink();
    virtual ~txStylesheetSink();

    nsresult Init(nsITransformMediator* aTransformMediator,
                  nsIURI* aURL,
                  txMozillaXSLTProcessor* aProcessor);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIEXPATSINK

    // nsIContentSink
    NS_IMETHOD WillBuildModel(void) { return NS_OK; }
    NS_IMETHOD DidBuildModel(PRInt32 aQualityLevel);
    NS_IMETHOD WillInterrupt(void) { return NS_OK; }
    NS_IMETHOD WillResume(void) { return NS_OK; }
    NS_IMETHOD SetParser(nsIParser* aParser) { return NS_OK; }
    NS_IMETHOD FlushPendingNotifications() { return NS_OK; }
    NS_IMETHOD SetDocumentCharset(nsAString& aCharset) { return NS_OK; }

private:
    nsCOMPtr<nsITransformMediator> mTransformMediator;
    txStylesheetCompiler* mCompiler;
    txMozillaXSLTProcessor* mProcessor;
    nsCOMArray<nsINameSpace> mNameSpaceStack;
};

nsresult
TX_NewStylesheetSink(nsIXMLContentSink** aResult,
                     nsITransformMediator* aTM,
                     nsIURI* aURI,
                     txMozillaXSLTProcessor* aProcessor)
{
    NS_ENSURE_ARG(aResult);

    txStylesheetSink* it;
    NS_NEWXPCOM(it, txStylesheetSink);
    NS_ENSURE_TRUE(it, NS_ERROR_OUT_OF_MEMORY);

    nsCOMPtr<nsIXMLContentSink> sink = it;
    nsresult rv = it->Init(aTM, aURI, aProcessor);
    NS_ENSURE_SUCCESS(rv, rv);

    return CallQueryInterface(it, aResult);
}

txStylesheetSink::txStylesheetSink()
{
}

txStylesheetSink::~txStylesheetSink()
{
}

NS_IMPL_ISUPPORTS2(txStylesheetSink, nsIXMLContentSink, nsIExpatSink)

nsresult
txStylesheetSink::Init(nsITransformMediator* aTransformMediator,
                       nsIURI* aURL,
                       txMozillaXSLTProcessor* aProcessor)
{
  mTransformMediator = aTransformMediator;
  mProcessor = aProcessor;

  nsCAutoString uri;
  aURL->GetSpec(uri);
  mCompiler = new txStylesheetCompiler(NS_ConvertUTF8toUCS2(uri));
  NS_ENSURE_TRUE(mCompiler, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP
txStylesheetSink::HandleStartElement(const PRUnichar *aName,
                                     const PRUnichar **aAtts,
                                     PRUint32 aAttsCount,
                                     PRUint32 aIndex,
                                     PRUint32 aLineNumber)
{
    nsCOMPtr<nsIAtom> prefix, localname;
    XMLUtils::splitXMLName(nsDependentString(aName), getter_AddRefs(prefix),
                           getter_AddRefs(localname));

    txStylesheetAttr* atts = nsnull;
    if (aAttsCount > 0) {
        atts = new txStylesheetAttr[aAttsCount];
        NS_ENSURE_TRUE(atts, NS_ERROR_OUT_OF_MEMORY);
    }

    nsresult rv = NS_OK;
    nsCOMPtr<nsINameSpace> nameSpace;
    if (mNameSpaceStack.Count() > 0) {
        nameSpace = mNameSpaceStack.ObjectAt(mNameSpaceStack.Count() - 1);
    }
    else {
        extern nsINameSpaceManager* gTxNameSpaceManager;
        rv = gTxNameSpaceManager->CreateRootNameSpace(*getter_AddRefs(nameSpace));
        NS_ENSURE_SUCCESS(rv, rv);
    }

    NS_ENSURE_TRUE(nameSpace, NS_ERROR_UNEXPECTED);

    PRUint32 attTotal = 0;
    while (*aAtts) {
        XMLUtils::splitXMLName(nsDependentString(aAtts[0]),
                               &atts[attTotal].mPrefix,
                               &atts[attTotal].mLocalName);

        atts[attTotal].mValue.Append(aAtts[1]);

        if (atts[attTotal].mPrefix == txXMLAtoms::xmlns) {
            nsCOMPtr<nsINameSpace> child;
            rv = nameSpace->CreateChildNameSpace(atts[attTotal].mLocalName,
                                                 atts[attTotal].mValue,
                                                 *getter_AddRefs(child));
            NS_ENSURE_SUCCESS(rv, rv);

            nameSpace = child;
        }
        else if (!atts[attTotal].mPrefix &&
                 atts[attTotal].mLocalName == txXMLAtoms::xmlns) {
            nsCOMPtr<nsINameSpace> child;
            rv = nameSpace->CreateChildNameSpace(nsnull,
                                                 atts[attTotal].mValue,
                                                 *getter_AddRefs(child));
            NS_ENSURE_SUCCESS(rv, rv);

            nameSpace = child;
        }

        ++attTotal;
        aAtts += 2;
    }

    rv = mNameSpaceStack.AppendObject(nameSpace);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt32 namespaceID = prefix ? kNameSpaceID_Unknown : kNameSpaceID_None;
    nameSpace->FindNameSpaceID(prefix, namespaceID);

    PRInt32 attCount;
    for (attCount = 0; attCount < attTotal; ++attCount) {
        if (atts[attCount].mPrefix) {
            nameSpace->FindNameSpaceID(atts[attCount].mPrefix,
                                       atts[attCount].mNamespaceID);
        }
        else if (atts[attCount].mLocalName == txXMLAtoms::xmlns) {
            atts[attCount].mNamespaceID = kNameSpaceID_XMLNS;
        }
        else {
            atts[attCount].mNamespaceID = kNameSpaceID_None;
        }
    }

    rv = mCompiler->startElement(namespaceID, localname, prefix, atts,
                                 attTotal);
    destroyAttributesArray(atts, attTotal);
    return rv;
}

NS_IMETHODIMP
txStylesheetSink::HandleEndElement(const PRUnichar *aName)
{
    return mCompiler->endElement();
}

NS_IMETHODIMP
txStylesheetSink::HandleComment(const PRUnichar *aName)
{
    return NS_OK;
}

NS_IMETHODIMP
txStylesheetSink::HandleCDataSection(const PRUnichar *aData,
                                     PRUint32 aLength)
{
    return mCompiler->characters(Substring(aData, aData + aLength));
}

NS_IMETHODIMP
txStylesheetSink::HandleDoctypeDecl(const nsAString & aSubset,
                                    const nsAString & aName,
                                    const nsAString & aSystemId,
                                    const nsAString & aPublicId,
                                    nsISupports *aCatalogData)
{
    return NS_OK;
}

NS_IMETHODIMP
txStylesheetSink::HandleCharacterData(const PRUnichar *aData,
                                      PRUint32 aLength)
{
    return mCompiler->characters(Substring(aData, aData + aLength));
}

NS_IMETHODIMP
txStylesheetSink::HandleProcessingInstruction(const PRUnichar *aTarget,
                                              const PRUnichar *aData)
{
    return NS_OK;
}

NS_IMETHODIMP
txStylesheetSink::HandleXMLDeclaration(const PRUnichar *aData,
                                       PRUint32 aLength)
{
    return NS_OK;
}

NS_IMETHODIMP
txStylesheetSink::ReportError(const PRUnichar *aErrorText,
                              const PRUnichar *aSourceText)
{
    //mCompiler->cancel(NS_ERROR_FAILURE);
    return NS_OK;
}

NS_IMETHODIMP 
txStylesheetSink::DidBuildModel(PRInt32 aQualityLevel)
{  
    nsresult rv = mCompiler->doneLoading();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mProcessor->addStylesheet(mCompiler->getStylesheet());
    NS_ENSURE_SUCCESS(rv, rv);

    return mTransformMediator->StyleSheetLoadFinished(NS_OK);
}


static nsresult handleNode(nsIDOMNode* aNode, txStylesheetCompiler& aCompiler)
{
    PRUint16 nodetype;
    aNode->GetNodeType(&nodetype);
    switch (nodetype) {
        case nsIDOMNode::ELEMENT_NODE:
        {
            nsCOMPtr<nsIContent> element = do_QueryInterface(aNode);

            nsCOMPtr<nsINodeInfo> ni;
            element->GetNodeInfo(*getter_AddRefs(ni));

            PRInt32 namespaceID;
            nsCOMPtr<nsIAtom> prefix, localname;
            ni->GetNamespaceID(namespaceID);
            ni->GetNameAtom(*getter_AddRefs(localname));
            ni->GetPrefixAtom(*getter_AddRefs(prefix));

            PRInt32 attsCount;
            element->GetAttrCount(attsCount);
            txStylesheetAttr* atts = nsnull;
            if (attsCount > 0) {
                atts = new txStylesheetAttr[attsCount];
                //NS_ENSURE_TRUE(atts, NS_ERROR_OUT_OF_MEMORY);
                PRInt32 counter;
                for (counter = 0; counter < attsCount; ++counter) {
                    txStylesheetAttr& att = atts[counter];
                    element->GetAttrNameAt(counter, att.mNamespaceID,
                                           att.mLocalName, att.mPrefix);
                    element->GetAttr(att.mNamespaceID, att.mLocalName, att.mValue);
                }
            }

            aCompiler.startElement(namespaceID, localname, prefix, atts,
                                    attsCount);
            destroyAttributesArray(atts, attsCount);

            PRInt32 childCount;
            element->ChildCount(childCount);
            if (childCount > 0) {
                PRInt32 counter = 0;
                nsCOMPtr<nsIContent> child;
                while (NS_SUCCEEDED(element->ChildAt(counter++, *getter_AddRefs(child))) && child) {
                    nsCOMPtr<nsIDOMNode> childNode = do_QueryInterface(child);
                    handleNode(childNode, aCompiler);
                }
            }

            aCompiler.endElement();
            break;
        }
        case nsIDOMNode::CDATA_SECTION_NODE:
        case nsIDOMNode::TEXT_NODE:
        {
            nsAutoString chars;
            aNode->GetNodeValue(chars);
            aCompiler.characters(chars);
            break;
        }
        case nsIDOMNode::DOCUMENT_NODE:
        {
            nsCOMPtr<nsIDocument> document = do_QueryInterface(aNode);
            PRInt32 childCount;
            document->GetChildCount(childCount);
            if (childCount > 0) {
                PRInt32 counter = 0;
                nsCOMPtr<nsIContent> child;
                while (NS_SUCCEEDED(document->ChildAt(counter++, *getter_AddRefs(child))) && child) {
                    nsCOMPtr<nsIDOMNode> childNode = do_QueryInterface(child);
                    handleNode(childNode, aCompiler);
                }
            }
            break;
        }
    }
    return NS_OK;
}

txStylesheet* TX_CompileStylesheet(nsIDOMNode* aNode)
{
    nsCOMPtr<nsIDOMDocument> document;
    aNode->GetOwnerDocument(getter_AddRefs(document));
    if (!document) {
        document = do_QueryInterface(aNode);
    }
    nsCOMPtr<nsIDOM3Node> docNode = do_QueryInterface(document);
    nsAutoString baseURI;
    docNode->GetBaseURI(baseURI);

    txStylesheetCompiler compiler(baseURI);

    handleNode(document, compiler);
    compiler.doneLoading();

    return compiler.getStylesheet();
}
