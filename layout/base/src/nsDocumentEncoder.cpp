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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "nsIDocumentEncoder.h"

#include "nscore.h"
#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsIComponentManager.h" 
#include "nsIServiceManager.h"
#include "nsIDocument.h"
#include "nsIDOMSelection.h"
#include "nsIPresShell.h"
#include "nsParserCIID.h"
#include "nsIParser.h"
#include "nsHTMLContentSinkStream.h"
#include "nsCOMPtr.h"
#include "nsIContentSerializer.h"
#include "nsIUnicodeEncoder.h"
#include "nsIOutputStream.h"
#include "nsIDOMElement.h"
#include "nsIDOMText.h"
#include "nsIDOMCDATASection.h"
#include "nsIDOMComment.h"
#include "nsIDOMProcessingInstruction.h"
#include "nsIDOMDocumentType.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMRange.h"
#include "nsRange.h"
#include "nsICharsetConverterManager.h"

static NS_DEFINE_CID(kCharsetConverterManagerCID,
                     NS_ICHARSETCONVERTERMANAGER_CID);


enum nsRangeIterationDirection {
  kDirectionOut = -1,
  kDirectionIn = 1
};

class nsDocumentEncoder : public nsIDocumentEncoder
{
public:
  static const nsIID& GetIID() { static nsIID iid = NS_IDOCUMENT_ENCODER_IID; return iid; }

  nsDocumentEncoder();
  virtual ~nsDocumentEncoder();

  NS_IMETHOD Init(nsIDocument* aDocument, const nsAReadableString& aMimeType, PRUint32 aFlags);

  /* Interfaces for addref and release and queryinterface */
  NS_DECL_ISUPPORTS

  // Inherited methods from nsIDocumentEncoder
  NS_IMETHOD SetSelection(nsIDOMSelection* aSelection);
  NS_IMETHOD SetRange(nsIDOMRange* aRange);
  NS_IMETHOD SetWrapColumn(PRUint32 aWC);
  NS_IMETHOD SetCharset(const nsAReadableString& aCharset);

  NS_IMETHOD EncodeToStream(nsIOutputStream* aStream);
  NS_IMETHOD EncodeToString(nsAWritableString& aOutputString);

protected:
  nsresult SerializeNodeStart(nsIDOMNode* aNode, PRInt32 aStartOffset,
                              PRInt32 aEndOffset, nsAWritableString& aStr);
  nsresult SerializeToStringRecursive(nsIDOMNode* aNode,
                                      nsAWritableString& aStr);
  nsresult SerializeNodeEnd(nsIDOMNode* aNode, nsAWritableString& aStr);
  nsresult SerializeRangeToString(nsIDOMRange *aRange,
                                  nsAWritableString& aOutputString);
  nsresult SerializeRangeNodes(nsVoidArray& aAncestors,
                               nsIDOMNode *aCommonParent,
                               nsIDOMNode* aStart,
                               PRInt32 aStartOffset,
                               nsIDOMNode* aEnd,
                               PRInt32 aEndOffset,
                               nsAWritableString& aString);

  nsresult FlushText(nsAWritableString& aString, PRBool aForce);

  nsCOMPtr<nsIDocument>          mDocument;
  nsCOMPtr<nsIDOMSelection>      mSelection;
  nsCOMPtr<nsIDOMRange>          mRange;
  nsCOMPtr<nsIOutputStream>      mStream;
  nsCOMPtr<nsIContentSerializer> mSerializer;
  nsCOMPtr<nsIUnicodeEncoder>    mUnicodeEncoder;

  nsString          mMimeType;
  nsString          mCharset;
  PRUint32          mFlags;
  PRUint32          mWrapColumn;
};


NS_IMPL_ADDREF(nsDocumentEncoder)
NS_IMPL_RELEASE(nsDocumentEncoder)

NS_INTERFACE_MAP_BEGIN(nsDocumentEncoder)
   NS_INTERFACE_MAP_ENTRY(nsIDocumentEncoder)
   NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

nsDocumentEncoder::nsDocumentEncoder()
{
  NS_INIT_REFCNT();

  mMimeType.AssignWithConversion("text/plain");

  mFlags = 0;
  mWrapColumn = 72;
}

nsDocumentEncoder::~nsDocumentEncoder()
{
}

NS_IMETHODIMP
nsDocumentEncoder::Init(nsIDocument* aDocument,
                        const nsAReadableString& aMimeType,
                        PRUint32 aFlags)
{
  if (!aDocument)
    return NS_ERROR_INVALID_ARG;

  mDocument = aDocument;

  mMimeType = aMimeType;

  mFlags = aFlags;

  return NS_OK;
}

NS_IMETHODIMP
nsDocumentEncoder::SetWrapColumn(PRUint32 aWC)
{
  mWrapColumn = aWC;
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentEncoder::SetSelection(nsIDOMSelection* aSelection)
{
  mSelection = aSelection;
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentEncoder::SetRange(nsIDOMRange* aRange)
{
  mRange = aRange;
  return NS_OK;
}

NS_IMETHODIMP
nsDocumentEncoder::SetCharset(const nsAReadableString& aCharset)
{
  mCharset = aCharset;
  return NS_OK;
}

nsresult
nsDocumentEncoder::SerializeNodeStart(nsIDOMNode* aNode, PRInt32 aStartOffset,
                                      PRInt32 aEndOffset,
                                      nsAWritableString& aStr)
{
  PRUint16 type;

  aNode->GetNodeType(&type);
  switch (type) {
    case nsIDOMNode::ELEMENT_NODE:
    {
      nsCOMPtr<nsIDOMElement> element = do_QueryInterface(aNode);
      mSerializer->AppendElementStart(element, aStr);
      break;
    }
    case nsIDOMNode::TEXT_NODE:
    {
      nsCOMPtr<nsIDOMText> text = do_QueryInterface(aNode);
      mSerializer->AppendText(text, aStartOffset, aEndOffset, aStr);
      break;
    }
    case nsIDOMNode::CDATA_SECTION_NODE:
    {
      nsCOMPtr<nsIDOMCDATASection> cdata = do_QueryInterface(aNode);
      mSerializer->AppendCDATASection(cdata, aStartOffset, aEndOffset, aStr);
      break;
    }
    case nsIDOMNode::PROCESSING_INSTRUCTION_NODE:
    {
      nsCOMPtr<nsIDOMProcessingInstruction> pi = do_QueryInterface(aNode);
      mSerializer->AppendProcessingInstruction(pi, aStartOffset, aEndOffset,
                                               aStr);
      break;
    }
    case nsIDOMNode::COMMENT_NODE:
    {
      nsCOMPtr<nsIDOMComment> comment = do_QueryInterface(aNode);
      mSerializer->AppendComment(comment, aStartOffset, aEndOffset, aStr);
      break;
    }
    case nsIDOMNode::DOCUMENT_TYPE_NODE:
    {
      nsCOMPtr<nsIDOMDocumentType> doctype = do_QueryInterface(aNode);
      mSerializer->AppendDoctype(doctype, aStr);
      break;
    }
  }
  
  return NS_OK;
}

nsresult
nsDocumentEncoder::SerializeNodeEnd(nsIDOMNode* aNode,
                                    nsAWritableString& aStr)
{
  PRUint16 type;

  aNode->GetNodeType(&type);
  switch (type) {
    case nsIDOMNode::ELEMENT_NODE:
    {
      nsCOMPtr<nsIDOMElement> element = do_QueryInterface(aNode);
      mSerializer->AppendElementEnd(element, aStr);
      break;
    }
  }

  return NS_OK;
}

nsresult
nsDocumentEncoder::SerializeToStringRecursive(nsIDOMNode* aNode,
                                              nsAWritableString& aStr)
{
  nsresult rv = SerializeNodeStart(aNode, 0, -1, aStr);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasChildren = PR_FALSE;

  aNode->HasChildNodes(&hasChildren);

  if (hasChildren) {
    nsCOMPtr<nsIDOMNodeList> childNodes;
    rv = aNode->GetChildNodes(getter_AddRefs(childNodes));
    NS_ENSURE_TRUE(childNodes, NS_SUCCEEDED(rv) ? NS_ERROR_FAILURE : rv);

    PRInt32 index, count;

    childNodes->GetLength((PRUint32*)&count);
    for (index = 0; index < count; index++) {
      nsCOMPtr<nsIDOMNode> child;

      rv = childNodes->Item(index, getter_AddRefs(child));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = SerializeToStringRecursive(child, aStr);
      NS_ENSURE_SUCCESS(rv, rv);     
    }
  }

  rv = SerializeNodeEnd(aNode, aStr);
  NS_ENSURE_SUCCESS(rv, rv);

  return FlushText(aStr, PR_FALSE);
}

static nsresult
ConvertAndWrite(nsAReadableString& aString,
                nsIOutputStream* aStream,
                nsIUnicodeEncoder* aEncoder)
{
  NS_ENSURE_ARG_POINTER(aStream);
  NS_ENSURE_ARG_POINTER(aEncoder);
  nsresult rv;
  PRInt32 charLength;
  const PRUnichar* unicodeBuf = (const PRUnichar*)nsPromiseFlatString(aString);
  PRInt32 unicodeLength = aString.Length();

  rv = aEncoder->GetMaxLength(unicodeBuf, unicodeLength, &charLength);
  if (NS_SUCCEEDED(rv)) {
    nsCAutoString charXferString;
    charXferString.SetCapacity(charLength);
    char* charXferBuf = (char*)charXferString.GetBuffer();

    rv = aEncoder->Convert(unicodeBuf, &unicodeLength, charXferBuf, &charLength);
    if (NS_SUCCEEDED(rv)) {
      PRUint32 written;
      rv = aStream->Write(charXferBuf, charLength, &written);
    }
  }

  return rv;
}

nsresult
nsDocumentEncoder::FlushText(nsAWritableString& aString, PRBool aForce)
{
  if (!mStream)
    return NS_OK;

  nsresult rv = NS_OK;

  if (aString.Length() > 1024 || aForce) {
    rv = ConvertAndWrite(aString, mStream, mUnicodeEncoder);

    aString.Truncate();
  }

  return rv;
}

static nsresult ChildAt(nsIDOMNode* aNode, PRInt32 aIndex, nsIDOMNode*& aChild)
{
  nsCOMPtr<nsIContent> node(do_QueryInterface(aNode));
  nsCOMPtr<nsIContent> child;

  aChild = nsnull;

  NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);

  node->ChildAt(aIndex, *getter_AddRefs(child));

  if (child)
    child->QueryInterface(NS_GET_IID(nsIDOMNode), (void **)&aChild);

  return NS_OK;
}

static PRInt32 IndexOf(nsIDOMNode* aParent, nsIDOMNode* aChild)
{
  nsCOMPtr<nsIContent> parent(do_QueryInterface(aParent));
  nsCOMPtr<nsIContent> child(do_QueryInterface(aChild));

  if (!parent || !child)
    return -1;

  PRInt32 indx = 0;

  parent->IndexOf(child, indx);

  return indx;
}

static inline PRInt32 GetIndex(nsVoidArray& aIndexArray)
{
  PRInt32 count = aIndexArray.Count();

  if (count) {
    return (PRInt32)aIndexArray.ElementAt(count - 1);
  }

  return 0;
}

static nsresult GetNextNode(nsIDOMNode* aNode, nsVoidArray& aIndexArray,
                            nsIDOMNode*& aNextNode,
                            nsRangeIterationDirection& aDirection)
{
  PRBool hasChildren;

  aNextNode = nsnull;

  aNode->HasChildNodes(&hasChildren);

  if (hasChildren && aDirection == kDirectionIn) {
    ChildAt(aNode, 0, aNextNode);
    NS_ENSURE_TRUE(aNextNode, NS_ERROR_FAILURE);

    aIndexArray.AppendElement((void *)0);

    aDirection = kDirectionIn;
  } else if (aDirection == kDirectionIn) {
    aNextNode = aNode;

    NS_ADDREF(aNextNode);

    aDirection = kDirectionOut;
  } else {
    nsCOMPtr<nsIDOMNode> parent;

    aNode->GetParentNode(getter_AddRefs(parent));
    NS_ENSURE_TRUE(parent, NS_ERROR_FAILURE);

    PRInt32 count = aIndexArray.Count();

    if (count) {
      PRInt32 indx = (PRInt32)aIndexArray.ElementAt(count - 1);

      ChildAt(parent, indx + 1, aNextNode);

      if (aNextNode)
        aIndexArray.ReplaceElementAt((void *)(indx + 1), count - 1);
      else
        aIndexArray.RemoveElementAt(count - 1);
    } else {
      PRInt32 indx = IndexOf(parent, aNode);

      if (indx >= 0) {
        ChildAt(parent, indx + 1, aNextNode);

        if (aNextNode)
          aIndexArray.AppendElement((void *)(indx + 1));
      }
    }

    if (aNextNode) {
      aDirection = kDirectionIn;
    } else {
      aDirection = kDirectionOut;

      aNextNode = parent;

      NS_ADDREF(aNextNode);
    }
  }

  return NS_OK;
}

nsresult
nsDocumentEncoder::SerializeRangeNodes(nsVoidArray& aAncestors,
                                       nsIDOMNode* aCommonParent,
                                       nsIDOMNode* aStart,
                                       PRInt32 aStartOffset,
                                       nsIDOMNode* aEnd,
                                       PRInt32 aEndOffset,
                                       nsAWritableString& aOutputString)
{
  PRInt32 i = aAncestors.Count();
  nsCOMPtr<nsIDOMNode> node;
  nsresult rv = NS_OK;

  node = NS_STATIC_CAST(nsIDOMNode *, aAncestors.ElementAt(--i));

  while (node) {
    SerializeNodeStart(node, 0, -1, aOutputString);

    node = NS_STATIC_CAST(nsIDOMNode *, aAncestors.ElementAt(--i));
  }

  nsCOMPtr<nsIDOMNode> start, endContainer, lastNode;

  nsRangeIterationDirection dir = kDirectionIn;

  PRUint16 type;
  aStart->GetNodeType(&type);

  if (type == nsIDOMNode::ELEMENT_NODE || type == nsIDOMNode::DOCUMENT_NODE) {
    rv = ChildAt(aStart, aStartOffset, *getter_AddRefs(start));
    NS_ENSURE_SUCCESS(rv, rv);

    if (!start) {
      start = aStart;

      dir = kDirectionOut;
    }

    aStartOffset = 0;
  } else {
    start = aStart;
  }

  aEnd->GetNodeType(&type);

  if (type == nsIDOMNode::ELEMENT_NODE || type == nsIDOMNode::DOCUMENT_NODE) {
    rv = ChildAt(aEnd, aEndOffset, *getter_AddRefs(lastNode));
    NS_ENSURE_SUCCESS(rv, rv);

    if (!lastNode) {
      // The endpoint of the range points to the end of the last child in
      // a container, in this case we set the endContainer to the container.

      endContainer = aEnd;
    } else {
      aEndOffset = -1;
    }
  } else {
    lastNode = aEnd;
  }

  if (start.get() == aEnd) {
    SerializeNodeStart(start, aStartOffset, aEndOffset, aOutputString); // Is this correct?
  } else {
    nsVoidArray offsets;

    SerializeNodeStart(start, aStartOffset, -1, aOutputString);

    GetNextNode(start, offsets, *getter_AddRefs(node), dir);

    while (node) {
      if (dir == kDirectionIn) {
        if (node == endContainer || node == lastNode) {
          if (aEndOffset > 0)
            SerializeNodeStart(node, 0, aEndOffset, aOutputString);

          if (endContainer) {
            if (GetIndex(offsets) >= aEndOffset)
              break;
          } else {
            break;
          }
        } else {
          SerializeNodeStart(node, 0, -1, aOutputString);
        }
      } else {
        if (node.get() == aEnd) {
          break;
        }

        SerializeNodeEnd(node, aOutputString);
      }

      nsIDOMNode *tmpNode = node;
      GetNextNode(tmpNode, offsets, *getter_AddRefs(node), dir);
    }
  }

  while (node && aCommonParent && node.get() != aCommonParent) {
    SerializeNodeEnd(node, aOutputString);

    nsIDOMNode *tmpNode = node;
    tmpNode->GetParentNode(getter_AddRefs(node));
  }

  nsCAutoString tmpStr; tmpStr.AssignWithConversion(aOutputString);
  printf ("range = '%s'\n", (const char *)tmpStr);

  return NS_OK;
}

nsresult
nsDocumentEncoder::SerializeRangeToString(nsIDOMRange *aRange,
                                          nsAWritableString& aOutputString)
{
  if (!aRange)
    return NS_OK;

  PRBool collapsed;

  aRange->GetCollapsed(&collapsed);

  if (collapsed)
    return NS_OK;

  nsCOMPtr<nsIDOMNode> start, end, commonParent;
  PRInt32 startOffset = 0, endOffset = 0;

  aRange->GetStartContainer(getter_AddRefs(start));
  NS_ENSURE_TRUE(start, NS_ERROR_FAILURE);

  aRange->GetStartOffset(&startOffset);

  aRange->GetEndContainer(getter_AddRefs(end));
  NS_ENSURE_TRUE(end, NS_ERROR_FAILURE);

  aRange->GetEndOffset(&endOffset);

  aRange->GetCommonAncestorContainer(getter_AddRefs(commonParent));

  if (!commonParent)
    return NS_OK;

  nsVoidArray ancestors;

  nsRange::FillArrayWithAncestors(&ancestors, start);
  ancestors.RemoveElementAt(0); // Remove 'start'

  PRInt32 i = ancestors.Count();

  while (i--) {
    nsIDOMNode *node = NS_STATIC_CAST(nsIDOMNode *, ancestors.ElementAt(i));

    ancestors.RemoveElementAt(i);

    if (!node || node == commonParent.get())
      break;
  }

  return SerializeRangeNodes(ancestors, commonParent, start, startOffset,
                             end, endOffset, aOutputString);
}

NS_IMETHODIMP
nsDocumentEncoder::EncodeToString(nsAWritableString& aOutputString)
{
  if (!mDocument)
    return NS_ERROR_NOT_INITIALIZED;

  aOutputString.Truncate();

  // xxx Also make sure mString is a mime type "text/html" or "text/plain"

  nsCAutoString progId(NS_CONTENTSERIALIZER_PROGID_PREFIX);
  progId.AppendWithConversion(mMimeType);

  mSerializer = do_CreateInstance(NS_STATIC_CAST(const char *, progId));
  NS_ENSURE_TRUE(mSerializer, NS_ERROR_NOT_IMPLEMENTED);

  mSerializer->Init(mFlags, mWrapColumn);

  nsresult rv = NS_OK;

  if (mSelection) {
    nsCOMPtr<nsIDOMRange> range;
    PRInt32 i, count = 0;

    rv = mSelection->GetRangeCount(&count);
    NS_ENSURE_SUCCESS(rv, rv);

    for (i = 0; i < count; i++) {
      mSelection->GetRangeAt(i, getter_AddRefs(range));

      rv = SerializeRangeToString(range, aOutputString);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    mSelection = nsnull;
  } else if (mRange) {
      rv = SerializeRangeToString(mRange, aOutputString);

      mRange = nsnull;
  } else {
    nsCOMPtr<nsIDOMNode> doc(do_QueryInterface(mDocument));

    rv = SerializeToStringRecursive(doc, aOutputString);
  }

  NS_ENSURE_SUCCESS(rv, rv);
  rv = mSerializer->Flush(aOutputString);

  return rv;
}

NS_IMETHODIMP
nsDocumentEncoder::EncodeToStream(nsIOutputStream* aStream)
{
  nsresult rv = NS_OK;

  if (!mDocument)
    return NS_ERROR_NOT_INITIALIZED;

  NS_WITH_SERVICE(nsICharsetConverterManager,
                  charsetConv, 
                  kCharsetConverterManagerCID,
                  &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString charsetStr;
  charsetStr.Assign(mCharset);
  rv = charsetConv->GetUnicodeEncoder(&charsetStr,
                                      getter_AddRefs(mUnicodeEncoder));
  NS_ENSURE_SUCCESS(rv, rv);

  // xxx Also make sure mString is a mime type "text/html" or "text/plain"

  mStream = aStream;

  nsAutoString buf;

  rv = EncodeToString(buf);

  // Force a flush of the last chunk of data.
  FlushText(buf, PR_TRUE);

  mStream = nsnull;
  mUnicodeEncoder = nsnull;

  return rv;
}

nsresult
NS_NewTextEncoder(nsIDocumentEncoder** aResult)
{
  *aResult = new nsDocumentEncoder;
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
 NS_ADDREF(*aResult);
 return NS_OK;
}


