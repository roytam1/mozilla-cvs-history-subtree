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
 * (C) 1999 Keith Visco. All Rights Reserved.
 *
 * Contributor(s):
 * Keith Visco, kvisco@ziplink.net
 *    -- original author.
 *
 */


#ifndef TRANSFRMX_TXSTANDALONEXSLTPROCESSOR_H
#define TRANSFRMX_TXSTANDALONEXSLTPROCESSOR_H

#include "XSLTProcessor.h"
#ifndef __BORLANDC__
#include <iostream.h>
#include <fstream.h>
#endif

/*
 * txStandaloneXSLTProcessor is a front-end to the XSLT Processor
 */

class txStandaloneXSLTProcessor : public txXSLTProcessor
{
public:
    /*
     * Creates a new txStandaloneXSLTProcessor
     */
    txStandaloneXSLTProcessor();

    /*
     * Default destructor for txStandaloneXSLTProcessor
     */
    virtual ~txStandaloneXSLTProcessor();

    /*
     * Methods that return a document
     */

    /*
     * Reads an XML document from the given XML input stream. The
     * XML document is processed using the associated XSL document
     * retrieved from the XML document's Stylesheet Processing Instruction,
     * otherwise an empty document will be returned.
     *
     * @param xmlInput
     *
     * @returns the result tree.
     */
    Document* process(istream& xmlInput)

    /*
     * Processes the given XML Document, the XSL stylesheet
     * will be retrieved from the XML Stylesheet Processing instruction,
     * otherwise an empty document will be returned.
     *
     * @param xmlDocument the XML document to process
     *
     * @returns the result tree.
     */
    Document* process(Document& xmlDocument);

    /*
     * Reads an XML Document from the given XML input stream, and
     * processes the document using the XSL document derived from
     * the given XSL input stream.
     *
     * @param xmlInput
     * @param xslInput
     *
     * @returns the result tree.
     */
    Document* process(istream& xmlInput,
                      istream& xslInput);

    /*
     * Processes the given XML Document using the given XSL document
     *
     * @returns the result tree.
     */
    Document* process(Document& xmlDocument,
                      Document& xslDocument);

    /*
     * Methods that print the result to a stream
     */

    /*
     * Reads an XML Document from the given XML input stream.
     * The XSL Stylesheet is obtained from the XML Documents stylesheet PI.
     * If no Stylesheet is found, an empty document will be the result;
     * otherwise the XML Document is processed using the stylesheet.
     * The result tree is printed to the given ostream argument,
     * will not close the ostream argument
     */
    void process(istream& xmlInput,
                 ostream& out);

    /*
     * Processes the given XML Document, the XSL stylesheet
     * will be retrieved from the XML Stylesheet Processing instruction,
     * otherwise an empty document will be returned.
     */
    void process(Document& xmlDocument,
                 ostream& out)

    /*
     * Reads an XML Document from the given XML input stream, and
     * processes the document using the XSL document derived from
     * the given XSL input stream.
     * The result tree is printed to the given ostream argument,
     * will not close the ostream argument
     */
    void process(istream& xmlInput,
                 istream& xslInput,
                 ostream& out);

    /*
     * Processes the given XML Document using the given XSL document.
     * The result tree is printed to the given ostream argument,
     * will not close the ostream argument
     */
    void process(Document& xmlDocument,
                 Document& xslDocument,
                 ostream& out);

private:
    virtual txOutputXMLEventHandler* getOutputHandler(txOutputMethod aMethod);
    virtual void logMessage(const String& aMessage);

    Document* transform(Document& xmlDocument,
                        Document& xslDocument,
                        ostream& out)

    /*
     * Parses all XML Stylesheet PIs associated with the
     * given XML document. If any stylesheet PIs are found with
     * type="text/xsl" the href psuedo attribute value will be
     * added to the given href argument. If multiple text/xsl stylesheet PIs
     * are found, the one closest to the end of the document is used.
     */
    void getHrefFromStylesheetPI(Document& xmlDocument,
                                 String& href);

    /*
     * Parses the contents of data, and returns the type and href psuedo attributes
     */
    void parseStylesheetPI(String& data,
                           String& type,
                           String& href);

    txOutputXMLEventHandler* mStandaloneOutputHandler;
    ostream* mOut;
};

#endif
