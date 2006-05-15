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

#ifndef __ZAP_AUDIOIO_H__
#define __ZAP_AUDIOIO_H__

#include "zapIMediaNode.h"
#include "zapIMediaSource.h"
#include "zapIMediaSink.h"
#include "zapIMediaGraph.h"
#include "zapIMediaFrame.h"
#include "zapIAudioIO.h"
#include "nsCOMPtr.h"
#include "portaudio.h"
#include "nsIEventTarget.h"
#include "nsIWritablePropertyBag2.h"
#include "zapAudioStreamUtils.h"

class zapAudioIOMonitor;

////////////////////////////////////////////////////////////////////////
// zapAudioIO

// {57429DEE-A835-46A8-BA52-1E8839BA57AF}
#define ZAP_AUDIOIO_CID                             \
  { 0x57429dee, 0xa835, 0x46a8, { 0xba, 0x52, 0x1e, 0x88, 0x39, 0xba, 0x57, 0xaf } }

#define ZAP_AUDIOIO_CONTRACTID ZAP_MEDIANODE_CONTRACTID_PREFIX "audioio"

class zapAudioIO : public zapIMediaNode,
                   public zapIMediaSink,
                   public zapIMediaSource,
                   public zapIAudioIO
{
public:
  zapAudioIO();
  ~zapAudioIO();

  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIMEDIANODE
  NS_DECL_ZAPIMEDIASINK
  NS_DECL_ZAPIMEDIASOURCE
  NS_DECL_ZAPIAUDIOIO

private:
  friend class zapAudioIOMonitor;
  friend class zapAudioIOEvent;
  friend int AudioIOCallback(void* inputBuffer, void* outputBuffer,
                             unsigned long framesPerBuffer,
                             PaTimestamp outTime, void* userData);
  PRBool mKeepRunning; // signals the portaudio callback whether to
                       // shut down or not

  void ProcessInputFrame(void* outputBuffer);
  PRBool ValidateInputFrame(zapIMediaFrame* frame);
  void CreateOutputFrame(double timestamp, void* inputBuffer);
  nsresult StartStream();
  void CloseStream();
    
  nsCOMPtr<zapIMediaGraph> mGraph;
  nsCOMPtr<nsIEventTarget> mEventTarget;   // media graph event target

  // node parameters (set in zapIMediaGraph::AddNode()):
  PRUint32 mBuffers; // number of internal port audio buffers (>=2)
  PaDeviceID mInputDevice;
  PaDeviceID mOutputDevice;
  zapAudioStreamParameters mStreamParameters;
  
  PortAudioStream* mStream;

  zapAudioIOMonitor *mAudioIOMonitor; // weak reference
  nsCOMPtr<zapIMediaSink> mOutput;
  nsCOMPtr<zapIMediaSource> mInput;
  nsCOMPtr<nsIWritablePropertyBag2> mOutputStreamInfo;
  nsCOMPtr<nsIPropertyBag2> mLastValidInputStreamInfo;
};

#endif // __ZAP_AUDIOIO_H__
