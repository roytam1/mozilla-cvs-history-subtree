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
 * The Original Code is TransforMiiX XSLT processor.
 *
 * The Initial Developer of the Original Code is The MITRE Corporation.
 * Portions created by MITRE are Copyright (C) 1999 The MITRE Corporation.
 *
 * Portions created by Keith Visco as a Non MITRE employee,
 * (C) 1999, 2000 Keith Visco. All Rights Reserved.
 *
 * Contributor(s):
 * Keith Visco, kvisco@ziplink.net
 *    -- original author.
 *
 * Bob Miller, kbob@oblix.com
 *    -- plugged core leak.
 *
 * Pierre Phaneuf, pp@ludusdesign.com
 *    -- fixed some XPCOM usage.
 *
 * Marina Mechtcheriakova, mmarina@mindspring.com
 *    -- Added call to recurisvely attribute-set processing on
 *       xsl:attribute-set itself
 *    -- Added call to handle attribute-set processing for xsl:copy
 *
 * Nathan Pride, npride@wavo.com
 *    -- fixed a document base issue
 *
 * Olivier Gerardin
 *    -- Changed behavior of passing parameters to templates
 *
 */

#include "XSLTProcessor.h"
#include "Names.h"
#include "Numbering.h"
#include "Tokenizer.h"
#include "txAtoms.h"
#include "txNodeSetContext.h"
#include "txNodeSorter.h"
#include "txRtfHandler.h"
#include "txTextHandler.h"
#include "txURIUtils.h"
#include "VariableBinding.h"
#include "XMLDOMUtils.h"
#include "XMLUtils.h"

const String txXSLTProcessor::NON_TEXT_TEMPLATE_WARNING(
"templates for the following element are not allowed to generate non character data: ");

/*
 * Implement static variables for atomservice and dom.
 */
#ifdef TX_EXE
TX_IMPL_ATOM_STATICS;
TX_IMPL_DOM_STATICS;
#endif

/* static */
MBool
txXSLTProcessor::txInit()
{
#ifdef TX_EXE
    if (!txNamespaceManager::init())
        return MB_FALSE;
#endif
    if (!txHTMLAtoms::init())
        return MB_FALSE;
    if (!txXMLAtoms::init())
        return MB_FALSE;
    if (!txXPathAtoms::init())
        return MB_FALSE;
    return txXSLTAtoms::init();
}

/* static */
MBool
txXSLTProcessor::txShutdown()
{
#ifdef TX_EXE
    txNamespaceManager::shutdown();
#endif
    txHTMLAtoms::shutdown();
    txXMLAtoms::shutdown();
    txXPathAtoms::shutdown();
    txXSLTAtoms::shutdown();
    return MB_TRUE;
}

txXSLTProcessor::txXSLTProcessor() : mOutputHandler(0),
                                     mResultHandler(0),
                                     mXsltVersion("1.0"),
                                     mAppName("TransforMiiX"),
                                     mAppVersion("1.0 [beta]")
{
    // Create default expressions

    // "node()"
    txNodeTest* nt = new txNodeTypeTest(txNodeTypeTest::NODE_TYPE);
    mNodeExpr = new LocationStep(nt, LocationStep::CHILD_AXIS);
}

txXSLTProcessor::~txXSLTProcessor()
{
    delete mNodeExpr;
}

String&
txXSLTProcessor::getAppName()
{
    return mAppName;
}

String&
txXSLTProcessor::getAppVersion()
{
    return mAppVersion;
}

void
txXSLTProcessor::bindVariable(String& aName,
                              ExprResult* aValue,
                              MBool aAllowShadowing,
                              ProcessorState* aPs)
{
    NamedMap* varSet = (NamedMap*)aPs->getVariableSetStack()->peek();
    //-- check for duplicate variable names
    VariableBinding* current = (VariableBinding*) varSet->get(aName);
    VariableBinding* binding = 0;
    if (current) {
        binding = current;
        if (current->isShadowingAllowed()) {
            current->setShadowValue(aValue);
        }
        else {
            //-- error cannot rebind variables
            String err("cannot rebind variables: ");
            err.append(aName);
            err.append(" already exists in this scope.");
            aPs->receiveError(err, NS_ERROR_FAILURE);
        }
    }
    else {
        binding = new VariableBinding(aName, aValue);
        varSet->put((const String&)aName, binding);
    }
    if (aAllowShadowing) {
        binding->allowShadowing();
    }
    else {
        binding->disallowShadowing();
    }

}

void
txXSLTProcessor::copyNode(Node* aNode, ProcessorState* aPs)
{
    if (!aNode)
        return;

    switch (aNode->getNodeType()) {
        case Node::ATTRIBUTE_NODE:
        {
            NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
            mResultHandler->attribute(aNode->getNodeName(), aNode->getNamespaceID(),
                                      aNode->getNodeValue());
            break;
        }
        case Node::COMMENT_NODE:
        {
            NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
            mResultHandler->comment(((Comment*)aNode)->getData());
            break;
        }
        case Node::DOCUMENT_NODE:
        case Node::DOCUMENT_FRAGMENT_NODE:
        {
            // Copy children
            Node* child = aNode->getFirstChild();
            while (child) {
                copyNode(child, aPs);
                child = child->getNextSibling();
            }
            break;
        }
        case Node::ELEMENT_NODE:
        {
            Element* element = (Element*)aNode;
            const String& name = element->getNodeName();
            PRInt32 nsID = element->getNamespaceID();
            startElement(name, nsID, aPs);

            // Copy attributes
            NamedNodeMap* attList = element->getAttributes();
            if (attList) {
                PRUint32 i = 0;
                for (i = 0; i < attList->getLength(); i++) {
                    Attr* attr = (Attr*)attList->item(i);
                    NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
                    mResultHandler->attribute(attr->getName(), attr->getNamespaceID(),
                                              attr->getValue());
                }
            }

            // Copy children
            Node* child = element->getFirstChild();
            while (child) {
                copyNode(child, aPs);
                child = child->getNextSibling();
            }

            NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
            mResultHandler->endElement(name, nsID);
            break;
        }
        case Node::PROCESSING_INSTRUCTION_NODE:
        {
            ProcessingInstruction* pi = (ProcessingInstruction*)aNode;
            NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
            mResultHandler->processingInstruction(pi->getTarget(), pi->getData());
            break;
        }
        case Node::TEXT_NODE:
        case Node::CDATA_SECTION_NODE:
        {
            NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
            mResultHandler->characters(((CharacterData*)aNode)->getData());
            break;
        }
    }
}

void
txXSLTProcessor::process(Node* aNode,
                         const txExpandedName& aMode,
                         ProcessorState* aPs)
{
    if (!aNode)
        return;

    ProcessorState::ImportFrame *frame;
    Node* xslTemplate = aPs->findTemplate(aNode, aMode, &frame);
    processMatchedTemplate(xslTemplate, aNode, 0, aMode, frame, aPs);
}

void
txXSLTProcessor::processAction(Node* aNode,
                               Node* aXsltAction,
                               ProcessorState* aPs)
{
    nsresult rv = NS_OK;
    NS_ASSERTION(aXsltAction, "We need an action to process.");
    if (!aXsltAction)
        return;

    unsigned short nodeType = aXsltAction->getNodeType();

    // Handle text nodes
    if (nodeType == Node::TEXT_NODE ||
        nodeType == Node::CDATA_SECTION_NODE) {
        const String& textValue = aXsltAction->getNodeValue();
        if (!aPs->isXSLStripSpaceAllowed(aXsltAction) ||
            !XMLUtils::isWhitespace(textValue)) {
            NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
            mResultHandler->characters(textValue);
        }
        return;
    }

    if (nodeType != Node::ELEMENT_NODE) {
        return;
    }

    // Handle element nodes
    Element* actionElement = (Element*)aXsltAction;
    PRInt32 nsID = aXsltAction->getNamespaceID();
    if (nsID != kNameSpaceID_XSLT) {
        // Literal result element
        // XXX TODO Check for excluded namespaces and aliased namespaces (element and attributes) 
        const String& nodeName = aXsltAction->getNodeName();
        startElement(nodeName, nsID, aPs);

        processAttributeSets(actionElement, aNode, aPs);

        // Handle attributes
        NamedNodeMap* atts = actionElement->getAttributes();

        if (atts) {
            // Process all non XSLT attributes
            PRUint32 i;
            for (i = 0; i < atts->getLength(); ++i) {
                Attr* attr = (Attr*)atts->item(i);
                if (attr->getNamespaceID() == kNameSpaceID_XSLT)
                    continue;
                // Process Attribute Value Templates
                String value;
                aPs->processAttrValueTemplate(attr->getValue(), actionElement, value);
                NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
                mResultHandler->attribute(attr->getName(), attr->getNamespaceID(), value);
            }
        }

        // Process children
        processChildren(aNode, actionElement, aPs);
        NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
        mResultHandler->endElement(nodeName, nsID);
        return;
    }

    Expr* expr = 0;
    txAtom* localName;
    aXsltAction->getLocalName(&localName);
    // xsl:apply-imports
    if (localName == txXSLTAtoms::applyImports) {
        ProcessorState::TemplateRule* curr;
        Node* xslTemplate;
        ProcessorState::ImportFrame *frame;

        curr = aPs->getCurrentTemplateRule();
        if (!curr) {
            String err("apply-imports not allowed here");
            aPs->receiveError(err, NS_ERROR_FAILURE);
            TX_RELEASE_ATOM(localName);
            return;
        }

        xslTemplate = aPs->findTemplate(aNode, *curr->mMode,
                                        curr->mFrame, &frame);
        processMatchedTemplate(xslTemplate, aNode, curr->mParams,
                               *curr->mMode, frame, aPs);

    }
    // xsl:apply-templates
    else if (localName == txXSLTAtoms::applyTemplates) {
        if (actionElement->hasAttr(txXSLTAtoms::select,
                                   kNameSpaceID_None))
            expr = aPs->getExpr(actionElement,
                                ProcessorState::SelectAttr);
        else
            expr = mNodeExpr;

        if (!expr) {
            TX_RELEASE_ATOM(localName);
            return;
        }

        ExprResult* exprResult = expr->evaluate(aPs->getEvalContext());
        if (!exprResult) {
            TX_RELEASE_ATOM(localName);
            return;
        }

        if (exprResult->getResultType() == ExprResult::NODESET) {
            NodeSet* nodeSet = (NodeSet*)exprResult;

            // Look for xsl:sort elements
            txNodeSorter sorter(aPs);
            Node* child = actionElement->getFirstChild();
            while (child) {
                if (child->getNodeType() == Node::ELEMENT_NODE &&
                    child->getNamespaceID() == kNameSpaceID_XSLT) {
                    txAtom* childLocalName;
                    child->getLocalName(&childLocalName);
                    if (childLocalName == txXSLTAtoms::sort) {
                        sorter.addSortElement((Element*)child, aNode);
                    }
                    TX_IF_RELEASE_ATOM(childLocalName);
                }
                child = child->getNextSibling();
            }
            sorter.sortNodeSet(nodeSet);

            // Process xsl:with-param elements
            NamedMap* actualParams = processParameters(actionElement, aNode, aPs);

            // Get mode
            String modeStr;
            txExpandedName mode;
            if (actionElement->getAttr(txXSLTAtoms::mode,
                                       kNameSpaceID_None, modeStr)) {
                rv = mode.init(modeStr, actionElement, MB_FALSE);
                if (NS_FAILED(rv)) {
                    String err("malformed mode-name in xsl:apply-templates");
                    aPs->receiveError(err);
                    TX_IF_RELEASE_ATOM(localName);
                    delete actualParams;
                    return;
                }
            }

            txNodeSetContext evalContext(nodeSet, aPs);
            txIEvalContext* priorEC =
                aPs->setEvalContext(&evalContext);
            while (evalContext.hasNext()) {
                evalContext.next();
                ProcessorState::ImportFrame *frame;
                Node* currNode = evalContext.getContextNode();
                Node* xslTemplate;
                xslTemplate = aPs->findTemplate(currNode, mode, &frame);
                processMatchedTemplate(xslTemplate, currNode,
                                       actualParams, mode, frame, aPs);
            }

            aPs->setEvalContext(priorEC);

            delete actualParams;
        }
        else {
            String err("error processing apply-templates");
            aPs->receiveError(err, NS_ERROR_FAILURE);
        }
        //-- clean up
        delete exprResult;
    }
    // xsl:attribute
    else if (localName == txXSLTAtoms::attribute) {
        String nameAttr;
        if (!actionElement->getAttr(txXSLTAtoms::name,
                                    kNameSpaceID_None, nameAttr)) {
            String err("missing required name attribute for xsl:attribute");
            aPs->receiveError(err, NS_ERROR_FAILURE);
            TX_RELEASE_ATOM(localName);
            return;
        }

        // Process name as an AttributeValueTemplate
        String name;
        aPs->processAttrValueTemplate(nameAttr, actionElement, name);

        // Check name validity (must be valid QName and not xmlns)
        if (!XMLUtils::isValidQName(name)) {
            String err("error processing xsl:attribute, ");
            err.append(name);
            err.append(" is not a valid QName.");
            aPs->receiveError(err, NS_ERROR_FAILURE);
            TX_RELEASE_ATOM(localName);
            return;
        }

        txAtom* nameAtom = TX_GET_ATOM(name);
        if (nameAtom == txXMLAtoms::xmlns) {
            TX_RELEASE_ATOM(nameAtom);
            String err("error processing xsl:attribute, name is xmlns.");
            aPs->receiveError(err, NS_ERROR_FAILURE);
            TX_RELEASE_ATOM(localName);
            return;
        }
        TX_IF_RELEASE_ATOM(nameAtom);

        // Determine namespace URI from the namespace attribute or
        // from the prefix of the name (using the xslt action element).
        String resultNs;
        PRInt32 resultNsID = kNameSpaceID_None;
        if (actionElement->getAttr(txXSLTAtoms::_namespace, kNameSpaceID_None,
                                   resultNs)) {
            String nsURI;
            aPs->processAttrValueTemplate(resultNs, actionElement,
                                          nsURI);
            resultNsID = aPs->getStylesheetDocument()->namespaceURIToID(nsURI);
        }
        else {
            String prefix;
            XMLUtils::getPrefix(name, prefix);
            txAtom* prefixAtom = TX_GET_ATOM(prefix);
            if (prefixAtom != txXMLAtoms::_empty) {
                if (prefixAtom != txXMLAtoms::xmlns)
                    resultNsID = actionElement->lookupNamespaceID(prefixAtom);
                else
                    // Cut xmlns: (6 characters)
                    name.deleteChars(0, 6);
            }
            TX_IF_RELEASE_ATOM(prefixAtom);
        }

        // XXX Should verify that this is correct behaviour. Signal error too?
        if (resultNsID == kNameSpaceID_Unknown) {
            TX_RELEASE_ATOM(localName);
            return;
        }

        // Compute value
        String value;
        processChildrenAsValue(aNode, actionElement, aPs, MB_TRUE, value);

        NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
        mResultHandler->attribute(name, resultNsID, value);
    }
    // xsl:call-template
    else if (localName == txXSLTAtoms::callTemplate) {
        String nameStr;
        txExpandedName templateName;
        actionElement->getAttr(txXSLTAtoms::name,
                               kNameSpaceID_None, nameStr);

        rv = templateName.init(nameStr, actionElement, MB_FALSE);
        if (NS_SUCCEEDED(rv)) {
            Element* xslTemplate = aPs->getNamedTemplate(templateName);
            if (xslTemplate) {
#ifdef PR_LOGGING
                char *nameBuf = 0, *uriBuf = 0;
                PR_LOG(txLog::xslt, PR_LOG_DEBUG,
                       ("CallTemplate, Name %s, Stylesheet %s\n",
                        (nameBuf = nameStr.toCharArray()),
                        (uriBuf = xslTemplate->getBaseURI().toCharArray())));
                delete nameBuf;
                delete uriBuf;
#endif
                NamedMap* actualParams = processParameters(actionElement, aNode, aPs);
                processTemplate(aNode, xslTemplate, aPs, actualParams);
                delete actualParams;
            }
        }
        else {
            String err("missing or malformed name in xsl:call-template");
            aPs->receiveError(err, NS_ERROR_FAILURE);
        }
    }
    // xsl:choose
    else if (localName == txXSLTAtoms::choose) {
        Node* condition = actionElement->getFirstChild();
        MBool caseFound = MB_FALSE;
        while (!caseFound && condition) {
            if (condition->getNodeType() != Node::ELEMENT_NODE ||
                condition->getNamespaceID() != kNameSpaceID_XSLT) {
                condition = condition->getNextSibling();
                continue;
            }

            Element* xslTemplate = (Element*)condition;
            txAtom* conditionLocalName;
            condition->getLocalName(&conditionLocalName);
            if (conditionLocalName == txXSLTAtoms::when) {
                expr = aPs->getExpr(xslTemplate,
                                    ProcessorState::TestAttr);
                if (!expr) {
                    TX_RELEASE_ATOM(conditionLocalName);
                    condition = condition->getNextSibling();
                    continue;
                }

                ExprResult* result =
                    expr->evaluate(aPs->getEvalContext());
                if (result && result->booleanValue()) {
                    processChildren(aNode, xslTemplate, aPs);
                    caseFound = MB_TRUE;
                }
                delete result;
            }
            else if (conditionLocalName == txXSLTAtoms::otherwise) {
                processChildren(aNode, xslTemplate, aPs);
                caseFound = MB_TRUE;
            }
            TX_IF_RELEASE_ATOM(conditionLocalName);
            condition = condition->getNextSibling();
        } // end for-each child of xsl:choose
    }
    // xsl:comment
    else if (localName == txXSLTAtoms::comment) {
        String value;
        processChildrenAsValue(aNode, actionElement, aPs, MB_TRUE, value);
        PRInt32 pos = 0;
        PRUint32 length = value.length();
        while ((pos = value.indexOf('-', pos)) != kNotFound) {
            ++pos;
            if ((pos == length) || (value.charAt(pos) == '-'))
                value.insert(pos++, ' ');
        }
        NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
        mResultHandler->comment(value);
    }
    // xsl:copy
    else if (localName == txXSLTAtoms::copy) {
        xslCopy(aNode, actionElement, aPs);
    }
    // xsl:copy-of
    else if (localName == txXSLTAtoms::copyOf) {
        expr = aPs->getExpr(actionElement, ProcessorState::SelectAttr);
        if (!expr) {
            TX_RELEASE_ATOM(localName);
            return;
        }

        ExprResult* exprResult = expr->evaluate(aPs->getEvalContext());
        xslCopyOf(exprResult, aPs);
        delete exprResult;
    }
    // xsl:element
    else if (localName == txXSLTAtoms::element) {
        String nameAttr;
        if (!actionElement->getAttr(txXSLTAtoms::name,
                                    kNameSpaceID_None, nameAttr)) {
            String err("missing required name attribute for xsl:element");
            aPs->receiveError(err, NS_ERROR_FAILURE);
            TX_RELEASE_ATOM(localName);
            return;
        }

        // Process name as an AttributeValueTemplate
        String name;
        aPs->processAttrValueTemplate(nameAttr, actionElement, name);

        // Check name validity (must be valid QName and not xmlns)
        if (!XMLUtils::isValidQName(name)) {
            String err("error processing xsl:element, '");
            err.append(name);
            err.append("' is not a valid QName.");
            aPs->receiveError(err, NS_ERROR_FAILURE);
            // XXX We should processChildren without creating attributes or
            //     namespace nodes.
            TX_RELEASE_ATOM(localName);
            return;
        }

        // Determine namespace URI from the namespace attribute or
        // from the prefix of the name (using the xslt action element).
        String resultNs;
        PRInt32 resultNsID;
        if (actionElement->getAttr(txXSLTAtoms::_namespace, kNameSpaceID_None, resultNs)) {
            String nsURI;
            aPs->processAttrValueTemplate(resultNs, actionElement, nsURI);
            if (nsURI.isEmpty())
                resultNsID = kNameSpaceID_None;
            else
                resultNsID = aPs->getStylesheetDocument()->namespaceURIToID(nsURI);
        }
        else {
            String prefix;
            XMLUtils::getPrefix(name, prefix);
            txAtom* prefixAtom = TX_GET_ATOM(prefix);
            resultNsID = actionElement->lookupNamespaceID(prefixAtom);
            TX_IF_RELEASE_ATOM(prefixAtom);
         }

        if (resultNsID == kNameSpaceID_Unknown) {
            String err("error processing xsl:element, can't resolve prefix on'");
            err.append(name);
            err.append("'.");
            aPs->receiveError(err, NS_ERROR_FAILURE);
            // XXX We should processChildren without creating attributes or
            //     namespace nodes.
            TX_RELEASE_ATOM(localName);
            return;
        }

        startElement(name, resultNsID, aPs);
        processAttributeSets(actionElement, aNode, aPs);
        processChildren(aNode, actionElement, aPs);
        NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
        mResultHandler->endElement(name, resultNsID);
    }
    // xsl:for-each
    else if (localName == txXSLTAtoms::forEach) {
        expr = aPs->getExpr(actionElement, ProcessorState::SelectAttr);
        if (!expr) {
            TX_RELEASE_ATOM(localName);
            return;
        }

        ExprResult* exprResult = expr->evaluate(aPs->getEvalContext());
        if (!exprResult) {
            TX_RELEASE_ATOM(localName);
            return;
        }

        if (exprResult->getResultType() == ExprResult::NODESET) {
            NodeSet* nodeSet = (NodeSet*)exprResult;
            txNodeSetContext evalContext(nodeSet, aPs);
            txIEvalContext* priorEC =
                aPs->setEvalContext(&evalContext);

            // Look for xsl:sort elements
            txNodeSorter sorter(aPs);
            Node* child = actionElement->getFirstChild();
            while (child) {
                unsigned short nodeType = child->getNodeType();
                if (nodeType == Node::ELEMENT_NODE) {
                    txAtom* childLocalName;
                    child->getLocalName(&childLocalName);
                    if (child->getNamespaceID() != kNameSpaceID_XSLT ||
                        childLocalName != txXSLTAtoms::sort) {
                        // xsl:sort must occur first
                        TX_IF_RELEASE_ATOM(childLocalName);
                        break;
                    }
                    sorter.addSortElement((Element*)child, aNode);
                    TX_RELEASE_ATOM(childLocalName);
                }
                else if ((nodeType == Node::TEXT_NODE ||
                          nodeType == Node::CDATA_SECTION_NODE) &&
                         !XMLUtils::isWhitespace(child->getNodeValue())) {
                    break;
                }

                child = child->getNextSibling();
            }
            sorter.sortNodeSet(nodeSet);

            // Set current template to null
            ProcessorState::TemplateRule *oldTemplate;
            oldTemplate = aPs->getCurrentTemplateRule();
            aPs->setCurrentTemplateRule(0);

            while (evalContext.hasNext()) {
                evalContext.next();
                Node* currNode = evalContext.getContextNode();
                processChildren(currNode, actionElement, aPs);
            }

            aPs->setCurrentTemplateRule(oldTemplate);
            aPs->setEvalContext(priorEC);
        }
        else {
            String err("error processing for-each");
            aPs->receiveError(err, NS_ERROR_FAILURE);
        }
        //-- clean up exprResult
        delete exprResult;
    }
    // xsl:if
    else if (localName == txXSLTAtoms::_if) {
        expr = aPs->getExpr(actionElement, ProcessorState::TestAttr);
        if (!expr) {
            TX_RELEASE_ATOM(localName);
            return;
        }

        ExprResult* exprResult = expr->evaluate(aPs->getEvalContext());
        if (!exprResult) {
            TX_RELEASE_ATOM(localName);
            return;
        }

        if (exprResult->booleanValue()) {
            processChildren(aNode, actionElement, aPs);
        }
        delete exprResult;
    }
    // xsl:message
    else if (localName == txXSLTAtoms::message) {
        String message;
        processChildrenAsValue(aNode, actionElement, aPs, MB_FALSE, message);
        // We should add a MessageObserver class
        aPs->getProcessorHelper()->logMessage(message);
    }
    // xsl:number
    else if (localName == txXSLTAtoms::number) {
        String result;
        Numbering::doNumbering(actionElement, result, aPs);
        NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
        mResultHandler->characters(result);
    }
    // xsl:param
    else if (localName == txXSLTAtoms::param) {
        // Ignore in this loop (already processed)
    }
    // xsl:processing-instruction
    else if (localName == txXSLTAtoms::processingInstruction) {
        String nameAttr;
        if (!actionElement->getAttr(txXSLTAtoms::name,
                                    kNameSpaceID_None, nameAttr)) {
            String err("missing required name attribute for"
                       " xsl:processing-instruction");
            aPs->receiveError(err, NS_ERROR_FAILURE);
            TX_RELEASE_ATOM(localName);
            return;
        }

        // Process name as an AttributeValueTemplate
        String name;
        aPs->processAttrValueTemplate(nameAttr, actionElement, name);

        // Check name validity (must be valid NCName and a PITarget)
        // XXX Need to check for NCName and PITarget
        if (!XMLUtils::isValidQName(name)) {
            String err("error processing xsl:processing-instruction, '");
            err.append(name);
            err.append("' is not a valid QName.");
            aPs->receiveError(err, NS_ERROR_FAILURE);
        }

        // Compute value
        String value;
        processChildrenAsValue(aNode, actionElement, aPs, MB_TRUE, value);
        XMLUtils::normalizePIValue(value);
        NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
        mResultHandler->processingInstruction(name, value);
    }
    // xsl:sort
    else if (localName == txXSLTAtoms::sort) {
        // Ignore in this loop
    }
    // xsl:text
    else if (localName == txXSLTAtoms::text) {
        String data;
        XMLDOMUtils::getNodeValue(actionElement, data);

        NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
#ifdef TX_EXE
        String aValue;
        if ((mResultHandler == mOutputHandler) &&
            actionElement->getAttr(txXSLTAtoms::disableOutputEscaping,
                                   kNameSpaceID_None, aValue) &&
            aValue.isEqual(YES_VALUE))
            mOutputHandler->charactersNoOutputEscaping(data);
        else
            mResultHandler->characters(data);
#else
        mResultHandler->characters(data);
#endif
    }
    // xsl:value-of
    else if (localName == txXSLTAtoms::valueOf) {
        expr = aPs->getExpr(actionElement, ProcessorState::SelectAttr);
        if (!expr) {
            TX_RELEASE_ATOM(localName);
            return;
        }

        ExprResult* exprResult = expr->evaluate(aPs->getEvalContext());
        String value;
        if (!exprResult) {
            String err("null ExprResult");
            aPs->receiveError(err, NS_ERROR_FAILURE);
            TX_RELEASE_ATOM(localName);
            return;
        }
        exprResult->stringValue(value);

        NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
#ifdef TX_EXE
        String aValue;
        if ((mResultHandler == mOutputHandler) &&
            actionElement->getAttr(txXSLTAtoms::disableOutputEscaping,
                                   kNameSpaceID_None, aValue) &&
            aValue.isEqual(YES_VALUE))
            mOutputHandler->charactersNoOutputEscaping(value);
        else
            mResultHandler->characters(value);
#else
        mResultHandler->characters(value);
#endif
        delete exprResult;
    }
    // xsl:variable
    else if (localName == txXSLTAtoms::variable) {
        String name;
        if (!actionElement->getAttr(txXSLTAtoms::name,
                                    kNameSpaceID_None, name)) {
            String err("missing required name attribute for xsl:variable");
            aPs->receiveError(err, NS_ERROR_FAILURE);
            TX_IF_RELEASE_ATOM(localName);
            return;
        }
        ExprResult* exprResult = processVariable(aNode, actionElement, aPs);
        bindVariable(name, exprResult, MB_FALSE, aPs);
    }
    TX_IF_RELEASE_ATOM(localName);
}

void
txXSLTProcessor::processAttributeSets(Element* aElement,
                                      Node* aNode,
                                      ProcessorState* aPs)
{
    nsresult rv = NS_OK;
    String names;
    PRInt32 namespaceID;
    if (aElement->getNamespaceID() == kNameSpaceID_XSLT)
        namespaceID = kNameSpaceID_None;
    else
        namespaceID = kNameSpaceID_XSLT;
    if (!aElement->getAttr(txXSLTAtoms::useAttributeSets, namespaceID, names) || names.isEmpty())
        return;

    // Split names
    txTokenizer tokenizer(names);
    String nameStr;
    while (tokenizer.hasMoreTokens()) {
        tokenizer.nextToken(nameStr);
        txExpandedName name;
        rv = name.init(nameStr, aElement, MB_FALSE);
        if (NS_FAILED(rv)) {
            String err("missing or malformed name in use-attribute-sets");
            aPs->receiveError(err);
            return;
        }

        txStackIterator attributeSets(&mAttributeSetStack);
        while (attributeSets.hasNext()) {
            if (name == *(txExpandedName*)attributeSets.next()) {
                String err("circular inclusion detected in use-attribute-sets");
                aPs->receiveError(err);
                return;
            }
        }

        NodeSet* attSet = aPs->getAttributeSet(name);
        if (attSet) {
            int i;
            //-- issue: we still need to handle the following fix cleaner, since
            //-- attribute sets are merged, a different parent could exist
            //-- for different xsl:attribute nodes. I will probably create
            //-- an AttributeSet object, which will handle this case better. - Keith V.
            if (attSet->size() > 0) {
                mAttributeSetStack.push(&name);
                Element* parent = (Element*) attSet->get(0)->getXPathParent();
                processAttributeSets(parent, aNode, aPs);
                mAttributeSetStack.pop();
            }
            for (i = 0; i < attSet->size(); i++)
                processAction(aNode, attSet->get(i), aPs);
            delete attSet;
        }
    }
}

void
txXSLTProcessor::processChildren(Node* aNode,
                                 Element* aXsltElement,
                                 ProcessorState* aPs)
{
    NS_ASSERTION(aXsltElement,"xslElement is NULL in call to txXSLTProcessor::processChildren!");

    Stack* bindings = aPs->getVariableSetStack();
    NamedMap localBindings;
    localBindings.setObjectDeletion(MB_TRUE);
    bindings->push(&localBindings);
    Node* child = aXsltElement->getFirstChild();
    while (child) {
        processAction(aNode, child, aPs);
        child = child->getNextSibling();
    }
    bindings->pop();
}

void
txXSLTProcessor::processChildrenAsValue(Node* aNode,
                                        Element* aElement,
                                        ProcessorState* aPs,
                                        MBool aOnlyText,
                                        String& aValue)
{
    txXMLEventHandler* previousHandler = mResultHandler;
    txTextHandler valueHandler(aValue, aOnlyText);
    mResultHandler = &valueHandler;
    processChildren(aNode, aElement, aPs);
    mResultHandler = previousHandler;
}

void
txXSLTProcessor::processDefaultTemplate(Node* aNode,
                                        ProcessorState* aPs,
                                        const txExpandedName& aMode)
{
    NS_ASSERTION(aNode, "context node is NULL in call to txXSLTProcessor::processTemplate!");

    switch (aNode->getNodeType())
    {
        case Node::ELEMENT_NODE :
        case Node::DOCUMENT_NODE :
        {
            if (!mNodeExpr)
                break;

            ExprResult* exprResult = mNodeExpr->evaluate(aPs->getEvalContext());
            if (!exprResult ||
                exprResult->getResultType() != ExprResult::NODESET) {
                String err("None-nodeset returned while processing default template");
                aPs->receiveError(err, NS_ERROR_FAILURE);
                delete exprResult;
                return;
            }

            NodeSet* nodeSet = (NodeSet*)exprResult;
            txNodeSetContext evalContext(nodeSet, aPs);
            txIEvalContext* priorEC = aPs->setEvalContext(&evalContext);

            while (evalContext.hasNext()) {
                evalContext.next();
                Node* currNode = evalContext.getContextNode();

                ProcessorState::ImportFrame *frame;
                Node* xslTemplate = aPs->findTemplate(currNode, aMode, &frame);
                processMatchedTemplate(xslTemplate, currNode, 0, aMode, frame,
                                       aPs);
            }
            aPs->setEvalContext(priorEC);
            delete exprResult;
            break;
        }
        case Node::ATTRIBUTE_NODE :
        case Node::TEXT_NODE :
        case Node::CDATA_SECTION_NODE :
        {
            NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
            mResultHandler->characters(aNode->getNodeValue());
            break;
        }
        default:
            // on all other nodetypes (including namespace nodes)
            // we do nothing
            break;
    }
}

void
txXSLTProcessor::processInclude(String& aHref,
                                Document* aSource,
                                txListIterator* aImportFrame,
                                ProcessorState* aPs)
{
    // make sure the include isn't included yet
    txStackIterator iter(aPs->getEnteredStylesheets());
    while (iter.hasNext()) {
        if (((String*)iter.next())->isEqual(aHref)) {
            String err("Stylesheet includes itself. URI: ");
            err.append(aHref);
            aPs->receiveError(err, NS_ERROR_FAILURE);
            return;
        }
    }
    aPs->getEnteredStylesheets()->push(&aHref);

    // Load XSL document
    Node* stylesheet = aPs->retrieveDocument(aHref, NULL_STRING);
    if (!stylesheet) {
        String err("Unable to load included stylesheet ");
        err.append(aHref);
        aPs->receiveError(err, NS_ERROR_FAILURE);
        aPs->getEnteredStylesheets()->pop();
        return;
    }

    switch(stylesheet->getNodeType()) {
        case Node::DOCUMENT_NODE :
            processStylesheet(aSource,
                              (Document*)stylesheet,
                              aImportFrame,
                              aPs);
            break;
        case Node::ELEMENT_NODE :
            processTopLevel(aSource, (Element*)stylesheet, aImportFrame, aPs);
            break;
        default:
            // This should never happen
            String err("Unsupported fragment identifier");
            aPs->receiveError(err, NS_ERROR_FAILURE);
            break;
    }

    aPs->getEnteredStylesheets()->pop();
}

void
txXSLTProcessor::processMatchedTemplate(Node* aXsltTemplate,
                                        Node* aNode,
                                        NamedMap* aParams,
                                        const txExpandedName& aMode,
                                        ProcessorState::ImportFrame* aFrame,
                                        ProcessorState* aPs)
{
    if (aXsltTemplate) {
        ProcessorState::TemplateRule *oldTemplate, newTemplate;
        oldTemplate = aPs->getCurrentTemplateRule();
        newTemplate.mFrame = aFrame;
        newTemplate.mMode = &aMode;
        newTemplate.mParams = aParams;
        aPs->setCurrentTemplateRule(&newTemplate);

        processTemplate(aNode, aXsltTemplate, aPs, aParams);

        aPs->setCurrentTemplateRule(oldTemplate);
    }
    else {
        processDefaultTemplate(aNode, aPs, aMode);
    }
}

NamedMap*
txXSLTProcessor::processParameters(Element* aXsltAction,
                                   Node* aContext,
                                   ProcessorState* aPs)
{
    NamedMap* params = new NamedMap();

    if (!aXsltAction || !params)
      return params;

    params->setObjectDeletion(MB_TRUE);

    //-- handle xsl:with-param elements
    Node* tmpNode = aXsltAction->getFirstChild();
    while (tmpNode) {
        if (tmpNode->getNodeType() == Node::ELEMENT_NODE &&
            tmpNode->getNamespaceID() == kNameSpaceID_XSLT) {
            txAtom* localName;
            tmpNode->getLocalName(&localName);
            if (localName != txXSLTAtoms::withParam) {
                TX_IF_RELEASE_ATOM(localName);
                tmpNode = tmpNode->getNextSibling();
                continue;
            }

            Element* action = (Element*)tmpNode;
            String name;
            if (!action->getAttr(txXSLTAtoms::name,
                                 kNameSpaceID_None, name)) {
                String err("missing required name attribute for xsl:with-param");
                aPs->receiveError(err, NS_ERROR_FAILURE);
            }
            else {
                ExprResult* exprResult = processVariable(aContext, action, aPs);
                if (params->get(name)) {
                    //-- error cannot rebind parameters
                    String err("value for parameter '");
                    err.append(name);
                    err.append("' specified more than once.");
                    aPs->receiveError(err, NS_ERROR_FAILURE);
                }
                else {
                    VariableBinding* binding = new VariableBinding(name, exprResult);
                    params->put((const String&)name, binding);
                }
            }
            TX_RELEASE_ATOM(localName);
        }
        tmpNode = tmpNode->getNextSibling();
    }
    return params;
}

void
txXSLTProcessor::processStylesheet(Document* aSource,
                                   Document* aStylesheet,
                                   txListIterator* aImportFrame,
                                   ProcessorState* aPs)
{
    NS_ASSERTION(aStylesheet, "processTopLevel called without stylesheet");
    if (!aStylesheet || !aStylesheet->getDocumentElement())
        return;

    Element* elem = aStylesheet->getDocumentElement();

    txAtom* localName;
    PRInt32 namespaceID = elem->getNamespaceID();
    elem->getLocalName(&localName);

    if (((localName == txXSLTAtoms::stylesheet) ||
         (localName == txXSLTAtoms::transform)) &&
        (namespaceID == kNameSpaceID_XSLT)) {
        processTopLevel(aSource, elem, aImportFrame, aPs);
    }
    else {
        NS_ASSERTION(aImportFrame->current(), "no current importframe");
        if (!aImportFrame->current()) {
            TX_IF_RELEASE_ATOM(localName);
            return;
        }
        aPs->addLREStylesheet(aStylesheet,
            (ProcessorState::ImportFrame*)aImportFrame->current());
    }
    TX_IF_RELEASE_ATOM(localName);
}

void
txXSLTProcessor::processTemplate(Node* aNode,
                                 Node* aXsltTemplate,
                                 ProcessorState* aPs,
                                 NamedMap* aParams)
{
    NS_ASSERTION(aXsltTemplate, "xslTemplate is NULL in call to txXSLTProcessor::processTemplate!");

    Stack* bindings = aPs->getVariableSetStack();
    NamedMap localBindings;
    localBindings.setObjectDeletion(MB_TRUE);
    bindings->push(&localBindings);
    processTemplateParams(aXsltTemplate, aNode, aPs, aParams);
    Node* tmp = aXsltTemplate->getFirstChild();
    while (tmp) {
        processAction(aNode, tmp, aPs);
        tmp = tmp->getNextSibling();
    }
    
    if (aParams) {
        StringList* keys = aParams->keys();
        if (keys) {
            StringListIterator keyIter(keys);
            String* key;
            while((key = keyIter.next())) {
                VariableBinding *var, *param;
                var = (VariableBinding*)localBindings.get(*key);
                param = (VariableBinding*)aParams->get(*key);
                if (var && var->getValue() == param->getValue()) {
                    // Don't delete the contained ExprResult since it's
                    // not ours
                    var->setValue(0);
                }
            }
        }
        else {
            // out of memory so we can't get the keys
            // don't delete any variables since it's better we leak than
            // crash
            localBindings.setObjectDeletion(MB_FALSE);
        }
        delete keys;
    }
    
    bindings->pop();
}

void
txXSLTProcessor::processTemplateParams(Node* aXsltTemplate,
                                       Node* aContext,
                                       ProcessorState* aPs,
                                       NamedMap* aActualParams)
{
    if (aXsltTemplate) {
        Node* tmpNode = aXsltTemplate->getFirstChild();
        //-- handle params
        while (tmpNode) {
            unsigned short nodeType = tmpNode->getNodeType();
            if (nodeType == Node::ELEMENT_NODE) {
                txAtom* localName;
                tmpNode->getLocalName(&localName);
                if (tmpNode->getNamespaceID() != kNameSpaceID_XSLT ||
                    localName != txXSLTAtoms::param) {
                    TX_IF_RELEASE_ATOM(localName);
                    break;
                }
                TX_RELEASE_ATOM(localName);

                Element* action = (Element*)tmpNode;
                String name;
                if (!action->getAttr(txXSLTAtoms::name,
                                     kNameSpaceID_None, name)) {
                    String err("missing required name attribute for xsl:param");
                    aPs->receiveError(err, NS_ERROR_FAILURE);
                }
                else {
                    VariableBinding* binding = 0;
                    if (aActualParams) {
                        binding = (VariableBinding*) aActualParams->get((const String&)name);
                    }
                    if (binding) {
                        // the formal parameter has a corresponding actual parameter, use it
                        ExprResult* exprResult = binding->getValue();
                        bindVariable(name, exprResult, MB_FALSE, aPs);
                    }
                    else {
                        // no actual param, use default
                        ExprResult* exprResult = processVariable(aContext, action, aPs);
                        bindVariable(name, exprResult, MB_FALSE, aPs);
                    }
                }
            }
            else if (nodeType == Node::TEXT_NODE ||
                     nodeType == Node::CDATA_SECTION_NODE) {
                if (!XMLUtils::isWhitespace(tmpNode->getNodeValue()))
                    break;
            }
            tmpNode = tmpNode->getNextSibling();
        }
    }
}

void
txXSLTProcessor::processTopLevel(Document* aSource,
                                 Element* aStylesheet,
                                 txListIterator* importFrame,
                                 ProcessorState* aPs)
{
    // Index templates and process top level xslt elements
    NS_ASSERTION(aStylesheet, "processTopLevel called without stylesheet element");
    if (!aStylesheet)
        return;

    ProcessorState::ImportFrame* currentFrame =
        (ProcessorState::ImportFrame*)importFrame->current();

    NS_ASSERTION(currentFrame,
                 "processTopLevel called with no current importframe");
    if (!currentFrame)
        return;

    NS_ASSERTION(aSource, "processTopLevel called without source document");

    MBool importsDone = MB_FALSE;
    Node* node = aStylesheet->getFirstChild();
    while (node && !importsDone) {
        if (node->getNodeType() == Node::ELEMENT_NODE) {
            txAtom* localName;
            node->getLocalName(&localName);
            if (node->getNamespaceID() == kNameSpaceID_XSLT &&
                localName == txXSLTAtoms::import) {
                Element* element = (Element*)node;
                String hrefAttr, href;
                element->getAttr(txXSLTAtoms::href, kNameSpaceID_None,
                                 hrefAttr);
                URIUtils::resolveHref(hrefAttr, element->getBaseURI(),
                                      href);

                // Create a new ImportFrame with correct firstNotImported
                ProcessorState::ImportFrame *nextFrame, *newFrame;
                nextFrame =
                    (ProcessorState::ImportFrame*)importFrame->next();
                newFrame = new ProcessorState::ImportFrame(nextFrame);
                if (!newFrame) {
                    // XXX ErrorReport: out of memory
                    break;
                }

                // Insert frame and process stylesheet
                importFrame->addBefore(newFrame);
                importFrame->previous();
                processInclude(href, aSource, importFrame, aPs);

                // Restore iterator to initial position
                importFrame->previous();
            }
            else {
                importsDone = MB_TRUE;
            }
            TX_IF_RELEASE_ATOM(localName);
        }
        if (!importsDone)
            node = node->getNextSibling();
    }

    while (node) {
        if (node->getNodeType() != Node::ELEMENT_NODE ||
            node->getNamespaceID() != kNameSpaceID_XSLT) {
            node = node->getNextSibling();
            continue;
        }

        txAtom* localName;
        node->getLocalName(&localName);
        Element* element = (Element*)node;
        // xsl:attribute-set
        if (localName == txXSLTAtoms::attributeSet) {
            aPs->addAttributeSet(element, currentFrame);
        }
        // xsl:decimal-format
        else if (localName == txXSLTAtoms::decimalFormat) {
            if (!aPs->addDecimalFormat(element)) {
                // Add error to ErrorObserver
                String fName;
                element->getAttr(txXSLTAtoms::name, kNameSpaceID_None,
                                 fName);
                String err("unable to add ");
                if (fName.isEmpty()) {
                    err.append("default");
                }
                else {
                    err.append("\"");
                    err.append(fName);
                    err.append("\"");
                }
                err.append(" decimal format for xsl:decimal-format");
                aPs->receiveError(err, NS_ERROR_FAILURE);
            }
        }
        // xsl:param
        else if (localName == txXSLTAtoms::param) {
            String name;
            element->getAttr(txXSLTAtoms::name, kNameSpaceID_None,
                             name);
            if (name.isEmpty()) {
                String err("missing required name attribute for xsl:param");
                aPs->receiveError(err, NS_ERROR_FAILURE);
                break;
            }
            ExprResult* exprResult = processVariable(aSource, element, aPs);
            bindVariable(name, exprResult, MB_TRUE, aPs);
        }
        // xsl:import
        else if (localName == txXSLTAtoms::import) {
            String err("xsl:import only allowed at top of stylesheet");
            aPs->receiveError(err, NS_ERROR_FAILURE);
        }
        // xsl:include
        else if (localName == txXSLTAtoms::include) {
            String hrefAttr, href;
            element->getAttr(txXSLTAtoms::href, kNameSpaceID_None,
                             hrefAttr);
            URIUtils::resolveHref(hrefAttr, element->getBaseURI(),
                                  href);

            processInclude(href, aSource, importFrame, aPs);
        }
        // xsl:key
        else if (localName == txXSLTAtoms::key) {
            if (!aPs->addKey(element)) {
                String name;
                element->getAttr(txXSLTAtoms::name, kNameSpaceID_None,
                                 name);
                String err("error adding key '");
                err.append(name);
                err.append("'");
                aPs->receiveError(err, NS_ERROR_FAILURE);
            }
        }
        // xsl:output
        else if (localName == txXSLTAtoms::output) {
            txOutputFormat& format = currentFrame->mOutputFormat;
            String attValue;

            if (element->getAttr(txXSLTAtoms::method, kNameSpaceID_None,
                                 attValue)) {
                if (attValue.isEqual("html")) {
                    format.mMethod = eHTMLOutput;
                }
                else if (attValue.isEqual("text")) {
                    format.mMethod = eTextOutput;
                }
                else {
                    format.mMethod = eXMLOutput;
                }
            }

            if (element->getAttr(txXSLTAtoms::version, kNameSpaceID_None,
                                 attValue)) {
                format.mVersion = attValue;
            }

            if (element->getAttr(txXSLTAtoms::encoding, kNameSpaceID_None,
                                 attValue)) {
                format.mEncoding = attValue;
            }

            if (element->getAttr(txXSLTAtoms::omitXmlDeclaration,
                                 kNameSpaceID_None, attValue)) {
                format.mOmitXMLDeclaration = attValue.isEqual(YES_VALUE) ? eTrue : eFalse;
            }

            if (element->getAttr(txXSLTAtoms::standalone, kNameSpaceID_None,
                                 attValue)) {
                format.mStandalone = attValue.isEqual(YES_VALUE) ? eTrue : eFalse;
            }

            if (element->getAttr(txXSLTAtoms::doctypePublic,
                                 kNameSpaceID_None, attValue)) {
                format.mPublicId = attValue;
            }

            if (element->getAttr(txXSLTAtoms::doctypeSystem,
                                 kNameSpaceID_None, attValue)) {
                format.mSystemId = attValue;
            }

            if (element->getAttr(txXSLTAtoms::cdataSectionElements,
                                 kNameSpaceID_None, attValue)) {
                txTokenizer tokens(attValue);
                String token;
                while (tokens.hasMoreTokens()) {
                    tokens.nextToken(token);
                    if (!XMLUtils::isValidQName(token)) {
                        break;
                    }

                    String namePart;
                    XMLUtils::getPrefix(token, namePart);
                    txAtom* nameAtom = TX_GET_ATOM(namePart);
                    PRInt32 nsID = element->lookupNamespaceID(nameAtom);
                    TX_IF_RELEASE_ATOM(nameAtom);
                    if (nsID == kNameSpaceID_Unknown) {
                        // XXX ErrorReport: unknown prefix
                        break;
                    }
                    XMLUtils::getLocalPart(token, namePart);
                    nameAtom = TX_GET_ATOM(namePart);
                    if (!nameAtom) {
                        // XXX ErrorReport: out of memory
                        break;
                    }
                    txExpandedName* qname = new txExpandedName(nsID, nameAtom);
                    TX_RELEASE_ATOM(nameAtom);
                    if (!qname) {
                        // XXX ErrorReport: out of memory
                        break;
                    }
                    format.mCDATASectionElements.add(qname);
                }
            }

            if (element->getAttr(txXSLTAtoms::indent, kNameSpaceID_None,
                                 attValue)) {
                format.mIndent = attValue.isEqual(YES_VALUE) ? eTrue : eFalse;
            }

            if (element->getAttr(txXSLTAtoms::mediaType, kNameSpaceID_None,
                                 attValue)) {
                format.mMediaType = attValue;
            }
        }
        // xsl:template
        else if (localName == txXSLTAtoms::_template) {
            aPs->addTemplate(element, currentFrame);
        }
        // xsl:variable
        else if (localName == txXSLTAtoms::variable) {
            String name;
            element->getAttr(txXSLTAtoms::name, kNameSpaceID_None,
                             name);
            if (name.isEmpty()) {
                String err("missing required name attribute for xsl:variable");
                aPs->receiveError(err, NS_ERROR_FAILURE);
                break;
            }
            ExprResult* exprResult = processVariable(aSource, element, aPs);
            bindVariable(name, exprResult, MB_FALSE, aPs);
        }
        // xsl:preserve-space
        else if (localName == txXSLTAtoms::preserveSpace) {
            String elements;
            if (!element->getAttr(txXSLTAtoms::elements,
                                  kNameSpaceID_None, elements)) {
                //-- add error to ErrorObserver
                String err("missing required 'elements' attribute for ");
                err.append("xsl:preserve-space");
                aPs->receiveError(err, NS_ERROR_FAILURE);
            }
            else {
                aPs->shouldStripSpace(elements, element,
                                      MB_FALSE,
                                      currentFrame);
            }
        }
        // xsl:strip-space
        else if (localName == txXSLTAtoms::stripSpace) {
            String elements;
            if (!element->getAttr(txXSLTAtoms::elements,
                                  kNameSpaceID_None, elements)) {
                //-- add error to ErrorObserver
                String err("missing required 'elements' attribute for ");
                err.append("xsl:strip-space");
                aPs->receiveError(err, NS_ERROR_FAILURE);
            }
            else {
                aPs->shouldStripSpace(elements, element,
                                      MB_TRUE,
                                      currentFrame);
            }
        }
        TX_IF_RELEASE_ATOM(localName);
        node = node->getNextSibling();
    }
}

ExprResult*
txXSLTProcessor::processVariable(Node* aNode,
                                 Element* aXsltVariable,
                                 ProcessorState* aPs)
{

    if (!aXsltVariable) {
        return new StringResult("unable to process variable");
    }

    //-- check for select attribute
    if (aXsltVariable->hasAttr(txXSLTAtoms::select, kNameSpaceID_None)) {
        Expr* expr = aPs->getExpr(aXsltVariable, ProcessorState::SelectAttr);
        if (!expr)
            return new StringResult("unable to process variable");
        return expr->evaluate(aPs->getEvalContext());
    }
    else if (aXsltVariable->hasChildNodes()) {
        txResultTreeFragment* rtf = new txResultTreeFragment();
        if (!rtf)
            // XXX ErrorReport: Out of memory
            return 0;
        txXMLEventHandler* previousHandler = mResultHandler;
        Document* rtfDoc = aPs->getRTFDocument();
        if (!rtfDoc) {
            // XXX ErrorReport: Out of memory
            return 0;
        }
        txRtfHandler rtfHandler(rtfDoc, rtf);
        mResultHandler = &rtfHandler;
        processChildren(aNode, aXsltVariable, aPs);
        //NS_ASSERTION(previousHandler, "Setting mResultHandler to NULL!");
        mResultHandler = previousHandler;
        return rtf;
    }
    else {
        return new StringResult("");
    }
}

void
txXSLTProcessor::startElement(const String& aName,
                              const PRInt32 aNsID,
                              ProcessorState* aPs)
{
    if (!mHaveDocumentElement && (mResultHandler == mOutputHandler)) {
        txOutputFormat* format = aPs->getOutputFormat();
        if (format->mMethod == eMethodNotSet) {
            // XXX Should check for whitespace-only sibling text nodes
            if ((aNsID == kNameSpaceID_None) &&
                aName.isEqualIgnoreCase(String("html"))) {
                // Switch to html output mode according to the XSLT spec.
                format->mMethod = eHTMLOutput;
            }
            else {
                format->mMethod = eXMLOutput;
            }
            mOutputHandler = aPs->getProcessorHelper()->getOutputHandler(format->mMethod);
            if (!mOutputHandler) {
                // XXX Error
                return;
            }
            mOutputHandler->setOutputFormat(format);
            mResultHandler = mOutputHandler;
        }
        mHaveDocumentElement = MB_TRUE;
    }
    NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
    mResultHandler->startElement(aName, aNsID);
}

void
txXSLTProcessor::transform(Node* aNode, ProcessorState* aPs)
{
    txListIterator frameIter(aPs->getImportFrames());
    ProcessorState::ImportFrame* frame;
    txOutputFormat* outputFormat = aPs->getOutputFormat();
    while ((frame = (ProcessorState::ImportFrame*)frameIter.next())) {
        outputFormat->merge(frame->mOutputFormat);
    }

    mOutputHandler = aPs->getProcessorHelper()->getOutputHandler(outputFormat->mMethod);
    if (!mOutputHandler) {
        return;
    }
    mResultHandler = mOutputHandler;
    mOutputHandler->setOutputFormat(outputFormat);
    mHaveDocumentElement = MB_FALSE;
    mOutputHandler->startDocument();

    txExpandedName nullMode;
    process(aNode, nullMode, aPs);

    mOutputHandler->endDocument();
}


void
txXSLTProcessor::xslCopy(Node* aNode, Element* aAction, ProcessorState* aPs)
{
    if (!aNode)
        return;

    switch (aNode->getNodeType()) {
        case Node::DOCUMENT_NODE:
        {
            // Just process children
            processChildren(aNode, aAction, aPs);
            break;
        }
        case Node::ELEMENT_NODE:
        {
            Element* element = (Element*)aNode;
            String nodeName = element->getNodeName();
            PRInt32 nsID = element->getNamespaceID();

            startElement(nodeName, nsID, aPs);
            // XXX copy namespace attributes once we have them
            processAttributeSets(aAction, aNode, aPs);
            processChildren(aNode, aAction, aPs);
            NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
            mResultHandler->endElement(nodeName, nsID);
            break;
        }
        default:
        {
            copyNode(aNode, aPs);
        }
    }
}

void
txXSLTProcessor::xslCopyOf(ExprResult* aExprResult, ProcessorState* aPs)
{
    if (!aExprResult)
        return;

    switch (aExprResult->getResultType()) {
        case ExprResult::NODESET:
        {
            NodeSet* nodes = (NodeSet*)aExprResult;
            int i;
            for (i = 0; i < nodes->size(); i++) {
                Node* node = nodes->get(i);
                copyNode(node, aPs);
            }
            break;
        }
        default:
        {
            String value;
            aExprResult->stringValue(value);
            NS_ASSERTION(mResultHandler, "mResultHandler must not be NULL!");
            mResultHandler->characters(value);
        }
    }
}
