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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *    Simon Fraser <sfraser@netscape.com>
 *
 */

#include "nsCOMPtr.h"
#include "nsString.h"

#include "nsIEditor.h"
#include "nsIHTMLEditor.h"

#include "nsComposerUtils.h"


nsresult
nsComposerUtils::RemoveTextProperty(nsIEditor* aEditor,
                              const nsAString& prop, const nsAString& attr)
{
  // OK, I'm really hacking now. This is just so that we can accept 'all' as input.  
  nsAutoString  allStr(prop);
  
  PRBool    doingAll = (allStr.EqualsIgnoreCase("all"));
  nsresult  rv = NS_OK;

  nsCOMPtr<nsIHTMLEditor> htmlEditor(do_QueryInterface(aEditor));
  if (!htmlEditor) return NS_ERROR_NO_INTERFACE;
  
  if (doingAll)
  {
    rv = htmlEditor->RemoveAllInlineProperties();
  }
  else
  {
    nsAutoString  aProp(prop);
    rv = RemoveOneProperty(aEditor, prop, attr);
  }
  
  return rv;
}


nsresult
nsComposerUtils::RemoveOneProperty(nsIEditor* aEditor,
                              const nsAString& prop, const nsAString& attr)
{
  nsCOMPtr<nsIAtom> styleAtom = getter_AddRefs(NS_NewAtom(prop));      /// XXX Hack alert! Look in nsIEditProperty.h for this
  if (! styleAtom) return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsIHTMLEditor> htmlEditor(do_QueryInterface(aEditor));
  if (!htmlEditor) return NS_ERROR_NO_INTERFACE;

  return htmlEditor->RemoveInlineProperty(styleAtom, attr);
}

nsresult    
nsComposerUtils::SetTextProperty(nsIEditor* aEditor,
                              const nsAString& prop, const nsAString& attr, const nsAString& value)
{
  nsCOMPtr<nsIAtom> styleAtom = getter_AddRefs(NS_NewAtom(prop));      /// XXX Hack alert! Look in nsIEditProperty.h for this
  if (! styleAtom) return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsIHTMLEditor> htmlEditor(do_QueryInterface(aEditor));
  if (!htmlEditor) return NS_ERROR_NO_INTERFACE;

  return htmlEditor->SetInlineProperty(styleAtom, attr, value);
}


nsresult    
nsComposerUtils::GetListState(nsIEditor* aEditor, PRBool *aMixed, nsAString& outListType)
{
  if (!aMixed) return NS_ERROR_NULL_POINTER;
  outListType.Truncate();
  *aMixed = PR_FALSE;

  nsCOMPtr<nsIHTMLEditor> htmlEditor(do_QueryInterface(aEditor));
  if (!htmlEditor) return NS_ERROR_NO_INTERFACE;

  PRBool bOL, bUL, bDL;
  nsresult rv = htmlEditor->GetListState(aMixed, &bOL, &bUL, &bDL);
  if (NS_SUCCEEDED(rv))
  {
    if (!*aMixed)
    {
      if (bOL) outListType.Assign(NS_LITERAL_STRING("ol"));
      else if (bUL) outListType.Assign(NS_LITERAL_STRING("ul"));
      else if (bDL) outListType.Assign(NS_LITERAL_STRING("dl"));
    }
  }  

  return rv;
}


nsresult
nsComposerUtils::MakeOrChangeList(nsIEditor* aEditor, const nsAString& listType, PRBool entireList)
{
  nsresult rv = NS_NOINTERFACE;

  nsAutoString aListType(listType);

  nsCOMPtr<nsIHTMLEditor> htmlEditor(do_QueryInterface(aEditor));
  if (!htmlEditor) return NS_ERROR_NO_INTERFACE;
  
  if (aListType.IsEmpty())
  {
    rv = htmlEditor->RemoveList(NS_ConvertASCIItoUCS2("ol"));
    if(NS_SUCCEEDED(rv))
    {
      rv = htmlEditor->RemoveList(NS_ConvertASCIItoUCS2("ul"));
      if(NS_SUCCEEDED(rv))
        rv = htmlEditor->RemoveList(NS_ConvertASCIItoUCS2("dl"));
    }
  }
  else
    rv = htmlEditor->MakeOrChangeList(aListType, entireList);

  return rv;
}


nsresult
nsComposerUtils::RemoveList(nsIEditor* aEditor, const nsAString& listType)
{
  nsCOMPtr<nsIHTMLEditor> htmlEditor(do_QueryInterface(aEditor));
  if (!htmlEditor) return NS_ERROR_NO_INTERFACE;

  nsAutoString aListType(listType);
  return htmlEditor->RemoveList(aListType);
}


nsresult 
nsComposerUtils::GetListItemState(nsIEditor* aEditor, PRBool *aMixed, nsAString& outListState)
{
  if (!aMixed) return NS_ERROR_NULL_POINTER;
  outListState.Truncate();
  *aMixed = PR_FALSE;

  nsCOMPtr<nsIHTMLEditor> htmlEditor(do_QueryInterface(aEditor));
  if (!htmlEditor) return NS_ERROR_NO_INTERFACE;

  PRBool bLI,bDT,bDD;
  nsresult rv = htmlEditor->GetListItemState(aMixed, &bLI, &bDT, &bDD);
  if (NS_SUCCEEDED(rv))
  {
    if (!*aMixed)
    {
      if (bLI) outListState.Assign(NS_LITERAL_STRING("li"));
      else if (bDT) outListState.Assign(NS_LITERAL_STRING("dt"));
      else if (bDD) outListState.Assign(NS_LITERAL_STRING("dd"));
    }
  }

  return rv;
}

nsresult 
nsComposerUtils::GetAlignment(nsIEditor* aEditor, PRBool *aMixed, nsAString& outAlignment)
{
  if (!aMixed) return NS_ERROR_NULL_POINTER;
  outAlignment.Truncate();
  *aMixed = PR_FALSE;

  nsCOMPtr<nsIHTMLEditor> htmlEditor(do_QueryInterface(aEditor));
  if (!htmlEditor) return NS_ERROR_NO_INTERFACE;

  nsIHTMLEditor::EAlignment firstAlign;
  nsresult rv = htmlEditor->GetAlignment(aMixed, &firstAlign);
  if (NS_SUCCEEDED(rv))
  {
    switch (firstAlign)
    {
      case nsIHTMLEditor::eLeft:
        outAlignment.Assign(NS_LITERAL_STRING("left"));
        break;
      case nsIHTMLEditor::eCenter:
        outAlignment.Assign(NS_LITERAL_STRING("center"));
        break;
      case nsIHTMLEditor::eRight:
        outAlignment.Assign(NS_LITERAL_STRING("right"));
        break;
    }
  }  

  return rv;
}


