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
 * (C) 1999 Keith Visco. All Rights Reserved.
 *
 * Contributor(s):
 * Keith Visco, kvisco@ziplink.net
 *    -- original author.
 *
 */


#ifndef TRANSFRMX_XSLTPROCESSOR_H
#define TRANSFRMX_XSLTPROCESSOR_H

#include "ProcessorState.h"

class txXMLEventHandler;
class txOutputXMLEventHandler;

/*
 * A class for Processing XSLT Stylesheets
 */
class txXSLTProcessor
{
public:
#ifdef TX_EXE
    /*
     * Initialisation and shutdown routines for standalone
     * Allocate and free static atoms.
     */
    static MBool txInit();
    static MBool txShutdown();
#endif

    /*
     * Creates a new txXSLTProcessor.
     */
    txXSLTProcessor();

    /*
     * Default destructor for txXSLTProcessor.
     */
    virtual ~txXSLTProcessor();

protected:
    /*
     * Returns the name of this XSLT processor.
     */
    String& getAppName();

    /*
     * Returns the version of this XSLT processor.
     */
    String& getAppVersion();

    /*
     * XXX.
     *
     * @param aMethod YYY
     */
    virtual txOutputXMLEventHandler* getOutputHandler(txOutputMethod aMethod) = 0;

    /*
     * XXX.
     *
     * @param aMethod YYY
     */
    virtual void logMessage(const String& aMessage) = 0;

    /*
     * Processes a stylesheet document.
     *
     * @param aSource the source document
     * @param aStylesheet the stylesheet document to process
     * @param aPs the current ProcessorState
     */
    nsresult processStylesheet(Document* aSource,
                               Document* aStylesheet,
                               ProcessorState* aPs)
    {
        txListIterator importFrame(aPs->getImportFrames());
        importFrame.addAfter(new ProcessorState::ImportFrame(0));
        if (!importFrame.next()) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
        processStylesheet(aSource, aStylesheet, &importFrame, aPs);
        return NS_OK;
    }

    /*
     * Processes a stylesheet element.
     *
     * @param aSource the source document
     * @param aStylesheet the stylesheet element to process
     * @param aPs the current ProcessorState
     */
    nsresult processTopLevel(Document* aSource,
                             Element* aStylesheet,
                             ProcessorState* aPs)
    {
        txListIterator importFrame(aPs->getImportFrames());
        importFrame.addAfter(new ProcessorState::ImportFrame(0));
        if (!importFrame.next()) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
        processTopLevel(aSource, aStylesheet, &importFrame, aPs);
        return NS_OK;
    }

    /*
     * Transforms a node.
     *
     * @param aNode the node to transform
     * @param aPs the current ProcessorState
     */
    void transform(Node* aNode, ProcessorState* aPs);

private:
    /*
     * A warning message used by all templates that do not allow non character
     * data to be generated
     */
    static const String NON_TEXT_TEMPLATE_WARNING;

    /*
     * Binds the given Variable
     *
     * @param aName YYY
     * @param aValue YYY
     * @param aAllowShadowing YYY
     * @param aPs the current ProcessorState
     */
    void bindVariable(String& aName,
                      ExprResult* aValue,
                      MBool aAllowShadowing,
                      ProcessorState* aPs);

    /*
     * Copy a node. For document nodes, copy the children.
     *
     * @param aNode YYY
     * @param aPs the current ProcessorState
     */
    void copyNode(Node* aNode, ProcessorState* aPs);

    /*
     * XXX.
     *
     * @param aNode YYY
     * @param aMode YYY
     * @param aPs the current ProcessorState
     */
    void process(Node* aNode,
                 const String& aMode,
                 ProcessorState* aPs);

    /*
     * XXX.
     *
     * @param aNode YYY
     * @param aXsltAction YYY
     * @param aPs the current ProcessorState
     */
    void processAction(Node* aNode,
                       Node* aXsltAction,
                       ProcessorState* aPs);

    /*
     * Processes the attribute sets specified in the use-attribute-sets attribute
     * of the element specified in aElement
     *
     * @param aElement YYY
     * @param aNode YYY
     * @param aPs the current ProcessorState
     */
    void processAttributeSets(Element* aElement,
                              Node* aNode,
                              ProcessorState* aPs);

    /*
     * Processes the children of the specified element using the given context node
     * and ProcessorState.
     *
     * @param aNode the context node
     * @param aXsltElement the template to be processed. Must be != NULL
     * @param aPs the current ProcessorState
     */
    void processChildren(Node* aNode,
                         Element* aXsltElement,
                         ProcessorState* aPs);

    /*
     * XXX.
     *
     * @param aNode YYY
     * @param aElement YYY
     * @param aPs the current ProcessorState
     * @param aOnlyText YYY
     * @param aValue YYY
     */
    void processChildrenAsValue(Node* aNode, 
                                Element* aElement,
                                ProcessorState* aPs,
                                MBool aOnlyText,
                                String& aValue);

    /*
     * Invokes the default template for the specified node
     *
     * @param aNode context node
     * @param aPs the current ProcessorState
     * @param aMode template mode
     */
    void processDefaultTemplate(Node* aNode,
                                ProcessorState* aPs,
                                const String& aMode);

    /*
     * Processes an include or import stylesheet.
     *
     * @param aHref URI of stylesheet to process
     * @param aSource source document
     * @param aImportFrame current importFrame iterator
     * @param aPs the current ProcessorState
     */
    void processInclude(String& aHref,
                        Document* aSource,
                        txListIterator* aImportFrame,
                        ProcessorState* aPs);

    /*
     * XXX.
     *
     * @param aXslTemplate YYY
     * @param aNode YYY
     * @param aParams YYY
     * @param aMode YYY
     * @param aFrame YYY
     * @param aPs the current ProcessorState
     */
    void processMatchedTemplate(Node* aXslTemplate,
                                Node* aNode,
                                NamedMap* aParams,
                                const String& aMode,
                                ProcessorState::ImportFrame* aFrame,
                                ProcessorState* aPs);

    /*
     * Processes the xsl:with-param elements of the given xsl action
     *
     * @param aXsltAction YYY
     * @param aContext YYY
     * @param aPs the current ProcessorState
     */
    NamedMap* processParameters(Element* aXsltAction,
                                Node* aContext,
                                ProcessorState* aPs);

    /*
     * XXX.
     *
     * @param aSource YYY
     * @param aStylesheet YYY
     * @param aImportFrame YYY
     * @param aPs the current ProcessorState
     */
    void processStylesheet(Document* aSource,
                           Document* aStylesheet,
                           txListIterator* aImportFrame,
                           ProcessorState* aPs);

    /*
     * XXX.
     *
     * @param aNode YYY
     * @param aXsltTemplate YYY
     * @param aPs the current ProcessorState
     * @param aActualParams YYY
     */
    void processTemplate(Node* aNode,
                         Node* aXsltTemplate,
                         ProcessorState* aPs,
                         NamedMap* aActualParams = NULL);

    /*
     * XXX.
     *
     * @param aXsltTemplate YYY
     * @param aContext YYY
     * @param aPs the current ProcessorState
     * @param aActualParams YYY
     */
    void processTemplateParams(Node* aXsltTemplate,
                               Node* aContext,
                               ProcessorState* aPs,
                               NamedMap* aActualParams);

    /*
     * XXX.
     *
     * @param aSource YYY
     * @param aStylesheet YYY
     * @param aImportFrame YYY
     * @param aPs the current ProcessorState
     */
    void processTopLevel(Document* aSource,
                         Element* aStylesheet,
                         txListIterator* aImportFrame,
                         ProcessorState* aPs);

    /*
     * XXX.
     *
     * @param aNode YYY
     * @param aXsltVariable YYY
     * @param aPs the current ProcessorState
     */
    ExprResult* processVariable(Node* aNode,
                                Element* aXsltVariable,
                                ProcessorState* aPs);

    /*
     * XXX.
     *
     * @param aMethod YYY
     * @param aPs the current ProcessorState
     */
    void startElement(const String& aName,
                      const PRInt32 aNsID,
                      ProcessorState* aPs);

    /*
     * Performs the xsl:copy action as specified in the XSLT specification
     *
     * @param aNode YYY
     * @param aAction YYY
     * @param aPs the current ProcessorState
     */
    void xslCopy(Node* aNode,
                 Element* aAction,
                 ProcessorState* aPs);

    /*
     * Performs the xsl:copy-of action as specified in the XSLT specification
     *
     * @param aExprResult YYY
     * @param aPs the current ProcessorState
     */
    void xslCopyOf(ExprResult* aExprResult,
                   ProcessorState* aPs);

    /*
     * The version of XSL which this Processes
     */
    String mXsltVersion;

    /*
     * Used as default expression for some elements
     */
    Expr* mNodeExpr;

    /*
     * Fatal ErrorObserver
     */
    SimpleErrorObserver mFatalObserver;

    /*
     * An expression parser for creating AttributeValueTemplates
     */
    ExprParser mExprParser;

    txOutputXMLEventHandler* mOutputHandler;
    txXMLEventHandler* mResultHandler;
    MBool mHaveDocumentElement;
    Stack mAttributeSetStack;
    String mAppName;
    String mAppVersion;
};

#endif
