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
#include "nsIEventQueue.h"
#include "nsIWritablePropertyBag2.h"
#include "zapAudioStreamUtils.h"

class zapAudioInState;

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
  friend class zapAudioInState;
  friend class zapAudioIn_STOP_IDLE_CLOSED;
  friend class zapAudioIn_PLAY_IDLE_CLOSED;
  friend class zapAudioIn_PLAY_IDLE_OPEN;
  friend class zapAudioIn_STOP_WAITING_CLOSED;
  friend class zapAudioIn_STOP_IDLEEOF_CLOSED;
  friend class zapAudioIn_PLAY_IDLEEOF_CLOSED;
  friend class zapAudioIn_PLAY_WAITING_OPEN;
  
  void ChangeState(zapAudioInState* state);
    
  zapAudioInState* mState;
  
  nsresult StartStream();
  void CloseStream();
  void SendFrame(const nsACString& data, double timestamp);
  
  // node parameters (set from zapIMediaGraph::AddNode()):
  PaDeviceID mInputDevice;
  
  // source parameters (set from zapIMediaGraph::Connect()):
  double mSampleRate;
  double mFrameDuration;
  PRUint32 mNumChannels;
  zapAudioStreamSampleFormat mSampleFormat;
  PRUint32 mSamplesPerFrame;

  nsCOMPtr<zapIMediaSink> mSink;

  PortAudioStream* mStream;
  nsCOMPtr<nsIWritablePropertyBag2> mStreamInfo;
  
  nsCOMPtr<nsIEventQueue> mEventQ; // media graph event queue
  
  friend class zapAudioInSendEvent;
  friend int AudioInCallback(void* inputBuffer, void* outputBuffer,
                             unsigned long framesPerBuffer,
                             PaTimestamp outTime, void* userData);
  PRLock* mCallbackLock; // lock to synchronize portaudio and media threads
  PRBool mKeepRunning; // signals the portaudio callback whether to
                       // shut down or not
  
};

////////////////////////////////////////////////////////////////////////
// zapAudioInState: baseclass for zapAudioIn states

class zapAudioInState
{
public:
  // zapIAudioIn handlers:
  virtual nsresult Play(zapAudioIn* audioin)=0;
  virtual nsresult Stop(zapAudioIn* audioin)=0;
  
  // zapIMediaSource handlers:
  virtual nsresult ConnectSink(zapAudioIn* audioin, zapIMediaSink* sink,
                               const nsACString& connection_id);
  virtual nsresult DisconnectSink(zapAudioIn* audioin, zapIMediaSink *sink,
                                  const nsACString & connection_id)=0;
  virtual nsresult RequestFrame(zapAudioIn* audioin);

  // portaudio callback handlers:
  virtual void SendFrame(zapAudioIn* audioin, zapIMediaFrame* frame);

#ifdef DEBUG
  virtual const char* GetName()=0;
#endif
  
protected:
  void ChangeState(zapAudioIn* audioin, zapAudioInState* state);
};

////////////////////////////////////////////////////////////////////////
// zapAudioIn_STOP_IDLE_CLOSED

class zapAudioIn_STOP_IDLE_CLOSED : public zapAudioInState
{
public:
  static zapAudioInState* Instance();

  virtual nsresult Play(zapAudioIn* audioin);
  virtual nsresult Stop(zapAudioIn* audioin);
  
  virtual nsresult ConnectSink(zapAudioIn* audioin, zapIMediaSink* sink,
                               const nsACString& connection_id);
  virtual nsresult DisconnectSink(zapAudioIn* audioin, zapIMediaSink *sink,
                                  const nsACString & connection_id);
  virtual nsresult RequestFrame(zapAudioIn* audioin);
  
#ifdef DEBUG
  virtual const char* GetName() {
    static const char* name = "STOP_IDLE_CLOSED";
    return name;
  }
#endif
private:
  static zapAudioInState* mInstance;
};

////////////////////////////////////////////////////////////////////////
// zapAudioIn_PLAY_IDLE_CLOSED: waiting for sink to connect and/or
// request first frame

class zapAudioIn_PLAY_IDLE_CLOSED : public zapAudioInState
{
public:
  static zapAudioInState* Instance();

  virtual nsresult Play(zapAudioIn* audioin);
  virtual nsresult Stop(zapAudioIn* audioin);
  
  virtual nsresult ConnectSink(zapAudioIn* audioin, zapIMediaSink* sink,
                               const nsACString& connection_id);
  virtual nsresult DisconnectSink(zapAudioIn* audioin, zapIMediaSink *sink,
                                  const nsACString & connection_id);
  virtual nsresult RequestFrame(zapAudioIn* audioin);
#ifdef DEBUG
  virtual const char* GetName() {
    static const char* name = "PLAY_IDLE_CLOSED";
    return name;
  }
#endif

private:
  static zapAudioInState* mInstance;
};

////////////////////////////////////////////////////////////////////////
// zapAudioIn_PLAY_IDLE_OPEN

class zapAudioIn_PLAY_IDLE_OPEN : public zapAudioInState
{
public:
  static zapAudioInState* Instance();

  virtual nsresult Play(zapAudioIn* audioin);
  virtual nsresult Stop(zapAudioIn* audioin);
  
  virtual nsresult DisconnectSink(zapAudioIn* audioin, zapIMediaSink *sink,
                                  const nsACString & connection_id);
  virtual nsresult RequestFrame(zapAudioIn* audioin);

  virtual void SendFrame(zapAudioIn* audioin, zapIMediaFrame* frame);
#ifdef DEBUG
  virtual const char* GetName() {
    static const char* name = "PLAY_IDLE_OPEN";
    return name;
  }
#endif

private:
  static zapAudioInState* mInstance;
};

////////////////////////////////////////////////////////////////////////
// zapAudioIn_STOP_WAITING_CLOSED

class zapAudioIn_STOP_WAITING_CLOSED : public zapAudioInState
{
public:
  static zapAudioInState* Instance();

  virtual nsresult Play(zapAudioIn* audioin);
  virtual nsresult Stop(zapAudioIn* audioin);
  
  virtual nsresult DisconnectSink(zapAudioIn* audioin, zapIMediaSink *sink,
                                  const nsACString & connection_id);
#ifdef DEBUG
  virtual const char* GetName() {
    static const char* name = "STOP_WAITING_CLOSED";
    return name;
  }
#endif

private:
  static zapAudioInState* mInstance;
};

////////////////////////////////////////////////////////////////////////
// zapAudioIn_STOP_IDLEEOF_CLOSED

class zapAudioIn_STOP_IDLEEOF_CLOSED : public zapAudioInState
{
public:
  static zapAudioInState* Instance();

  virtual nsresult Play(zapAudioIn* audioin);
  virtual nsresult Stop(zapAudioIn* audioin);
  
  virtual nsresult DisconnectSink(zapAudioIn* audioin, zapIMediaSink *sink,
                                  const nsACString & connection_id);
  virtual nsresult RequestFrame(zapAudioIn* audioin);
#ifdef DEBUG
  virtual const char* GetName() {
    static const char* name = "STOP_IDLEEOF_CLOSED";
    return name;
  }
#endif

private:
  static zapAudioInState* mInstance;
};

////////////////////////////////////////////////////////////////////////
// zapAudioIn_STOP_IDLEEOF_CLOSED

class zapAudioIn_PLAY_IDLEEOF_CLOSED : public zapAudioInState
{
public:
  static zapAudioInState* Instance();

  virtual nsresult Play(zapAudioIn* audioin);
  virtual nsresult Stop(zapAudioIn* audioin);
  
  virtual nsresult DisconnectSink(zapAudioIn* audioin, zapIMediaSink *sink,
                                  const nsACString & connection_id);
  virtual nsresult RequestFrame(zapAudioIn* audioin);
#ifdef DEBUG
  virtual const char* GetName() {
    static const char* name = "PLAY_IDLEEOF_CLOSED";
    return name;
  }
#endif

private:
  static zapAudioInState* mInstance;
};


////////////////////////////////////////////////////////////////////////
// zapAudioIn_PLAY_WAITING_OPEN

class zapAudioIn_PLAY_WAITING_OPEN : public zapAudioInState
{
public:
  static zapAudioInState* Instance();

  virtual nsresult Play(zapAudioIn* audioin);
  virtual nsresult Stop(zapAudioIn* audioin);
  
  virtual nsresult DisconnectSink(zapAudioIn* audioin, zapIMediaSink *sink,
                                  const nsACString & connection_id);

  virtual void SendFrame(zapAudioIn* audioin, zapIMediaFrame* frame);
#ifdef DEBUG
  virtual const char* GetName() {
    static const char* name = "PLAY_WAITING_OPEN";
    return name;
  }
#endif

private:
  static zapAudioInState* mInstance;
};


#endif // __ZAP_AUDIOIN_H__
