/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Pierre Phaneuf <pp@ludusdesign.com>
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
 
#include "nsCOMPtr.h"

#include "nsEditorMode.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDOMDocument.h"
#include "nsIDOMWindow.h"
#include "nsIDocShell.h"

#include "nsIEditingSession.h"
#include "nsIEditor.h"
#include "nsIHTMLEditor.h"
#include "nsITableEditor.h"
#include "nsIDocumentEncoder.h" // for output flags

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "resources.h"
#include "nsISelectionController.h"


static nsresult PrintEditorOutput(nsIEditor* editor, PRInt32 aCommandID)
{
	nsString		outString;
	char*			cString;
	nsString		formatString;
    PRUint32        flags = 0;
	
	switch (aCommandID)
	{
      case VIEWER_DISPLAYTEXT:
        formatString.AssignWithConversion("text/plain");
        flags = nsIDocumentEncoder::OutputFormatted;
        editor->OutputToString(outString, formatString, flags);
        break;
        
      case VIEWER_DISPLAYHTML:
        formatString.AssignWithConversion("text/html");
        editor->OutputToString(outString, formatString, flags);
        break;
	}

	cString = ToNewCString(outString);
	printf(cString);
	delete [] cString;
	
	return NS_OK;
}


nsresult NS_DoEditorTest(nsIDocShell *aDocShell, PRInt32 aCommandID)
{
  nsCOMPtr<nsIEditingSession>   editingSession = do_GetInterface(aDocShell);
  if (!editingSession)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMWindow> domWindow = do_GetInterface(aDocShell);

  nsCOMPtr<nsIEditor>   editor;
  editingSession->GetEditorForWindow(domWindow, getter_AddRefs(editor));
  
  nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(editor);
  nsCOMPtr<nsITableEditor> tableEditor = do_QueryInterface(editor);
  if (htmlEditor && tableEditor)
  {
    switch (aCommandID)
    {
      case VIEWER_EDIT_SET_BGCOLOR_RED:
        htmlEditor->SetBodyAttribute(NS_ConvertASCIItoUCS2("bgcolor"), NS_ConvertASCIItoUCS2("red"));
        break;
      case VIEWER_EDIT_SET_BGCOLOR_YELLOW:
        htmlEditor->SetBodyAttribute(NS_ConvertASCIItoUCS2("bgcolor"), NS_ConvertASCIItoUCS2("yellow"));
        break;
      case VIEWER_EDIT_INSERT_CELL:  
        tableEditor->InsertTableCell(1, PR_FALSE);
        break;    
      case VIEWER_EDIT_INSERT_COLUMN:
        tableEditor->InsertTableColumn(1, PR_FALSE);
        break;    
      case VIEWER_EDIT_INSERT_ROW:    
        tableEditor->InsertTableRow(1, PR_FALSE);
        break;    
      case VIEWER_EDIT_DELETE_TABLE:  
        tableEditor->DeleteTable();
        break;    
      case VIEWER_EDIT_DELETE_CELL:  
        tableEditor->DeleteTableCell(1);
        break;    
      case VIEWER_EDIT_DELETE_COLUMN:
        tableEditor->DeleteTableColumn(1);
        break;    
      case VIEWER_EDIT_DELETE_ROW:
        tableEditor->DeleteTableRow(1);
        break;    
      case VIEWER_EDIT_JOIN_CELL_RIGHT:
        tableEditor->JoinTableCells(PR_FALSE);
        break;    
      case VIEWER_DISPLAYTEXT:
      case VIEWER_DISPLAYHTML:
        PrintEditorOutput(editor, aCommandID);
        break;
     default:
        return NS_ERROR_NOT_IMPLEMENTED;    
    }
  }
  return NS_OK;
}

