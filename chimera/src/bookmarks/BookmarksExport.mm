/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
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
 *    Simon Fraser <sfraser@netscape.com>
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <Carbon/Carbon.h>

#import "BookmarksExport.h"
#import "BookmarksService.h"

#include "nsCOMPtr.h"
#include "nsIFileStreams.h"
#include "nsILocalFile.h"
#include "nsIOutputStream.h"

#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"
#include "nsIContent.h"

const char* const gPrologue = "<!DOCTYPE NETSCAPE-Bookmark-file-1>\n"
  "<!-- This is an automatically generated file.\n"
  "It will be read and overwritten.\n"
  "Do Not Edit! -->\n"
  "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=UTF-8\">\n"
  "<TITLE>Bookmarks</TITLE>\n"
  "<H1>Bookmarks</H1>\n";

/*
  Netscape HTML bookmarks format is:
  
  <dl><p>
    <dt><h3>folder</h3>
    <dl><p>
      <dt>
      <dt>
      ...
    </dl>
    ..
  </dl>
  
*/


nsresult
BookmarksExport::ExportBookmarksToHTML(nsIDOMDocument* inBookmarksDoc, const nsAString& inFilePath)
{
  nsresult rv;
  
  nsCOMPtr<nsILocalFile> destFile;
  rv = NS_NewLocalFile(inFilePath, PR_FALSE, getter_AddRefs(destFile));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIOutputStream> outputStream;
  rv = NS_NewLocalFileOutputStream(getter_AddRefs(outputStream), destFile);
  if (NS_FAILED(rv)) return rv;

  PRUint32 bytesWritten;
  rv = outputStream->Write(gPrologue, strlen(gPrologue), &bytesWritten);
  
  if (inBookmarksDoc)
  {
    nsCOMPtr<nsIDOMElement> docElement;
    inBookmarksDoc->GetDocumentElement(getter_AddRefs(docElement));
    
    rv = WriteItem(outputStream, docElement, 0, PR_TRUE);
  }
  
  rv = outputStream->Close();

  return NS_OK;
}


nsresult
BookmarksExport::WriteChildren(nsIOutputStream* outputStream, nsIDOMElement* inElement, PRInt32 inDepth)
{
  nsCOMPtr<nsIContent> curContent = do_QueryInterface(inElement);
  if (!curContent) return NS_ERROR_FAILURE;
  
  nsresult rv = NS_OK;
  
  // recurse to children
  PRInt32 numChildren;
  curContent->ChildCount(numChildren);
  for (PRInt32 i = 0; i < numChildren; i ++)
  {
    nsCOMPtr<nsIContent> curChild;
    curContent->ChildAt(i, *getter_AddRefs(curChild));
    
    nsCOMPtr<nsIDOMElement> curElt = do_QueryInterface(curChild);
    rv = WriteItem(outputStream, curElt, inDepth);
    if (NS_FAILED(rv)) break;
  }

  return rv;
}
  
nsresult
BookmarksExport::WriteItem(nsIOutputStream* outputStream, nsIDOMElement* inElement, PRInt32 inDepth, PRBool isRoot)
{
  nsCOMPtr<nsIContent> curContent = do_QueryInterface(inElement);
  if (!inElement || !curContent) return NS_ERROR_FAILURE;

  nsresult rv;
  
  PRUint32 bytesWritten;
  
  nsCOMPtr<nsIAtom> tagName;
  curContent->GetTag(*getter_AddRefs(tagName));

  PRBool isContainer = isRoot || (tagName == BookmarksService::gFolderAtom);
  
  nsAutoString href, title;

  const char* const spaces = "                                                                                ";
  const char* indentString = spaces + strlen(spaces) - (inDepth * 4);
  if (indentString < spaces)
    indentString = spaces;
  
  if (isContainer)
  {
    if (!isRoot)
    {
      rv = outputStream->Write(indentString, strlen(indentString), &bytesWritten);
  
      const char* const prefixString = "<DT><H3";
      rv = outputStream->Write(prefixString, strlen(prefixString), &bytesWritten);
  
      nsAutoString typeAttribute;
      inElement->GetAttribute(NS_LITERAL_STRING("type"), typeAttribute);
      if (typeAttribute.Equals(NS_LITERAL_STRING("toolbar")))
      {
        const char* persToolbar = " PERSONAL_TOOLBAR_FOLDER=\"true\"";
        rv = outputStream->Write(persToolbar, strlen(persToolbar), &bytesWritten);
      }
  
      rv = outputStream->Write(">", 1, &bytesWritten);
  
      inElement->GetAttribute(NS_LITERAL_STRING("name"), title);
  
      NS_ConvertUCS2toUTF8 titleConverter(title);
      const char* utf8String = titleConverter.get();
  
      rv = outputStream->Write(utf8String, strlen(utf8String), &bytesWritten);
  
      rv = outputStream->Write("</H3>\n", 6, &bytesWritten);
    }
    
    rv = outputStream->Write(indentString, strlen(indentString), &bytesWritten);
    rv = outputStream->Write("<DL><p>\n", 8, &bytesWritten);    
  
    rv = WriteChildren(outputStream, inElement, inDepth + 1);

    rv = outputStream->Write(indentString, strlen(indentString), &bytesWritten);
    rv = outputStream->Write("</DL><p>\n", 9, &bytesWritten);    
  }
  else
  {
    if (tagName == BookmarksService::gBookmarkAtom)
    {
      rv = outputStream->Write(indentString, strlen(indentString), &bytesWritten);

      const char* const bookmarkPrefix = "<DT><A HREF=\"";
      rv = outputStream->Write(bookmarkPrefix, strlen(bookmarkPrefix), &bytesWritten);

      inElement->GetAttribute(NS_LITERAL_STRING("href"), href);
      inElement->GetAttribute(NS_LITERAL_STRING("name"), title);

      NS_ConvertUCS2toUTF8 hrefConverter(href);
      const char* utf8String = hrefConverter.get();
      
      rv = outputStream->Write(utf8String, strlen(utf8String), &bytesWritten);
      outputStream->Write("\">", 2, &bytesWritten);
      
      NS_ConvertUCS2toUTF8 titleConverter(title);
      utf8String = titleConverter.get();

      rv = outputStream->Write(utf8String, strlen(utf8String), &bytesWritten);
      rv = outputStream->Write("</A>\n", 5, &bytesWritten);
    }
    else if (tagName == BookmarksService::gSeparatorAtom)
    {
      rv = outputStream->Write(indentString, strlen(indentString), &bytesWritten);
      const char* const hrString = "<HR>\n";
      rv = outputStream->Write(hrString, strlen(hrString), &bytesWritten);
    }
  
  }
  
  return rv;
}

