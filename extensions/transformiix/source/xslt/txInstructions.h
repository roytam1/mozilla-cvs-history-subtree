/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is TransforMiiX XSLT processor.
 *
 * The Initial Developer of the Original Code is
 * Jonas Sicking.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * Jonas Sicking. All Rights Reserved.
 *
 * Contributor(s):
 *   Jonas Sicking <jonas@sicking.cc>
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

#ifndef TRANSFRMX_TXINSTRUCTIONS_H
#define TRANSFRMX_TXINSTRUCTIONS_H

#include "txError.h"
#include "txExecutionState.h"
#include "nsCOMPtr.h"
#include "TxObject.h"
#include "nsIAtom.h"
#include "nsString.h"
#include "XMLUtils.h"

class Expr;

class txInstruction : public TxObject
{
public:
    txInstruction() : mNext(0)
    {
    }

    virtual ~txInstruction()
    {
        delete mNext;
    }

    virtual nsresult execute(txExecutionState& aEs) = 0;

    txInstruction* mNext;
};

#define TX_DECL_TXINSTRUCTION  \
    virtual nsresult execute(txExecutionState& aEs);


class txApplyTemplates : public txInstruction
{
public:
    txApplyTemplates(const txExpandedName& aMode);

    TX_DECL_TXINSTRUCTION
    
    txExpandedName mMode;
};

class txAttribute : public txInstruction
{
public:
    txAttribute(Expr* aName, Expr* aNamespace,
                   const txNamespaceMap& aMappings);
    ~txAttribute();

    TX_DECL_TXINSTRUCTION

    Expr* mName;
    Expr* mNamespace;
    txNamespaceMap mMappings;
};

class txCallTemplate : public txInstruction
{
public:
    txCallTemplate(const txExpandedName& aName);

    TX_DECL_TXINSTRUCTION

    txExpandedName mName;
};

class txConditionalGoto : public txInstruction
{
public:
    txConditionalGoto(Expr* aCondition, txInstruction* aTarget);
    ~txConditionalGoto();

    TX_DECL_TXINSTRUCTION
    
    Expr* mCondition;
    txInstruction* mTarget;
};

class txCreateComment : public txInstruction
{
public:
    TX_DECL_TXINSTRUCTION
};

class txEndElement : public txInstruction
{
public:
    TX_DECL_TXINSTRUCTION
};

class txForEach : public txInstruction
{
public:
    txForEach();

    TX_DECL_TXINSTRUCTION

    txInstruction* mEndTarget;
};

class txGoTo : public txInstruction
{
public:
    txGoTo(txInstruction* aTarget);

    TX_DECL_TXINSTRUCTION
    
    txInstruction* mTarget;
};

class txInsertAttrSet : public txInstruction
{
public:
    txInsertAttrSet(const txExpandedName& aName);

    TX_DECL_TXINSTRUCTION

    txExpandedName mName;
};

class txLREAttribute : public txInstruction
{
public:
    txLREAttribute(PRInt32 aNamespaceID, nsIAtom* aLocalName,
                   nsIAtom* aPrefix, Expr* aValue);
    virtual ~txLREAttribute();

    TX_DECL_TXINSTRUCTION

    PRInt32 mNamespaceID;
    nsCOMPtr<nsIAtom> mLocalName;
    nsCOMPtr<nsIAtom> mPrefix;
    Expr* mValue;
};

class txMessage : public txInstruction
{
public:
    txMessage(PRBool aTerminate);

    TX_DECL_TXINSTRUCTION

    PRBool mTerminate;
};

class txProcessingInstruction : public txInstruction
{
public:
    txProcessingInstruction(Expr* aName);

    TX_DECL_TXINSTRUCTION

    Expr* mName;
};

class txPushNewContext : public txInstruction
{
public:
    txPushNewContext(Expr* aSelect);
    ~txPushNewContext();

    TX_DECL_TXINSTRUCTION
    
    //addSort(Expr* aSelectExpr, Expr* aLangExpr,
    //        Expr* aDataTypeExpr, Expr* aOrderExpr,
    //        Expr* aCaseOrderExpr, txIEvalContext* aContext);

    Expr* mSelect;
};

class txPushStringHandler : public txInstruction
{
public:
    txPushStringHandler(PRBool aOnlyText);

    TX_DECL_TXINSTRUCTION

    PRBool mOnlyText;
};

class txRecursionCheckpointEnd : public txInstruction
{
public:
    TX_DECL_TXINSTRUCTION
};

class txRecursionCheckpointStart : public txInstruction
{
public:
    txRecursionCheckpointStart(const nsAString& aName);

    TX_DECL_TXINSTRUCTION
    
    nsString mName;
};

class txReturn : public txInstruction
{
    TX_DECL_TXINSTRUCTION
};

class txStartElement : public txInstruction
{
public:
    txStartElement(Expr* aName, Expr* aNamespace,
                   const txNamespaceMap& aMappings);
    ~txStartElement();

    TX_DECL_TXINSTRUCTION

    Expr* mName;
    Expr* mNamespace;
    txNamespaceMap mMappings;
};

class txStartLREElement : public txInstruction
{
public:
    txStartLREElement(PRInt32 aNamespaceID, nsIAtom* aLocalName,
                      nsIAtom* aPrefix);

    TX_DECL_TXINSTRUCTION

    PRInt32 mNamespaceID;
    nsCOMPtr<nsIAtom> mLocalName;
    nsCOMPtr<nsIAtom> mPrefix;
};

class txText : public txInstruction
{
public:
    txText(const nsAString& aStr, PRBool aDOE);

    TX_DECL_TXINSTRUCTION

    nsString mStr;
    PRBool mDOE;
};

class txValueOf : public txInstruction
{
public:
    txValueOf(Expr* aExpr, PRBool aDOE);
    ~txValueOf();

    TX_DECL_TXINSTRUCTION

    Expr* mExpr;
    PRBool mDOE;
};

#endif //TRANSFRMX_TXINSTRUCTIONS_H
