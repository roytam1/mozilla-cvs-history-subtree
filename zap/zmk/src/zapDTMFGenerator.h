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
 * Portions created by the Initial Developer are Copyright (C) 2006
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

#ifndef __ZAP_DTMFGENERATOR_H__
#define __ZAP_DTMFGENERATOR_H__

#include "zapIMediaNode.h"
#include "zapIMediaSource.h"
#include "zapIMediaSink.h"
#include "zapIMediaGraph.h"
#include "zapIMediaFrame.h"
#include "nsCOMPtr.h"
#include "nsIWritablePropertyBag2.h"
#include "zapIDTMFGenerator.h"
#include "nsDeque.h"

////////////////////////////////////////////////////////////////////////
// zapDTMFGenerator

// {8A380362-B150-4B9C-88CF-1A8788699BF0}
#define ZAP_DTMFGENERATOR_CID                             \
  { 0x8a380362, 0xb150, 0x4b9c, { 0x88, 0xcf, 0x1a, 0x87, 0x88, 0x69, 0x9b, 0xf0 } }

#define ZAP_DTMFGENERATOR_CONTRACTID ZAP_MEDIANODE_CONTRACTID_PREFIX "dtmf-generator"

class zapDTMFGenerator : public zapIMediaNode,
                         public zapIMediaSource,
                         public zapIDTMFGenerator
{
public:
  zapDTMFGenerator();
  ~zapDTMFGenerator();
  
  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIMEDIANODE
  NS_DECL_ZAPIMEDIASOURCE
  NS_DECL_ZAPIDTMFGENERATOR
  
private:
  void ClearState();
  nsresult ParseDTMFData(const char *buf);
  void QueueTEvent(PRUint16 event);
  
  // node parameters (set from zapIMediaGraph::AddNode()):
  PRUint32 mToneDuration; // tone duration in sample units
  PRUint32 mPauseDuration; // pause duration in sample units
  PRUint16 mVolume; // volume in dBm0 with reversed sign
  PRUint16 mMaxSamplesPerFrame;

  nsDeque mBuffer; // tevent buffer
  PRUint32 mTEventSamplesPlayed; // samples played for current tevent 
  PRUint64 mSampleClock;
  
  nsCOMPtr<nsIWritablePropertyBag2> mStreamInfo;

  nsCOMPtr<zapIMediaSink> mOutput;
};

#endif // __ZAP_DTMFGENERATOR_H__
