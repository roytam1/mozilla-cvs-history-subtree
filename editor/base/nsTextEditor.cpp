/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL") you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#error "This file is no longer used"

#include "nsTextEditor.h"
#include "nsEditorEventListeners.h"

#include "nsIDocument.h"
#include "nsFileSpec.h"

#include "nsIDOMDocument.h"
#include "nsIDOMEventReceiver.h" 
#include "nsIDOMKeyListener.h" 
#include "nsIDOMMouseListener.h"
#include "nsIDOMDragListener.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMSelection.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMCharacterData.h"
#include "nsIDOMElement.h"
#include "nsIDOMTextListener.h"
#include "nsIDiskDocument.h"
#include "nsIDocumentEncoder.h"
#include "nsEditorCID.h"
#include "nsISupportsArray.h"
#include "nsIEnumerator.h"
#include "nsIContentIterator.h"
#include "nsIContent.h"
#include "nsLayoutCID.h"
#include "nsIPresShell.h"
#include "nsIStyleContext.h"
#include "nsVoidArray.h"

#if DEBUG
#include "TextEditorTest.h"
#endif

// transactions the text editor knows how to build itself
#include "TransactionFactory.h"
#include "PlaceholderTxn.h"
#include "InsertTextTxn.h"

#include "nsIFileStream.h"
#include "nsIStringStream.h"

#include "nsIAppShell.h"
#include "nsIToolkit.h"
#include "nsWidgetsCID.h"
#include "nsIFileWidget.h"

// Drag & Drop, Clipboard
#include "nsIClipboard.h"
#include "nsITransferable.h"
//#include "nsIFormatConverter.h"

// Drag & Drop, Clipboard Support
static NS_DEFINE_CID(kCClipboardCID,           NS_CLIPBOARD_CID);
static NS_DEFINE_CID(kCTransferableCID,        NS_TRANSFERABLE_CID);
//static NS_DEFINE_IID(kCXIFFormatConverterCID,  NS_XIFFORMATCONVERTER_CID);

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"

#include "nsTextEditRules.h"

#include "nsIPref.h"
#include "nsAOLCiter.h"
#include "nsInternetCiter.h"

#ifdef ENABLE_JS_EDITOR_LOG
#include "nsJSEditorLog.h"
#endif // ENABLE_JS_EDITOR_LOG

static NS_DEFINE_CID(kEditorCID,        NS_EDITOR_CID);
static NS_DEFINE_CID(kTextEditorCID,    NS_TEXTEDITOR_CID);
static NS_DEFINE_CID(kCContentIteratorCID,   NS_CONTENTITERATOR_CID);
static NS_DEFINE_CID(kCRangeCID,    NS_RANGE_CID);
static NS_DEFINE_CID(kPrefServiceCID, NS_PREF_CID);


#ifdef NS_DEBUG
static PRBool gNoisy = PR_FALSE;
#else
static const PRBool gNoisy = PR_FALSE;
#endif

/*****************************************************************************
 * nsTextEditor implementation
 ****************************************************************************/

nsTextEditor::nsTextEditor()
:  mTypeInState(nsnull)
,  mRules(nsnull)
,  mKeyListenerP(nsnull)
,  mIsComposing(PR_FALSE)
{
// Done in nsEditor
//  NS_INIT_REFCNT();
  mRules = nsnull;
  mMaxTextLength = -1;
  mWrapColumn = 72;
}

nsTextEditor::~nsTextEditor()
{}

// Adds appropriate AddRef, Release, and QueryInterface methods for derived class
//NS_IMPL_ISUPPORTS_INHERITED(nsTextEditor, nsEditor, nsITextEditor)

//NS_IMPL_ADDREF_INHERITED(Class, Super)
NS_IMETHODIMP_(nsrefcnt) nsTextEditor::AddRef(void)
{
  return nsEditor::AddRef();
}

//NS_IMPL_RELEASE_INHERITED(Class, Super)
NS_IMETHODIMP_(nsrefcnt) nsTextEditor::Release(void)
{
  return nsEditor::Release();
}

//NS_IMPL_QUERY_INTERFACE_INHERITED(Class, Super, AdditionalInterface)
NS_IMETHODIMP nsTextEditor::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (!aInstancePtr) return NS_ERROR_NULL_POINTER;
 
  if (aIID.Equals(nsITextEditor::GetIID())) {
    *aInstancePtr = NS_STATIC_CAST(nsITextEditor*, this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return nsEditor::QueryInterface(aIID, aInstancePtr);
}

NS_IMETHODIMP nsTextEditor::Init(nsIDOMDocument *aDoc, 
                                 nsIPresShell   *aPresShell)
{}

void nsTextEditor::HandleEventListenerError()
{}

void nsTextEditor::InitRules()
{}

NS_IMETHODIMP
nsTextEditor::GetFlags(PRUint32 *aFlags)
{
  if (!mRules || !aFlags) { return NS_ERROR_NULL_POINTER; }
  return mRules->GetFlags(aFlags);
}

NS_IMETHODIMP
nsTextEditor::SetFlags(PRUint32 aFlags)
{
  if (!mRules) { return NS_ERROR_NULL_POINTER; }
  return mRules->SetFlags(aFlags);
}

NS_IMETHODIMP nsTextEditor::SetTextProperty(nsIAtom        *aProperty, 
                                            const nsString *aAttribute, 
                                            const nsString *aValue)
{}

NS_IMETHODIMP nsTextEditor::GetTextProperty(nsIAtom *aProperty, 
                                            const nsString *aAttribute, 
                                            const nsString *aValue,
                                            PRBool &aFirst, PRBool &aAny, PRBool &aAll)
{}

void nsTextEditor::IsTextStyleSet(nsIStyleContext *aSC, 
                                  nsIAtom *aProperty, 
                                  const nsString *aAttribute,  
                                  PRBool &aIsSet) const
{}

// this will NOT find aAttribute unless aAttribute has a non-null value
// so singleton attributes like <Table border> will not be matched!
void nsTextEditor::IsTextPropertySetByContent(nsIDOMNode     *aNode,
                                              nsIAtom        *aProperty, 
                                              const nsString *aAttribute, 
                                              const nsString *aValue, 
                                              PRBool         &aIsSet,
                                              nsIDOMNode    **aStyleNode) const
{}


NS_IMETHODIMP nsTextEditor::RemoveTextProperty(nsIAtom *aProperty, const nsString *aAttribute)
{}

void nsTextEditor::GetTextSelectionOffsetsForRange(nsIDOMSelection *aSelection,
                                                   nsIDOMNode **aParent,
                                                   PRInt32     &aStartOffset, 
                                                   PRInt32     &aEndOffset)
{}

void nsTextEditor::ResetTextSelectionForRange(nsIDOMNode *aParent,
                                              PRInt32     aStartOffset,
                                              PRInt32     aEndOffset,
                                              nsIDOMSelection *aSelection)
{}


NS_IMETHODIMP nsTextEditor::DeleteSelection(nsIEditor::ESelectionCollapseDirection aAction)
{}

NS_IMETHODIMP nsTextEditor::InsertText(const nsString& aStringToInsert)
{}

NS_IMETHODIMP nsTextEditor::SetMaxTextLength(PRInt32 aMaxTextLength)
{
  mMaxTextLength = aMaxTextLength;
  return NS_OK;
}

NS_IMETHODIMP nsTextEditor::GetMaxTextLength(PRInt32& aMaxTextLength)
{
  aMaxTextLength = mMaxTextLength;
  return NS_OK;
}

NS_IMETHODIMP nsTextEditor::InsertBreak()
{
#ifdef ENABLE_JS_EDITOR_LOG
  nsAutoJSEditorLogLock logLock(mJSEditorLog);

  if (mJSEditorLog)
    mJSEditorLog->InsertBreak();
#endif // ENABLE_JS_EDITOR_LOG

  nsCOMPtr<nsIDOMSelection> selection;
  PRBool cancel= PR_FALSE;

  // pre-process
  nsEditor::GetSelection(getter_AddRefs(selection));
  nsTextRulesInfo ruleInfo(nsTextEditRules::kInsertBreak);
  nsresult result = mRules->WillDoAction(selection, &ruleInfo, &cancel);
  if ((PR_FALSE==cancel) && (NS_SUCCEEDED(result)))
  {
    // For plainttext just pass newlines through
    nsAutoString  key;
    key += '\n';
    result = InsertText(key);
    nsEditor::GetSelection(getter_AddRefs(selection));
    result = mRules->DidDoAction(selection, &ruleInfo, result);
  }
  return result;
}


NS_IMETHODIMP nsTextEditor::EnableUndo(PRBool aEnable)
{
  return nsEditor::EnableUndo(aEnable);
}

NS_IMETHODIMP nsTextEditor::Undo(PRUint32 aCount)
{
#ifdef ENABLE_JS_EDITOR_LOG
  nsAutoJSEditorLogLock logLock(mJSEditorLog);

  if (mJSEditorLog)
    mJSEditorLog->Undo(aCount);
#endif // ENABLE_JS_EDITOR_LOG

  nsCOMPtr<nsIDOMSelection> selection;
  PRBool cancel= PR_FALSE;

  // pre-process
  nsEditor::GetSelection(getter_AddRefs(selection));
  nsTextRulesInfo ruleInfo(nsTextEditRules::kUndo);
  nsresult result = mRules->WillDoAction(selection, &ruleInfo, &cancel);
  if ((PR_FALSE==cancel) && (NS_SUCCEEDED(result)))
  {
    result = nsEditor::Undo(aCount);
    nsEditor::GetSelection(getter_AddRefs(selection));
    result = mRules->DidDoAction(selection, &ruleInfo, result);
  }
  return result;
}

NS_IMETHODIMP nsTextEditor::CanUndo(PRBool &aIsEnabled, PRBool &aCanUndo)
{
  return nsEditor::CanUndo(aIsEnabled, aCanUndo);
}

NS_IMETHODIMP nsTextEditor::Redo(PRUint32 aCount)
{
#ifdef ENABLE_JS_EDITOR_LOG
  nsAutoJSEditorLogLock logLock(mJSEditorLog);

  if (mJSEditorLog)
    mJSEditorLog->Redo(aCount);
#endif // ENABLE_JS_EDITOR_LOG

  nsCOMPtr<nsIDOMSelection> selection;
  PRBool cancel= PR_FALSE;

  // pre-process
  nsEditor::GetSelection(getter_AddRefs(selection));
  nsTextRulesInfo ruleInfo(nsTextEditRules::kRedo);
  nsresult result = mRules->WillDoAction(selection, &ruleInfo, &cancel);
  if ((PR_FALSE==cancel) && (NS_SUCCEEDED(result)))
  {
    result = nsEditor::Redo(aCount);
    nsEditor::GetSelection(getter_AddRefs(selection));
    result = mRules->DidDoAction(selection, &ruleInfo, result);
  }
  return result;
}

NS_IMETHODIMP nsTextEditor::CanRedo(PRBool &aIsEnabled, PRBool &aCanRedo)
{
  return nsEditor::CanRedo(aIsEnabled, aCanRedo);
}

NS_IMETHODIMP nsTextEditor::BeginTransaction()
{
  return nsEditor::BeginTransaction();
}

NS_IMETHODIMP nsTextEditor::EndTransaction()
{
  return nsEditor::EndTransaction();
}

NS_IMETHODIMP nsTextEditor::MoveSelectionUp(nsIAtom *aIncrement, PRBool aExtendSelection)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsTextEditor::MoveSelectionDown(nsIAtom *aIncrement, PRBool aExtendSelection)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsTextEditor::MoveSelectionNext(nsIAtom *aIncrement, PRBool aExtendSelection)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsTextEditor::MoveSelectionPrevious(nsIAtom *aIncrement, PRBool aExtendSelection)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsTextEditor::SelectNext(nsIAtom *aIncrement, PRBool aExtendSelection) 
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsTextEditor::SelectPrevious(nsIAtom *aIncrement, PRBool aExtendSelection)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsTextEditor::SelectAll()
{
  return nsEditor::SelectAll();
}

NS_IMETHODIMP nsTextEditor::BeginningOfDocument()
{
  return nsEditor::BeginningOfDocument();
}

NS_IMETHODIMP nsTextEditor::EndOfDocument()
{
  return nsEditor::EndOfDocument();
}

NS_IMETHODIMP nsTextEditor::ScrollUp(nsIAtom *aIncrement)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsTextEditor::ScrollDown(nsIAtom *aIncrement)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsTextEditor::ScrollIntoView(PRBool aScrollToBegin)
{
  return nsEditor::ScrollIntoView(aScrollToBegin);
}

static NS_DEFINE_IID(kCFileWidgetCID, NS_FILEWIDGET_CID);
static NS_DEFINE_IID(kIFileWidgetIID, NS_IFILEWIDGET_IID);

NS_IMETHODIMP nsTextEditor::SaveDocument(PRBool saveAs, PRBool saveCopy)
{}

NS_IMETHODIMP nsTextEditor::Save()
{}

NS_IMETHODIMP nsTextEditor::SaveAs(PRBool aSavingCopy)
{}

NS_IMETHODIMP nsTextEditor::Cut()
{
  return nsEditor::Cut();
}

NS_IMETHODIMP nsTextEditor::Copy()
{
  return nsEditor::Copy();
}

NS_IMETHODIMP nsTextEditor::Paste()
{}

//
// Similar to that in nsEditor::Paste except that it does indentation:
//
NS_IMETHODIMP nsTextEditor::PasteAsQuotation()
{
#ifdef ENABLE_JS_EDITOR_LOG
  nsAutoJSEditorLogLock logLock(mJSEditorLog);

  if (mJSEditorLog)
    mJSEditorLog->PasteAsQuotation();
#endif // ENABLE_JS_EDITOR_LOG

  nsString stuffToPaste;

  // Get Clipboard Service
  nsIClipboard* clipboard;
  nsresult rv = nsServiceManager::GetService(kCClipboardCID,
                                             nsIClipboard::GetIID(),
                                             (nsISupports **)&clipboard);

  // Create generic Transferable for getting the data
  nsCOMPtr<nsITransferable> trans;
  rv = nsComponentManager::CreateInstance(kCTransferableCID, nsnull, 
                                          nsITransferable::GetIID(), 
                                          (void**) getter_AddRefs(trans));
  if (NS_OK == rv)
  {
    // Get nsITransferable interface for getting the data from the clipboard
    if (trans)
    {
      // We only handle plaintext pastes here
      nsAutoString flavor(kTextMime);

      trans->AddDataFlavor(&flavor);

      // Get the Data from the clipboard
      clipboard->GetData(trans);

      // Now we ask the transferable for the data
      // it still owns the data, we just have a pointer to it.
      // If it can't support a "text" output of the data the call will fail
      char *str = 0;
      PRUint32 len;
      if (NS_OK == trans->GetTransferData(&flavor, (void **)&str, &len)) {

        // Make adjustments for null terminated strings
        if (str && len > 0) {
          // stuffToPaste is ready for insertion into the content
          stuffToPaste.SetString(str, len);
        }
      }
    }
  }
  nsServiceManager::ReleaseService(kCClipboardCID, clipboard);

  return InsertAsQuotation(stuffToPaste);
}

NS_IMETHODIMP nsTextEditor::InsertAsQuotation(const nsString& aQuotedText)
{}

//
// Get the wrap width for the first PRE tag in the document.
// If no PRE tag, throw an error.
//
NS_IMETHODIMP nsTextEditor::GetBodyWrapWidth(PRInt32 *aWrapColumn)
{}

//
// Change the wrap width on the first <PRE> tag in this document.
// (Eventually want to search for more than one in case there are
// interspersed quoted text blocks.)
// 
NS_IMETHODIMP nsTextEditor::SetBodyWrapWidth(PRInt32 aWrapColumn)
{}  

NS_IMETHODIMP nsTextEditor::ApplyStyleSheet(const nsString& aURL)
{}

NS_IMETHODIMP nsTextEditor::OutputToString(nsString& aOutputString,
                                           const nsString& aFormatType,
                                           PRUint32 aFlags)
{}

NS_IMETHODIMP nsTextEditor::OutputToStream(nsIOutputStream* aOutputStream,
                                           const nsString& aFormatType,
                                           const nsString* aCharset,
                                           PRUint32 aFlags)
{}


NS_IMETHODIMP nsTextEditor::SetTextPropertiesForNode(nsIDOMNode  *aNode, 
                                                     nsIDOMNode  *aParent,
                                                     PRInt32      aStartOffset,
                                                     PRInt32      aEndOffset,
                                                     nsIAtom     *aPropName, 
                                                     const nsString *aAttribute,
                                                     const nsString *aValue)
{}

NS_IMETHODIMP nsTextEditor::MoveContentOfNodeIntoNewParent(nsIDOMNode  *aNode, 
                                                           nsIDOMNode  *aNewParentNode, 
                                                           PRInt32      aStartOffset, 
                                                           PRInt32      aEndOffset)
{}

/* this should only get called if the only intervening nodes are inline style nodes */
NS_IMETHODIMP 
nsTextEditor::SetTextPropertiesForNodesWithSameParent(nsIDOMNode  *aStartNode,
                                                      PRInt32      aStartOffset,
                                                      nsIDOMNode  *aEndNode,
                                                      PRInt32      aEndOffset,
                                                      nsIDOMNode  *aParent,
                                                      nsIAtom     *aPropName,
                                                      const nsString *aAttribute,
                                                      const nsString *aValue)
{}

//XXX won't work for selections that are not leaf nodes!
//    should fix up the end points to make sure they are leaf nodes
NS_IMETHODIMP nsTextEditor::MoveContiguousContentIntoNewParent(nsIDOMNode  *aStartNode, 
                                                               PRInt32      aStartOffset, 
                                                               nsIDOMNode  *aEndNode,
                                                               PRInt32      aEndOffset,
                                                               nsIDOMNode  *aGrandParentNode,
                                                               nsIDOMNode  *aNewParentNode)
{}


/* this wraps every selected text node in a new inline style node if needed
   the text nodes are treated as being unique -- each needs it's own style node 
   if the style is not already present.
   each action has immediate effect on the content tree and resolved style, so
   doing outermost text nodes first removes the need for interior style nodes in some cases.
   XXX: need to code test to see if new style node is needed
*/
NS_IMETHODIMP
nsTextEditor::SetTextPropertiesForNodeWithDifferentParents(nsIDOMRange *aRange,
                                                           nsIDOMNode  *aStartNode,
                                                           PRInt32      aStartOffset,
                                                           nsIDOMNode  *aEndNode,
                                                           PRInt32      aEndOffset,
                                                           nsIDOMNode  *aParent,
                                                           nsIAtom     *aPropName,
                                                           const nsString *aAttribute,
                                                           const nsString *aValue)
{}

NS_IMETHODIMP nsTextEditor::RemoveTextPropertiesForNode(nsIDOMNode *aNode, 
                                                        nsIDOMNode *aParent,
                                                        PRInt32     aStartOffset,
                                                        PRInt32     aEndOffset,
                                                        nsIAtom    *aPropName,
                                                        const nsString *aAttribute)
{}

/* this should only get called if the only intervening nodes are inline style nodes */
NS_IMETHODIMP 
nsTextEditor::RemoveTextPropertiesForNodesWithSameParent(nsIDOMNode *aStartNode,
                                                         PRInt32     aStartOffset,
                                                         nsIDOMNode *aEndNode,
                                                         PRInt32     aEndOffset,
                                                         nsIDOMNode *aParent,
                                                         nsIAtom    *aPropName,
                                                         const nsString *aAttribute)
{}

NS_IMETHODIMP
nsTextEditor::RemoveTextPropertiesForNodeWithDifferentParents(nsIDOMNode  *aStartNode,
                                                              PRInt32      aStartOffset,
                                                              nsIDOMNode  *aEndNode,
                                                              PRInt32      aEndOffset,
                                                              nsIDOMNode  *aParent,
                                                              nsIAtom     *aPropName,
                                                              const nsString *aAttribute)
{}

TypeInState * nsTextEditor::GetTypeInState()
{}

NS_IMETHODIMP
nsTextEditor::SetTypeInStateForProperty(TypeInState    &aTypeInState, 
                                        nsIAtom        *aPropName, 
                                        const nsString *aAttribute,
                                        const nsString *aValue)
{}

NS_IMETHODIMP nsTextEditor::SetBackgroundColor(const nsString& aColor)
{}

// This file should be rearranged to put all methods that simply call nsEditor together
NS_IMETHODIMP
nsTextEditor::CopyAttributes(nsIDOMNode *aDestNode, nsIDOMNode *aSourceNode)
{
  return nsEditor::CopyAttributes(aDestNode, aSourceNode);
}

NS_IMETHODIMP
nsTextEditor::BeginComposition(void)
{}

NS_IMETHODIMP
nsTextEditor::SetCompositionString(const nsString& aCompositionString,nsIDOMTextRangeList* aTextRangeList)
{}

NS_IMETHODIMP
nsTextEditor::EndComposition(void)
{}


NS_IMETHODIMP
nsTextEditor::DebugUnitTests(PRInt32 *outNumTests, PRInt32 *outNumTestsFailed)
{
#ifdef DEBUG
  if (!outNumTests || !outNumTestsFailed)
    return NS_ERROR_NULL_POINTER;

	TextEditorTest *tester = new TextEditorTest();
	if (!tester)
	  return NS_ERROR_OUT_OF_MEMORY;
	 
  tester->Run(this, outNumTests, outNumTestsFailed);
  delete tester;
  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}


NS_IMETHODIMP
nsTextEditor::GetDocumentLength(PRInt32 *aCount)                                              
{}



