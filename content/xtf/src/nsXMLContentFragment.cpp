/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ----- BEGIN LICENSE BLOCK -----
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Mozilla XTF project.
 *
 * The Initial Developer of the Original Code is 
 * Alex Fritze.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Alex Fritze <alex@croczilla.com> (original author)
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
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ----- END LICENSE BLOCK ----- */

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIXMLContentFragment.h"
#include "nsISupportsArray.h"
#include "nsINameSpaceManager.h"
#include "nsINodeInfo.h"
#include "nsIContent.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIElementFactory.h"
#include "nsIAtom.h"

class nsXMLContentFragment : public nsIXMLContentFragment // :nsIXMLContentBuilder
{
protected:
  friend nsresult NS_NewXMLContentFragment(nsIXMLContentFragment** aResult);
  
  nsXMLContentFragment();
  
public:
  // nsISupports interface
  NS_DECL_ISUPPORTS

  // nsIXMLContentFragment interface
  NS_DECL_NSIXMLCONTENTFRAGMENT

  // nsIXMLContentBuilder interface
  NS_DECL_NSIXMLCONTENTBUILDER

private:
  nsCOMPtr<nsISupportsArray> mContent;
  nsCOMPtr<nsIContent> mCurrentNode;
  nsCOMPtr<nsINodeInfoManager> mNodeInfoManager;
};

//----------------------------------------------------------------------
// implementation:

nsXMLContentFragment::nsXMLContentFragment()
{
#ifdef DEBUG
  printf("nsXMLContentFragment CTOR\n");
#endif
  NS_NewISupportsArray(getter_AddRefs(mContent));
  mNodeInfoManager = do_CreateInstance(NS_NODEINFOMANAGER_CONTRACTID);
}

nsresult
NS_NewXMLContentFragment(nsIXMLContentFragment** aResult)
{
  NS_PRECONDITION(aResult != nsnull, "null ptr");
  if (! aResult)
    return NS_ERROR_NULL_POINTER;

  nsXMLContentFragment* result = new nsXMLContentFragment();
  if (! result)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(result);
  *aResult = result;
  return NS_OK;
}

//----------------------------------------------------------------------
// nsISupports methods

NS_IMPL_ISUPPORTS2(nsXMLContentFragment, nsIXMLContentFragment, nsIXMLContentBuilder);

//----------------------------------------------------------------------
// nsIXMLContentFragment implementation

/* void clear (); */
NS_IMETHODIMP nsXMLContentFragment::Clear()
{
  mCurrentNode = nsnull;
  mContent->Clear();
  // XXX clear ns mgr?
  return NS_OK;
}

/* readonly attribute nsISupportsArray content; */
NS_IMETHODIMP nsXMLContentFragment::GetContent(nsISupportsArray * *aContent)
{
  *aContent = mContent;
  NS_IF_ADDREF(*aContent);
  return NS_OK;
}

//----------------------------------------------------------------------
// nsIXMLContentBuilder implementation

/* nsISupports beginElement (in AString ns, in AString tagname); */
NS_IMETHODIMP nsXMLContentFragment::BeginElement(const nsAString & ns,
                                                 const nsAString & tagname,
                                                 nsISupports **_retval)
{
  *_retval = nsnull;
  PRInt32 namespaceid;
  nsCOMPtr<nsIElementFactory> ef;
  {
    nsCOMPtr<nsINameSpaceManager> nsMgr = do_GetService(NS_NAMESPACEMANAGER_CONTRACTID);
    NS_ASSERTION(nsMgr, "could not get namespace manager");
    nsMgr->RegisterNameSpace(ns, namespaceid);
    nsMgr->GetElementFactory(namespaceid, getter_AddRefs(ef));
  }
  NS_ASSERTION(ef, "could not get element factory");
    
  nsCOMPtr<nsIContent> node;
  {
    nsCOMPtr<nsINodeInfo> nodeInfo;
    mNodeInfoManager->GetNodeInfo(tagname, nsnull,
                                  namespaceid,
                                  getter_AddRefs(nodeInfo));
    NS_ASSERTION(nodeInfo, "could not get node info");
  
    ef->CreateInstanceByTag(nodeInfo, getter_AddRefs(node));
  }
  NS_ASSERTION(node, "could not create node");

  // ok, we create a content element. now either append it to our
  // top-level array or to the current element.
  if (mCurrentNode) {
    mCurrentNode->AppendChildTo(node, PR_TRUE, PR_TRUE);
  }
  else {
    mContent->AppendElement(node);
  }
  mCurrentNode = node;

  *_retval = node;
  NS_IF_ADDREF(*_retval);
  
  return NS_OK;
}

/* void endElement (); */
NS_IMETHODIMP nsXMLContentFragment::EndElement()
{
  NS_ASSERTION(mCurrentNode, "unbalanced begin/endelement");
  mCurrentNode = mCurrentNode->GetParent();
  return NS_OK;
}

/* void attrib (in AString name, in AString value); */
NS_IMETHODIMP nsXMLContentFragment::Attrib(const nsAString & name, const nsAString & value)
{
  NS_ASSERTION(mCurrentNode, "can't set attrib w/o open element");
  nsCOMPtr<nsIAtom> nameAtom = do_GetAtom(name);
  mCurrentNode->SetAttr(0, nameAtom, value, PR_TRUE);
  return NS_OK;
}

