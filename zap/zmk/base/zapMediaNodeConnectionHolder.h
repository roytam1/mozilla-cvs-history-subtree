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
 * The Original Code is Joost Technologies B.V. code.
 *
 * The Initial Developer of the Original Code is Joost Technologies B.V.
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Alex Fritze <alex@croczilla.com> (original author)
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

#ifndef __ZAP_MEDIANODECONNECTIONHOLDER_H__
#define __ZAP_MEDIANODECONNECTIONHOLDER_H__

#include "zapIMediaNode.h"
#include "zapIMediaSource.h"
#include "zapIMediaSink.h"
#include "nsIPropertyBag2.h"

////////////////////////////////////////////////////////////////////////
// zapMediaNodeConnectionHolder: Helper class for managing a
// connection between two media nodes.

class zapMediaNodeConnectionHolder
{
public:
  ~zapMediaNodeConnectionHolder() { Disconnect(); }
  
  nsresult Connect(zapIMediaNode* source_node, nsIPropertyBag2* source_pars,
                   zapIMediaNode* sink_node, nsIPropertyBag2* sink_pars)
  {
    Disconnect();

    // obtain source:
    nsCOMPtr<zapIMediaSource> source;
    NS_ASSERTION(source_node, "uh-oh, null source node");
    nsresult rv = source_node->GetSource(source_pars, getter_AddRefs(source));
    if (NS_FAILED(rv)) return rv;

    // obtain sink:
    nsCOMPtr<zapIMediaSink> sink;
    NS_ASSERTION(sink_node, "uh-oh, null sink node");
    rv = sink_node->GetSink(sink_pars, getter_AddRefs(sink));
    if (NS_FAILED(rv)) return rv;

    // try to form connection:
    rv = source->ConnectSink(sink);
    if (NS_FAILED(rv)) return rv;
    rv = sink->ConnectSource(source);
    if (NS_FAILED(rv)) {
      source->DisconnectSink(sink);
      return rv;
    }

    // success!
    mSource.swap(source);
    mSink.swap(sink);

    return NS_OK;
  }

  PRBool IsDisconnected() { return !mSink; }
  
  void Disconnect()
  {
    if (!mSource) {
      NS_ASSERTION(!mSink, "invalid state");
      return;
    }
    NS_ASSERTION(mSink, "invalid state");
    mSink->DisconnectSource(mSource);
    mSource->DisconnectSink(mSink);
    mSink = nsnull;
    mSource = nsnull;
  }

private:
  nsCOMPtr<zapIMediaSource> mSource;
  nsCOMPtr<zapIMediaSink> mSink;
};

#endif // __ZAP_MEDIANODECONNECTIONHOLDER_H__
