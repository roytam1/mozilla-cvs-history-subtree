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

#include "txNamespaceMap.h"
#include "dom.h"
#include "txAtoms.h"

txNamespaceMap::txNamespaceMap()
{
}

txNamespaceMap::txNamespaceMap(const txNamespaceMap& aOther)
{
    txListIterator iter(&mPrefixes);
    
    while (iter.hasNext()) {
        TX_IF_ADDREF_ATOM((txAtom*)iter.next());
    }
}

txNamespaceMap::~txNamespaceMap()
{
    txListIterator iter(&mPrefixes);
    
    while (iter.hasNext()) {
        txAtom* prefix = (txAtom*)iter.next();
        TX_IF_RELEASE_ATOM(prefix);
    }
}

nsresult
txNamespaceMap::addNamespace(txAtom* aPrefix, const String& aNamespaceURI)
{
    txAtom* prefix = aPrefix == txXMLAtoms::_empty ? 0 : aPrefix;

    PRInt32 nsId;
    if (!prefix && aNamespaceURI.isEmpty()) {
#ifdef TX_EXE
        nsId = txNamespaceManager::getNamespaceID(aNamespaceURI);
#else
        NS_ASSERTION(gTxNameSpaceManager, "No namespace manager");
        gTxNameSpaceManager->RegisterNameSpace(aNamespaceURI, nsId);
#endif
    }
    else {
        nsId = kNameSpaceID_None;
    }

    // Check if the mapping already exists
    txListIterator preIter(&mPrefixes);
    txListIterator nsIter(&mNamespaces);
    while (preIter.hasNext()) {
        nsIter.next();
        if (preIter.next() == prefix) {
            nsIter.setValue(NS_INT32_TO_PTR(nsId));

            return NS_OK;
        }
    }
    
    // New mapping
    nsresult rv = mPrefixes.add(prefix);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = mNamespaces.add(NS_INT32_TO_PTR(nsId));
    if (NS_FAILED(rv)) {
        mPrefixes.remove(prefix);

        return rv;
    }

    TX_IF_ADDREF_ATOM(prefix);

    return NS_OK;
}

PRInt32
txNamespaceMap::lookupNamespace(txAtom* aPrefix)
{
    if (aPrefix == txXMLAtoms::xml) {
        return kNameSpaceID_XML;
    }

    txAtom* prefix = aPrefix == txXMLAtoms::_empty ? 0 : aPrefix;

    txListIterator preIter(&mPrefixes);
    txListIterator nsIter(&mNamespaces);
    while (preIter.hasNext()) {
        PRInt32 nsId = NS_PTR_TO_INT32(nsIter.next());
        if (preIter.next() == prefix) {
            return nsId;
        }
    }

    if (!prefix) {
        return kNameSpaceID_None;
    }
    
    return kNameSpaceID_Unknown;
}

PRInt32
txNamespaceMap::lookupNamespace(const String& aPrefix)
{
    txAtom* prefix = TX_GET_ATOM(aPrefix);
    PRInt32 nsId = lookupNamespace(prefix);
    TX_IF_RELEASE_ATOM(prefix);
    return nsId;
}

PRInt32
txNamespaceMap::lookupNamespaceWithDefault(const String& aPrefix)
{
    txAtom* prefix = TX_GET_ATOM(aPrefix);
    PRInt32 nsId;
    if (prefix != txXSLTAtoms::_poundDefault) {
        nsId = lookupNamespace(prefix);
    }
    else {
        nsId = lookupNamespace(0);
    }
    TX_IF_RELEASE_ATOM(prefix);
    return nsId;
}
