/*
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
 */

#include "txStandaloneXSLTProcessor.h"

txStandaloneXSLTProcessor::txStandaloneXSLTProcessor() :
    mStandaloneOutputHandler(0)
{
}

txStandaloneXSLTProcessor::~txStandaloneXSLTProcessor()
{
}

/*
 * Reads an XML document from the given XML input stream. The
 * XML document is processed using the associated XSL document
 * retrieved from the XML document's Stylesheet Processing Instruction,
 * otherwise an empty document will be returned.
 */
Document* txStandaloneXSLTProcessor::process(istream& xmlInput)
{
    XMLParser xmlParser;

    // Read in XML Document
    Document* xmlDoc = xmlParser.parse(xmlInput);
    if (!xmlDoc) {
        String err("error reading XML document: ");
        err.append(xmlParser.getErrorString());
        notifyError(err, ErrorObserver::FATAL);
        return 0;
    }

    Document* result = process(*xmlDoc);

    delete xmlDoc;
    return result;
}

/*
 * Processes the given XML Document, the XSL stylesheet
 * will be retrieved from the XML Stylesheet Processing instruction,
 * otherwise an empty document will be returned.
 */
Document* txStandaloneXSLTProcessor::process(Document& xmlDocument)
{
    // Look for stylesheet PIs
    String href;
    String errMsg;
    getHrefFromStylesheetPI(xmlDocument, href);
    istream* xslInput = URIUtils::getInputStream(href, errMsg);

    // Read in XSL document
    Document* xslDoc = 0;
    if (xslInput) {
        xslDoc = xmlParser.parse(*xslInput);
        delete xslInput;
    }
    if (!xslDoc) {
        String err("error reading XSL stylesheet document: ");
        err.append(xmlParser.getErrorString());
        notifyError(err, ErrorObserver::FATAL);
        return 0;
    }

    Document* result = transform(xmlDocument, *xslDoc, NULL);

    delete xslDoc;
    return result;
}

/*
 * Reads an XML Document from the given XML input stream, and
 * processes the document using the XSL document derived from
 * the given XSL input stream.
 */
Document* txStandaloneXSLTProcessor::process(istream& xmlInput,
                                             istream& xslInput)
{
    XMLParser xmlParser;

    // Read in XML Document
    Document* xmlDoc = xmlParser.parse(xmlInput);
    if (!xmlDoc) {
        String err("error reading XML document: ");
        err.append(xmlParser.getErrorString());
        notifyError(err, ErrorObserver::FATAL);
        return 0;
    }

    // Read in XSL document
    Document* xslDoc = xmlParser.parse(xslInput);
    if (!xslDoc) {
        String err("error reading XSL stylesheet document: ");
        err.append(xmlParser.getErrorString());
        notifyError(err, ErrorObserver::FATAL);
        delete xmlDoc;
        return 0;
    }

    Document* result = transform(*xmlDoc, *xslDoc, NULL);

    delete xmlDoc;
    delete xslDoc;
    return result;
}

/*
 * Processes the given XML Document using the given XSL document
 * and returns the result tree
 */
Document* txStandaloneXSLTProcessor::process(Document& xmlDocument,
                                             Document& xslDocument)
{
    Document* result = transform(xmlDocument, xslDocument, NULL);
    return result;
}

/*
 * Reads an XML Document from the given XML input stream.
 * The XSL Stylesheet is obtained from the XML Documents stylesheet PI.
 * If no Stylesheet is found, an empty document will be the result;
 * otherwise the XML Document is processed using the stylesheet.
 * The result tree is printed to the given ostream argument,
 * will not close the ostream argument
 */
void txStandaloneXSLTProcessor::process(istream& xmlInput,
                                        ostream& out)
{
    XMLParser xmlParser;

    // Read in XML Document
    Document* xmlDoc = xmlParser.parse(xmlInput);
    if (!xmlDoc) {
        String err("error reading XML document: ");
        err.append(xmlParser.getErrorString());
        notifyError(err, ErrorObserver::FATAL);
        return 0;
    }

    process(*xmlDoc, out);

    delete xmlDoc;
}

/*
 * Processes the given XML Document, the XSL stylesheet
 * will be retrieved from the XML Stylesheet Processing instruction,
 * otherwise an empty document will be returned.
 */
void txStandaloneXSLTProcessor::process(Document& xmlDocument,
                                        ostream& out)
{
    // Look for stylesheet PIs
    String href;
    String errMsg;
    getHrefFromStylesheetPI(xmlDocument, href);
    istream* xslInput = URIUtils::getInputStream(href, errMsg);

    // Read in XSL document
    Document* xslDoc = 0;
    if (xslInput) {
        xslDoc = xmlParser.parse(*xslInput);
        delete xslInput;
    }
    if (!xslDoc) {
        String err("error reading XSL stylesheet document: ");
        err.append(xmlParser.getErrorString());
        notifyError(err, ErrorObserver::FATAL);
        return 0;
    }

    process(xmlDocument, *xslDoc, out);

    delete xslDoc;
}

/*
 * Reads an XML Document from the given XML input stream, and
 * processes the document using the XSL document derived from
 * the given XSL input stream.
 * The result tree is printed to the given ostream argument,
 * will not close the ostream argument
 */
void txStandaloneXSLTProcessor::process(istream& xmlInput,
                                        istream& xslInput,
                                        ostream& out)
{
    XMLParser xmlParser;

    // Read in XML Document
    Document* xmlDoc = xmlParser.parse(xmlInput);
    if (!xmlDoc) {
        String err("error reading XML document: ");
        err.append(xmlParser.getErrorString());
        notifyError(err, ErrorObserver::FATAL);
        return 0;
    }

    // Read in XSL document
    Document* xslDoc = xmlParser.parse(xslInput);
    if (!xslDoc) {
        String err("error reading XSL stylesheet document: ");
        err.append(xmlParser.getErrorString());
        notifyError(err, ErrorObserver::FATAL);
        delete xmlDoc;
        return 0;
    }

    process(*xmlDoc, *xslDoc, out);

    delete xmlDoc;
    delete xslDoc;
}

/*
 * Processes the given XML Document using the given XSL document
 * and prints the results to the given ostream argument
 */
void txStandaloneXSLTProcessor::process(Document& xmlDocument,
                                        Document& xslDocument,
                                        ostream& out)
{
    Document* result = transform(xmlDocument, xslDocument, out);

    delete result;
}

/*
 * Private worker function
 */
Document* txStandaloneXSLTProcessor::transform(Document& aSource,
                                               Document& aStylesheet,
                                               ostream& out)
{
    Document* result = new Document;

    // Start of block to ensure the destruction of the ProcessorState
    // before the destruction of the result document.
    {
        // Create a new ProcessorState
        Document* stylesheetDoc = 0;
        Element* stylesheetElem = 0;
        if (aStylesheet.getNodeType() == Node::DOCUMENT_NODE) {
            stylesheetDoc = (Document*)&aStylesheet;
        }
        else {
            stylesheetElem = (Element*)&aStylesheet;
            stylesheetDoc = aStylesheet.getOwnerDocument();
        }
        ProcessorState ps(&aSource, stylesheetDoc, result);

        // Add error observers
        txListIterator iter(&mErrorObservers);
        while (iter.hasNext()) {
            ps.addErrorObserver(*(ErrorObserver*)iter.next());
        }

        txSingleNodeContext evalContext(&aXMLDocument, &ps);
        ps.setEvalContext(&evalContext);

        // Index templates and process top level xsl elements
        nsresult rv;
        if (stylesheetElem) {
            rv = processTopLevel(&aXMLDocument, stylesheetElem, &ps);
        }
        else {
            rv = processStylesheet(&aXMLDocument, stylesheetDoc, &ps);
        }
        if (NS_FAILED(rv)) {
            delete result;
            return 0;
        }

        mOut = &aOut;

        // Process root of XML source document
        transform(&aXMLDocument, &ps);

        delete mStandaloneOutputHandler;
        mStandaloneOutputHandler = 0;
    }
    // End of block to ensure the destruction of the ProcessorState
    // before the destruction of the result document.

    // Return result Document
    return result;
}

/*
 * Parses all XML Stylesheet PIs associated with the
 * given XML document. If any stylesheet PIs are found with
 * type="text/xsl" the href psuedo attribute value will be
 * added to the given href argument. If multiple text/xsl stylesheet PIs
 * are found, the one closest to the end of the document is used.
 */
void txStandaloneXSLTProcessor::getHrefFromStylesheetPI(Document& xmlDocument,
                                                        String& href)
{
    Node* node = xmlDocument.getFirstChild();
    String type;
    String tmpHref;
    while (node) {
        if (node->getNodeType() == Node::PROCESSING_INSTRUCTION_NODE) {
            String target = ((ProcessingInstruction*)node)->getTarget();
            if (STYLESHEET_PI.isEqual(target) || STYLESHEET_PI_OLD.isEqual(target)) {
                String data = ((ProcessingInstruction*)node)->getData();
                type.clear();
                tmpHref.clear();
                parseStylesheetPI(data, type, tmpHref);
                if (XSL_MIME_TYPE.isEqual(type)) {
                    href.clear();
                    URIUtils::resolveHref(tmpHref, node->getBaseURI(), href);
                }
            }
        }
        node = node->getNextSibling();
    }

}

/*
 * Parses the contents of data, and returns the type and href pseudo attributes
 */
void txStandaloneXSLTProcessor::parseStylesheetPI(String& data,
                                                  String& type,
                                                  String& href)
{
    Int32 size = data.length();
    NamedMap bufferMap;
    bufferMap.put("type", &type);
    bufferMap.put("href", &href);
    int ccount = 0;
    MBool inLiteral = MB_FALSE;
    char matchQuote = '"';
    String sink;
    String* buffer = &sink;

    for (ccount = 0; ccount < size; ccount++) {
        char ch = data.charAt(ccount);
        switch ( ch ) {
            case ' ':
                if (inLiteral) {
                    buffer->append(ch);
                }
                break;
            case '=':
                if (inLiteral) {
                    buffer->append(ch);
                }
                else if (buffer->length() > 0) {
                    buffer = (String*)bufferMap.get(*buffer);
                    if (!buffer) {
                        sink.clear();
                        buffer = &sink;
                    }
                }
                break;
            case '"':
            case '\'':
                if (inLiteral) {
                    if (matchQuote == ch) {
                        inLiteral = MB_FALSE;
                        sink.clear();
                        buffer = &sink;
                    }
                    else {
                        buffer->append(ch);
                    }
                }
                else {
                    inLiteral = MB_TRUE;
                    matchQuote = ch;
                }
                break;
            default:
                buffer->append(ch);
                break;
        }
    }
}

txOutputXMLEventHandler*
txStandaloneXSLTProcessor::getOutputHandler(txOutputMethod aMethod)
{
    if (mStandaloneOutputHandler) {
        if (aMethod == eHTMLOutput || aMethod == eXMLOutput) {
            txUnknownHandler* oldHandler =
                (txUnknownHandler*)mStandaloneOutputHandler;
            if (aMethod == eHTMLOutput) {
                mStandaloneOutputHandler = new txHTMLOutput();
            }
            else {
                mStandaloneOutputHandler = new txXMLOutput();
            }
            NS_ASSERTION(mStandaloneOutputHandler,
                         "Setting mStandaloneOutputHandler to NULL!");
            mStandaloneOutputHandler->setOutputStream(mOut);
            txOutputFormat* format = oldHandler->getOutputFormat();
            format->mMethod = aMethod;
            mStandaloneOutputHandler->setOutputFormat(format);
            oldHandler->flush(mStandaloneOutputHandler);
            delete oldHandler;
            return mStandaloneOutputHandler;
        }
        delete mStandaloneOutputHandler;
        mStandaloneOutputHandler = 0;
    }
    switch (aMethod) {
        case eXMLOutput:
        {
            mStandaloneOutputHandler = new txXMLOutput();
            break;
        }
        case eHTMLOutput:
        {
            mStandaloneOutputHandler = new txHTMLOutput();
            break;
        }
        case eTextOutput:
        {
            mStandaloneOutputHandler = new txTextOutput();
            break;
        }
        case eMethodNotSet:
        {
            mStandaloneOutputHandler = new txUnknownHandler(this);
            break;
        }
    }
    if (mOut) {
        mStandaloneOutputHandler->setOutputStream(mOut);
    }
}

void
txStandaloneXSLTProcessor::logMessage(const String& aMessage)
{
    cout << "xsl:message - "<< aMessage << endl;
}
