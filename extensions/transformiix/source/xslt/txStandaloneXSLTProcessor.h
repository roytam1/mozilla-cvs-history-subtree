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

#ifndef TRANSFRMX_TXSTANDALONEXSLTPROCESSOR_H
#define TRANSFRMX_TXSTANDALONEXSLTPROCESSOR_H

#include "XSLTProcessor.h"
#ifndef __BORLANDC__
#include <iostream.h>
#include <fstream.h>
#endif

class txStreamXMLEventHandler;

/*
 * txStandaloneXSLTProcessor is a front-end to the XSLT Processor
 */

class txStandaloneXSLTProcessor : public txXSLTProcessor
{
public:
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
    static void process(istream& xmlInput, ostream& out);

    /*
     * Processes the given XML Document, the XSL stylesheet
     * will be retrieved from the XML Stylesheet Processing instruction,
     * otherwise an empty document will be returned.
     */
    static void process(Document& xmlDocument, ostream& out);

    /*
     * Reads an XML Document from the given XML input stream, and
     * processes the document using the XSL document derived from
     * the given XSL input stream.
     * The result tree is printed to the given ostream argument,
     * will not close the ostream argument
     */
    static void process(istream& xmlInput,
                 istream& xslInput,
                 ostream& out);

    /*
     * Processes the given XML Document using the given XSL document.
     * The result tree is printed to the given ostream argument,
     * will not close the ostream argument
     */
    static void process(Document& xmlDocument,
                 Document& xslDocument,
                 ostream& out);

private:
    static Document* transform2(Document& aSource,
                        Node& aStylesheet,
                        ostream& out);

    /*
     * Parses all XML Stylesheet PIs associated with the
     * given XML document. If any stylesheet PIs are found with
     * type="text/xsl" the href psuedo attribute value will be
     * added to the given href argument. If multiple text/xsl stylesheet PIs
     * are found, the one closest to the end of the document is used.
     */
    static void getHrefFromStylesheetPI(Document& xmlDocument,
                                 String& href);

    /*
     * Parses the contents of data, and returns the type and href psuedo attributes
     */
    static void parseStylesheetPI(String& data,
                           String& type,
                           String& href);
};

#endif
