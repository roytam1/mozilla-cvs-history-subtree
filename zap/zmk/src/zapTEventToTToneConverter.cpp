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

#include "zapTEventToTToneConverter.h"
#include "zapITelephoneEventFrame.h"
#include "nsStringAPI.h"
#include "zapMediaUtils.h"
#include "stdio.h"

////////////////////////////////////////////////////////////////////////
// zapTEventToTToneConverter

zapTEventToTToneConverter::zapTEventToTToneConverter()
{
}

zapTEventToTToneConverter::~zapTEventToTToneConverter()
{
}

NS_IMETHODIMP
zapTEventToTToneConverter::InsertedIntoContainer(zapIMediaNodeContainer *container,
                                                 nsIPropertyBag2 *node_pars)
{
  return NS_OK;
}

NS_IMETHODIMP
zapTEventToTToneConverter::RemovedFromContainer()
{
  return NS_OK;
}

nsresult
zapTEventToTToneConverter::ValidateNewStream(nsIPropertyBag2* streamInfo)
{
  ZMK_VERIFY_STREAM_TYPE(streamInfo, "audio/telephone-event");

  // Create a new streaminfo:
  ZMK_CREATE_STREAM_INFO(mStreamInfo, "audio/tone");

  // Reset state:
  mCurrentFrameTimestamp = 0;
  mCurrentFrameSent = 0;
  mCurrentFrameEndSent = PR_FALSE;
  
  return NS_OK;
}

nsresult
zapTEventToTToneConverter::Filter(zapIMediaFrame* input, zapIMediaFrame** output)
{
  *output = nsnull;
  
  nsCOMPtr<zapITelephoneEventFrame> eventFrame = do_QueryInterface(input);
  if (!eventFrame) return NS_ERROR_FAILURE;

  PRBool M;
  PRBool E;
  PRUint64 timestamp;
  PRUint16 duration;
  PRUint16 volume;
  PRUint16 event;
  eventFrame->GetM(&M);
  eventFrame->GetE(&E);
  eventFrame->GetTimestamp(&timestamp);
  eventFrame->GetDuration(&duration);
  eventFrame->GetVolume(&volume);
  eventFrame->GetEvent(&event);
  

  if (E && timestamp == mCurrentFrameTimestamp && mCurrentFrameEndSent) {
    // this is a redundant end frame.
    // XXX what we really want to do here is request the next frame,
    // but with a passive filter that's not possible. -> ignore for now
    return NS_ERROR_FAILURE;
  }
  
  zapAudioToneFrame* outputFrame = CreateAudioToneFrame();
  outputFrame->mStreamInfo = mStreamInfo;
  
  if (NS_FAILED(SetFrequenciesForEvent(outputFrame, event))) {
    outputFrame->Release();
    return NS_ERROR_FAILURE;
  }

  outputFrame->SetM(M);
  outputFrame->SetVolume(volume);
  
  if (mCurrentFrameTimestamp != timestamp) {
    // this is a new event or a new segment. reset state:
    mCurrentFrameTimestamp = timestamp;
    mCurrentFrameSent = 0;
    mCurrentFrameEndSent = PR_FALSE;
  }
  
  outputFrame->SetDuration(duration - mCurrentFrameSent);
  outputFrame->mTimestamp = timestamp + mCurrentFrameSent;

  // update state:
  mCurrentFrameSent = duration;
  mCurrentFrameEndSent = E;
    
  *output = outputFrame;
  return NS_OK;
}

nsresult
zapTEventToTToneConverter::SetFrequenciesForEvent(zapAudioToneFrame*frame,
                                                  PRUint16 event)
{
  // column frequencies:
  switch (event) {
    case 1:
    case 4:
    case 7:
    case 10: /* '*' */
      frame->AddFrequency(1209);
      break;
    case 2:
    case 5:
    case 8:
    case 0:
      frame->AddFrequency(1336);
      break;
    case 3:
    case 6:
    case 9:
    case 11: /* '#' */
      frame->AddFrequency(1477);
      break;
    case 12: /* 'A' */
    case 13: /* 'B' */
    case 14: /* 'C' */
    case 15: /* 'D' */
      frame->AddFrequency(1633);
      break;
    default:
      NS_ERROR("unknown event");
      return NS_ERROR_FAILURE;
  }

  // row frequencies:
  switch (event) {
    case 1:
    case 2:
    case 3:
    case 12: /* 'A' */
      frame->AddFrequency(697);
      break;
    case 4:
    case 5:
    case 6:
    case 13: /* 'B' */
      frame->AddFrequency(770);
      break;
    case 7:
    case 8:
    case 9:
    case 14: /* 'C' */
      frame->AddFrequency(852);
      break;
    case 10: /* '*' */
    case 0:
    case 11: /* '#' */
    case 15: /* 'D' */
      frame->AddFrequency(941);
      break;
  }
  return NS_OK;
}
