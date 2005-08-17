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

#ifndef __ZAP_PACKETBUFFER_H__
#define __ZAP_PACKETBUFFER_H__

#include "zapIMediaNode.h"
#include "zapIMediaSource.h"
#include "zapIMediaSink.h"
#include "nsDeque.h"
#include "nsCOMPtr.h"
#include "nsIEventQueue.h"

////////////////////////////////////////////////////////////////////////
// zapPacketBuffer

// {2FD0FE12-0727-4EED-96EE-94901F4FE201}
#define ZAP_PACKETBUFFER_CID                             \
  { 0x2fd0fe12, 0x0727, 0x4eed, { 0x96, 0xee, 0x94, 0x90, 0x1f, 0x4f, 0xe2, 0x01 } }

#define ZAP_PACKETBUFFER_CONTRACTID ZAP_MEDIANODE_CONTRACTID_PREFIX "buffer"

class zapPacketBuffer : public zapIMediaNode,
                        public zapIMediaSource,
                        public zapIMediaSink
{
  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIMEDIANODE
  NS_DECL_ZAPIMEDIASOURCE
  NS_DECL_ZAPIMEDIASINK

  zapPacketBuffer();
  ~zapPacketBuffer();

private:
  friend class zapPacketBufferRequestFrameEvent;
  friend class zapPacketBufferSourceState;
  friend class zapPacketBufferSource_STOP_IDLE;
  friend class zapPacketBufferSource_STOP_WAITING;
  friend class zapPacketBufferSource_RUN_WAITING;
  friend class zapPacketBufferSource_RUN_IDLE;
  friend class zapPacketBufferSinkState;
  friend class zapPacketBufferSink_WAITING_PREFILLING;
  friend class zapPacketBufferSink_WAITING;
  friend class zapPacketBufferSink_IDLE;
  friend class zapPacketBufferSink_IDLE_PREFILLING;
  
  zapPacketBufferSourceState* mSourceState;
  zapPacketBufferSinkState* mSinkState;

  void ChangeSourceState(zapPacketBufferSourceState* state);
  void ChangeSinkState(zapPacketBufferSinkState* state);
  void RunQueueing();
  void StopQueueing();
  void PacketQueued();
  void PacketDequeued();
  void ClearBuffer();
  void PostFrameRequest();
  
  // node parameters (set in zapIMediaGraph::AddNode()):  
  PRUint32 mPrefillSize; // number of packets to prefill the buffer with
  PRUint32 mMaxSize; // maximum number of packets buffered
  PRBool mRebuffer; // rebuffer to prefill_size whenever the number of packets
                    // in the buffer drops to 0

  nsDeque mBuffer; // the actual buffer

  nsCOMPtr<nsIEventQueue> mEventQueue; // media graph event queue

  nsCOMPtr<zapIMediaSink> mSink;
  nsCOMPtr<zapIMediaSource> mSource;
};

////////////////////////////////////////////////////////////////////////
// zapPacketBufferSourceState: baseclass for zapPacketBuffer source
// states

class zapPacketBufferSourceState 
{
public:
  virtual void PacketDequeued(zapPacketBuffer* pb)=0;
  virtual void RunQueueing(zapPacketBuffer* pb)=0;
  virtual void StopQueueing(zapPacketBuffer* pb)=0;

  // zapIMediaSink handlers:
  virtual nsresult ConnectSource(zapPacketBuffer* pb, zapIMediaSource* source,
                                 const nsACString & connection_id);
  virtual nsresult DisconnectSource(zapPacketBuffer* pb, zapIMediaSource* source,
                                    const nsACString& connection_id);
  virtual nsresult ProcessFrame(zapPacketBuffer* pb, zapIMediaFrame* frame);
#ifdef DEBUG
  virtual const char* GetName()=0;
#endif
  
protected:
  void ChangeState(zapPacketBuffer* pb, zapPacketBufferSourceState* state);
};

////////////////////////////////////////////////////////////////////////
// zapPacketBufferSource_STOP_IDLE

class zapPacketBufferSource_STOP_IDLE : public zapPacketBufferSourceState
{
public:
  static zapPacketBufferSourceState* Instance();

  virtual void PacketDequeued(zapPacketBuffer* pb);
  virtual void RunQueueing(zapPacketBuffer* pb);
  virtual void StopQueueing(zapPacketBuffer* pb);

  virtual nsresult ConnectSource(zapPacketBuffer* pb, zapIMediaSource* source,
                                 const nsACString & connection_id);
  virtual nsresult DisconnectSource(zapPacketBuffer* pb, zapIMediaSource* source,
                                    const nsACString& connection_id);
#ifdef DEBUG
  virtual const char* GetName() {
    static const char* name = "STOP_IDLE";
    return name;
  }
#endif

private:
  static zapPacketBufferSourceState* mInstance;
};

////////////////////////////////////////////////////////////////////////
// zapPacketBufferSource_STOP_WAITING

class zapPacketBufferSource_STOP_WAITING : public zapPacketBufferSourceState
{
public:
  static zapPacketBufferSourceState* Instance();

  virtual void PacketDequeued(zapPacketBuffer* pb);
  virtual void RunQueueing(zapPacketBuffer* pb);
  virtual void StopQueueing(zapPacketBuffer* pb);

  virtual nsresult DisconnectSource(zapPacketBuffer* pb, zapIMediaSource* source,
                                    const nsACString& connection_id);
  virtual nsresult ProcessFrame(zapPacketBuffer* pb, zapIMediaFrame* frame);
#ifdef DEBUG
  virtual const char* GetName() {
    static const char* name = "STOP_WAITING";
    return name;
  }
#endif

private:
  static zapPacketBufferSourceState* mInstance;
};

////////////////////////////////////////////////////////////////////////
// zapPacketBufferSource_RUN_WAITING

class zapPacketBufferSource_RUN_WAITING : public zapPacketBufferSourceState
{
public:
  static zapPacketBufferSourceState* Instance();

  virtual void PacketDequeued(zapPacketBuffer* pb);
  virtual void RunQueueing(zapPacketBuffer* pb);
  virtual void StopQueueing(zapPacketBuffer* pb);

  virtual nsresult DisconnectSource(zapPacketBuffer* pb, zapIMediaSource* source,
                                    const nsACString& connection_id);
  virtual nsresult ProcessFrame(zapPacketBuffer* pb, zapIMediaFrame* frame);
#ifdef DEBUG
  virtual const char* GetName() {
    static const char* name = "RUN_WAITING";
    return name;
  }
#endif

private:
  static zapPacketBufferSourceState* mInstance;
};

////////////////////////////////////////////////////////////////////////
// zapPacketBufferSource_RUN_IDLE

class zapPacketBufferSource_RUN_IDLE : public zapPacketBufferSourceState
{
public:
  static zapPacketBufferSourceState* Instance();

  virtual void PacketDequeued(zapPacketBuffer* pb);
  virtual void RunQueueing(zapPacketBuffer* pb);
  virtual void StopQueueing(zapPacketBuffer* pb);

  virtual nsresult ConnectSource(zapPacketBuffer* pb, zapIMediaSource* source,
                                 const nsACString & connection_id);
  virtual nsresult DisconnectSource(zapPacketBuffer* pb, zapIMediaSource* source,
                                    const nsACString& connection_id);
#ifdef DEBUG
  virtual const char* GetName() {
    static const char* name = "RUN_IDLE";
    return name;
  }
#endif

private:
  static zapPacketBufferSourceState* mInstance;
};


////////////////////////////////////////////////////////////////////////
// zapPacketBufferSinkState: baseclass for zapPacketBuffer sink states

class zapPacketBufferSinkState
{
public:
  virtual void PacketQueued(zapPacketBuffer* pb)=0;

  // zapIMediaSource handlers:
  virtual nsresult ConnectSink(zapPacketBuffer* pb, zapIMediaSink* sink,
                               const nsACString& connection_id);
  virtual nsresult DisconnectSink(zapPacketBuffer* pb, zapIMediaSink *sink,
                                  const nsACString & connection_id);
  virtual nsresult RequestFrame(zapPacketBuffer* pb);  
#ifdef DEBUG
  virtual const char* GetName()=0;
#endif
  
protected:
  void ChangeState(zapPacketBuffer* pb, zapPacketBufferSinkState* state);
};

////////////////////////////////////////////////////////////////////////
// zapPacketBufferSink_WAITING_PREFILLING

class zapPacketBufferSink_WAITING_PREFILLING : public zapPacketBufferSinkState
{
public:
  static zapPacketBufferSinkState* Instance();

  virtual void PacketQueued(zapPacketBuffer* pb);

  // zapIMediaSource handlers:
  virtual nsresult DisconnectSink(zapPacketBuffer* pb, zapIMediaSink *sink,
                                  const nsACString & connection_id);
#ifdef DEBUG
  virtual const char* GetName() {
    static const char* name = "WAITING_PREFILLING";
    return name;
  }
#endif

private:
  static zapPacketBufferSinkState* mInstance;
};

////////////////////////////////////////////////////////////////////////
// zapPacketBufferSink_WAITING

class zapPacketBufferSink_WAITING : public zapPacketBufferSinkState
{
public:
  static zapPacketBufferSinkState* Instance();

  virtual void PacketQueued(zapPacketBuffer* pb);

  // zapIMediaSource handlers:
  virtual nsresult DisconnectSink(zapPacketBuffer* pb, zapIMediaSink *sink,
                                  const nsACString & connection_id);
#ifdef DEBUG
  virtual const char* GetName() {
    static const char* name = "WAITING";
    return name;
  }
#endif

private:
  static zapPacketBufferSinkState* mInstance;
};

////////////////////////////////////////////////////////////////////////
// zapPacketBufferSink_IDLE

class zapPacketBufferSink_IDLE : public zapPacketBufferSinkState
{
public:
  static zapPacketBufferSinkState* Instance();

  virtual void PacketQueued(zapPacketBuffer* pb);

  // zapIMediaSource handlers:
  virtual nsresult DisconnectSink(zapPacketBuffer* pb, zapIMediaSink *sink,
                                  const nsACString & connection_id);
  virtual nsresult RequestFrame(zapPacketBuffer* pb);  
#ifdef DEBUG
  virtual const char* GetName() {
    static const char* name = "IDLE";
    return name;
  }
#endif

private:
  static zapPacketBufferSinkState* mInstance;
};

////////////////////////////////////////////////////////////////////////
// zapPacketBufferSink_IDLE_PREFILLING

class zapPacketBufferSink_IDLE_PREFILLING : public zapPacketBufferSinkState
{
public:
  static zapPacketBufferSinkState* Instance();

  virtual void PacketQueued(zapPacketBuffer* pb);

  // zapIMediaSource handlers:
  virtual nsresult ConnectSink(zapPacketBuffer* pb, zapIMediaSink* sink,
                               const nsACString& connection_id);
  virtual nsresult DisconnectSink(zapPacketBuffer* pb, zapIMediaSink *sink,
                                  const nsACString & connection_id);
  virtual nsresult RequestFrame(zapPacketBuffer* pb);  
#ifdef DEBUG
  virtual const char* GetName() {
    static const char* name = "IDLE_PREFILLING";
    return name;
  }
#endif

private:
  static zapPacketBufferSinkState* mInstance;
};


#endif // __ZAP_PACKETBUFFER_H__
