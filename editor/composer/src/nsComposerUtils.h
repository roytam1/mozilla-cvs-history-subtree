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


/*
  nsComposerUtils is a helper class of static methods that
  act as wrappers for editor calls.  
  
*/

class nsIEditor;

class nsComposerUtils
{
public:


  static nsresult   RemoveTextProperty(nsIEditor* aEditor,
                              const nsAString& prop, const nsAString& attr);
  static nsresult   RemoveOneProperty(nsIEditor* aEditor,
                              const nsAString& prop, const nsAString& attr);
  static nsresult   SetTextProperty(nsIEditor* aEditor,
                              const nsAString& prop, const nsAString& attr, const nsAString& value);

  static nsresult   GetListState(nsIEditor* aEditor, PRBool *aMixed, nsAString& outListType);
  static nsresult   MakeOrChangeList(nsIEditor* aEditor, const nsAString& listType, PRBool entireList);
  static nsresult   RemoveList(nsIEditor* aEditor, const nsAString& listType);
  static nsresult   GetListItemState(nsIEditor* aEditor, PRBool *aMixed, nsAString& outListState);
  
  static nsresult   GetAlignment(nsIEditor* aEditor, PRBool *aMixed, nsAString& outAlignment);
  
};

