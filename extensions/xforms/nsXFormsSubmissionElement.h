/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is Mozilla XForms support.
 *
 * The Initial Developer of the Original Code is
 * IBM Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Darin Fisher <darin@meer.net>
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

#include "nsIXTFGenericElement.h"
#include "nsIDOMEventListener.h"
#include "nsXFormsElement.h"

class nsIInputStream;
class nsIContent;
class nsString;

class nsXFormsSubmissionElement : public nsXFormsElement,
                                  public nsIXTFGenericElement,
                                  public nsIDOMEventListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXTFELEMENT
  NS_DECL_NSIXTFGENERICELEMENT
  NS_DECL_NSIDOMEVENTLISTENER

  nsXFormsSubmissionElement()
    : mContent(nsnull)
  {}

  NS_HIDDEN_(void)     Submit();
  NS_HIDDEN_(nsresult) SubmitEnd(PRBool succeeded);
  NS_HIDDEN_(void)     GetDefaultInstanceData(nsIDOMNode **result);
  NS_HIDDEN_(void)     GetSelectedInstanceData(nsIDOMNode **result);
  NS_HIDDEN_(nsresult) SerializeData(nsIDOMNode *data, nsString &uri, nsIInputStream **);
  NS_HIDDEN_(void)     AppendDataToURI(nsIDOMNode *data, nsString &uri, const nsString &separator);
  NS_HIDDEN_(nsresult) SendData(nsString &uri, nsIInputStream *stream);

private:
  nsIContent *mContent;
};

NS_HIDDEN_(nsresult)
NS_NewXFormsSubmissionElement(nsIXTFElement **aResult);
