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

#ifndef __ZAP_AUDIOOUT_H__
#define __ZAP_AUDIOOUT_H__

#include "zapIMediaNode.h"
#include "zapIMediaSource.h"
#include "zapIMediaSink.h"
#include "zapIMediaGraph.h"
#include "zapIMediaFrame.h"
#include "zapIAudioOut.h"
#include "nsCOMPtr.h"
#include "portaudio.h"
#include "nsIEventTarget.h"
#include "nsIPropertyBag2.h"
#include "zapAudioStreamUtils.h"

////////////////////////////////////////////////////////////////////////
// zapAudioOut

// {1FE87612-773C-4FBC-8EF2-C7509F6584D0}
#define ZAP_AUDIOOUT_CID                             \
  { 0x1fe87612, 0x773c, 0x4fbc, { 0x8e, 0xf2, 0xc7, 0x50, 0x9f, 0x65, 0x84, 0xd0 } }

#define ZAP_AUDIOOUT_CONTRACTID ZAP_MEDIANODE_CONTRACTID_PREFIX "audioout"

class zapAudioOut : public zapIMediaNode,
                    public zapIMediaSink,
                    public zapIAudioOut
{
public:
  zapAudioOut();
  ~zapAudioOut();

  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIMEDIANODE
  NS_DECL_ZAPIMEDIASINK
  NS_DECL_ZAPIAUDIOOUT

private:
  friend class zapAudioOutPlayFrameEvent;
  friend int AudioOutCallback(void* inputBuffer, void* outputBuffer,
                              unsigned long framesPerBuffer,
                              PaTimestamp outTime, void* userData);

  void PlayFrame(void* outputBuffer);
  PRBool ValidateFrame(zapIMediaFrame* frame);
  nsresult StartStream();
  void CloseStream();
    
  nsCOMPtr<zapIMediaGraph> mGraph;
  nsCOMPtr<nsIEventTarget> mEventTarget;   // media graph event target

  // node parameters (set in zapIMediaGraph::AddNode()):
  PaDeviceID mOutputDevice;
  zapAudioStreamParameters mStreamParameters;
  
  PortAudioStream* mStream;
  PRUint32 mBuffers; // number of internal port audio buffers (>=2)

  nsCOMPtr<zapIMediaSource> mInput;
  nsCOMPtr<nsIPropertyBag2> mLastValidStreamInfo; // the most recent
                                                  // valid stream info
                                                  // (used for
                                                  // detecting stream
                                                  // breaks)
};

#endif // __ZAP_AUDIOOUT_H__
