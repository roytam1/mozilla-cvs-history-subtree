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
 * The Original Code is the Mozilla SIP client project.
 *
 * The Initial Developer of the Original Code is 8x8 Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
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

#ifndef __ZAP_FILTERNODE_H__
#define __ZAP_FILTERNODE_H__

#include "zapIMediaNode.h"
#include "zapIMediaSource.h"
#include "zapIMediaSink.h"
#include "nsCOMPtr.h"

////////////////////////////////////////////////////////////////////////
// zapFilterNode: baseclass for filter-type nodes

class zapFilterNode : public zapIMediaNode,
                      public zapIMediaSource,
                      public zapIMediaSink
{
  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIMEDIASOURCE
  NS_DECL_ZAPIMEDIASINK
  
  // zapIMediaSource:
  NS_IMETHOD GetSource(nsIPropertyBag2 *source_pars, zapIMediaSource **_retval);
  NS_IMETHOD GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval);
  
  zapFilterNode();
  virtual ~zapFilterNode();

  // methods to be implemented by subclass:
  // NS_IMETHOD AddedToGraph(zapIMediaGraph *graph, const nsACString & id, nsIPropertyBag2 *node_pars) = 0;
  // NS_IMETHOD RemovedFromGraph(zapIMediaGraph *graph) = 0;
  virtual nsresult OpenStream(nsIPropertyBag2* streamInfo)=0;
  virtual void CloseStream()=0;
  virtual nsresult Filter(zapIMediaFrame* input, zapIMediaFrame** output)=0;
  
private:
  PRBool mStreamOpen;
  PRBool mWaiting;
  enum { IDLE_CLOSED, WAITING_CLOSED, IDLE_RUNNING, WAITING_RUNNING } mState;
  nsCOMPtr<zapIMediaSink> mSink;
  nsCOMPtr<zapIMediaSource> mSource;
};

#endif // __ZAP_FILTER_H__
