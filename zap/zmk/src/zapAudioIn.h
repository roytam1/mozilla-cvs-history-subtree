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

#ifndef __ZAP_AUDIOIN_H__
#define __ZAP_AUDIOIN_H__

#include "zapIMediaNode.h"
#include "zapIMediaSource.h"
#include "zapIMediaSink.h"
#include "zapIMediaGraph.h"
#include "zapIMediaFrame.h"
#include "zapIAudioIn.h"
#include "nsCOMPtr.h"
#include "portaudio.h"
#include "nsIWritablePropertyBag2.h"
#include "zapAudioStreamUtils.h"
#include "nsThreadUtils.h"

class zapAudioIn;

////////////////////////////////////////////////////////////////////////
// zapAudioInEvent
// revocable event helper class 

class zapAudioInEvent : public nsRunnable
{
public:
  zapAudioInEvent(zapAudioIn* audioin)
      : mAudioIn(audioin)
  {
  }

  NS_IMETHOD Run();
  
  void Revoke() {
    mAudioIn = nsnull;
  }
  
  nsCString data;
  double timestamp;

private:
  zapAudioIn *mAudioIn; // weak ref
};

////////////////////////////////////////////////////////////////////////
// zapAudioIn

// {FF982BFD-61EB-4C5E-9C11-90584445399A}
#define ZAP_AUDIOIN_CID                             \
  { 0xff982bfd, 0x61eb, 0x4c5e, { 0x9c, 0x11, 0x90, 0x58, 0x44, 0x45, 0x39, 0x9a } }

#define ZAP_AUDIOIN_CONTRACTID ZAP_MEDIANODE_CONTRACTID_PREFIX "audioin"

class zapAudioIn : public zapIMediaNode,
                   public zapIMediaSource,
                   public zapIAudioIn
{
public:
  zapAudioIn();
  ~zapAudioIn();
  
  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIMEDIANODE
  NS_DECL_ZAPIMEDIASOURCE
  NS_DECL_ZAPIAUDIOIN

private:
  friend class zapAudioInEvent;
  nsRevocableEventPtr<zapAudioInEvent> mEvent;
  
  nsresult StartStream();
  void CloseStream();
  
  void CreateFrame(const nsACString& data, double timestamp);
  void AudioInEventDone();
  
  // node parameters (set from zapIMediaGraph::AddNode()):
  PaDeviceID mInputDevice;
  zapAudioStreamParameters mStreamParameters;

  PortAudioStream* mStream;
  PRUint32 mBuffers; // number of internal port audio buffers (>=2)
  
  nsCOMPtr<nsIWritablePropertyBag2> mStreamInfo;
  
  nsCOMPtr<zapIMediaGraph> mGraph; // media graph in which this node lives
  nsCOMPtr<nsIEventTarget> mEventTarget; // media graph event target
  
  friend int AudioInCallback(void* inputBuffer, void* outputBuffer,
                             unsigned long framesPerBuffer,
                             PaTimestamp outTime, void* userData);
  PRLock* mCallbackLock; // lock to synchronize portaudio and media threads
  PRBool mKeepRunning; // signals the portaudio callback whether to
                       // shut down or not

  nsCOMPtr<zapIMediaSink> mOutput;
};

#endif // __ZAP_AUDIOIN_H__
