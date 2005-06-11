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

#include "zapMediaStream.h"
#include "nsServiceManagerUtils.h"
#include "stdio.h"
#include "zapRtpPacket.h"
#include "zapRtpTransport.h"
#include "zapIMediaService.h"
#include "zapIRtpTransport.h"
#include "zapIAudioService.h"
#include "zapIAudioDevice.h"
#include "nsIEventQueueService.h"
#include "nsIEventQueue.h"
#include "nsIThread.h"

////////////////////////////////////////////////////////////////////////
// zapMediaStream implementation

zapMediaStream::zapMediaStream()
    : mRemoteRtpPort(0),
      mRemoteRtcpPort(0),
      mLocalRtpPort(0),
      mLocalRtcpPort(0),
      mSending(PR_FALSE),
      mReceiving(PR_FALSE),
      mClosed(PR_FALSE)
{
}

zapMediaStream::~zapMediaStream()
{
#ifdef DEBUG
  printf("~zapMediaStream\n");
#endif
}

PRBool
zapMediaStream::Init(PRInt32 localRtpPort, PRInt32 localRtcpPort)
{
  mLocalRtpPort = localRtpPort;
  mLocalRtcpPort = localRtcpPort;
  return PR_TRUE;
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_THREADSAFE_ADDREF(zapMediaStream)
NS_IMPL_THREADSAFE_RELEASE(zapMediaStream)

NS_INTERFACE_MAP_BEGIN(zapMediaStream)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaStream)
  NS_INTERFACE_MAP_ENTRY(zapIMediaStream)
  NS_INTERFACE_MAP_ENTRY(nsIRunnable)
  NS_INTERFACE_MAP_ENTRY(zapIRtpHandler)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapMediaStream methods:

/* void startReceive (); */
NS_IMETHODIMP
zapMediaStream::StartReceive()
{
  mReceiving = PR_TRUE;
  return NS_OK;
}

/* void stopReceive (); */
NS_IMETHODIMP
zapMediaStream::StopReceive()
{
  mReceiving = PR_FALSE;
  return NS_OK;
}

/* void startSend (in ACString remoteHost, in unsigned long remoteRtpPort, in unsigned long remoteRtcpPort, in unsigned short payloadFormat); */
NS_IMETHODIMP
zapMediaStream::StartSend(const nsACString &remoteHost, PRUint32 remoteRtpPort, PRUint32 remoteRtcpPort, PRUint16 payloadFormat)
{
  mLocalTimestamp = 0;
  mRemoteHost = remoteHost;
  mRemoteRtpPort = remoteRtpPort;
  mRemoteRtcpPort = remoteRtcpPort;
  mRtpTransport->SetRemoteEndpoint(remoteHost, remoteRtpPort, remoteRtcpPort);
  mPayloadType = payloadFormat;
  mSending = PR_TRUE;
  return NS_OK;
}

/* void stopSend (); */
NS_IMETHODIMP
zapMediaStream::StopSend()
{
  mSending = PR_FALSE;
  return NS_OK;
}

/* void close (); */
NS_IMETHODIMP
zapMediaStream::Close()
{
  StopSend();
  StopReceive();
  mClosed = PR_TRUE;
  return NS_OK;
}

//----------------------------------------------------------------------
// nsIRunnable methods:

/* void run (); */
NS_IMETHODIMP
zapMediaStream::Run()
{
  // set up an event queue for our thread:
  nsCOMPtr<nsIEventQueueService> eventQService = do_GetService(NS_EVENTQUEUESERVICE_CONTRACTID);
  NS_ENSURE_TRUE(eventQService, NS_ERROR_FAILURE);
  eventQService->CreateMonitoredThreadEventQueue();

  // initialize decoder instance:
  //initDecode(&mDecoderInstance, 30, 1);

  speex_bits_init(&mDecoderBits);
  mDecoderState = speex_decoder_init(&speex_nb_mode);
  
  // initialize encoder instance:
  //initEncode(&mEncoderInstance, 30);

  speex_bits_init(&mEncoderBits);
  mEncoderState = speex_encoder_init(&speex_nb_mode);
  
  // create an audio transport:
  nsCOMPtr<zapIAudioService> audioService = do_GetService(ZAP_AUDIOSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(audioService, NS_ERROR_FAILURE);
  
  nsCOMPtr<zapIAudioDevice> outputDevice;
  audioService->GetDefaultOutputDevice(getter_AddRefs(outputDevice));
  NS_ENSURE_TRUE(outputDevice, NS_ERROR_FAILURE);
  nsCOMPtr<zapIAudioDevice> inputDevice;
  audioService->GetDefaultInputDevice(getter_AddRefs(inputDevice));
  NS_ENSURE_TRUE(inputDevice, NS_ERROR_FAILURE);
  
  audioService->OpenAudioTransport(inputDevice,
                                   1,
                                   zapIAudioService::sampleFormat_Int16,
                                   0.2,
                                   outputDevice,
                                   1,
                                   zapIAudioService::sampleFormat_Int16,
                                   0.4, // suggested latency
                                   8000, // sample rate
                                   240, // frames/buffer
                                   0,
                                   getter_AddRefs(mAudioTransport));
  if (!mAudioTransport) return NS_ERROR_FAILURE;

  // open the audio output stream:
  mAudioTransport->OpenOutputStream(getter_AddRefs(mOutputStream));
  NS_ENSURE_TRUE(mOutputStream, NS_ERROR_FAILURE);

  // open the audio input stream:
  mAudioTransport->OpenInputStream(getter_AddRefs(mInputStream));
  NS_ENSURE_TRUE(mInputStream, NS_ERROR_FAILURE);
  
  // start the transport, ready or not:
  mAudioTransport->Start();
  
  // create an rtp transport:
  mRtpTransport = dont_AddRef(CreateRtpTransport());
  NS_ENSURE_TRUE(mRtpTransport, NS_ERROR_FAILURE);
  if (NS_FAILED(mRtpTransport->Open(mLocalRtpPort, mLocalRtcpPort)))
    return NS_ERROR_FAILURE;
  
  // register ourselves as listener:
  mRtpTransport->SetRtpHandler(this);

  // get event queue for pumping events:
  nsCOMPtr<nsIEventQueue> eventQ;
  eventQService->GetSpecialEventQueue(nsIEventQueueService::CURRENT_THREAD_EVENT_QUEUE, getter_AddRefs(eventQ));
  NS_ENSURE_TRUE(eventQ, NS_ERROR_FAILURE);

  nsCOMPtr<nsIThread> thread;
    
  nsIThread::GetCurrent(getter_AddRefs(thread));
  
  while(!mClosed) {

    // send audio data:
    if (mSending) {
      PRUint32 count;
      mInputStream->Available(&count);
      if (count > 0) {
//        PRInt16 buf[240];
//        mInputStream->Read((char*)buf, 240*2, &count);
//         NS_ASSERTION(count == 480, "uh-oh, audio read mismatch!");
//         // convert data to float:
//         float block[240];
//         for (int i=0; i<240; ++i)
//           block[i] = (float)buf[i];
//         // do the actual encoding:
//         nsCString data;
//         data.SetLength(50);
//         iLBC_encode((unsigned char*)data.BeginWriting(), block, &mEncoderInstance);
//         NS_ASSERTION(mEncoderInstance.no_of_bytes == 50, "uh-oh, encoded bytes mismatch");
        
//         // send:
//         mRtpTransport->Send(data, mPayloadType, mLocalTimestamp);
//         mLocalTimestamp += 240;
        PRInt16 buf[160];
        mInputStream->Read((char*)buf, 160*sizeof(PRInt16), &count);
        NS_ASSERTION(count == 160*sizeof(PRInt16), "uh-oh, audio read mismatch!");
        // convert data to float:
        float block[160];
        for (int i=0; i<160; ++i)
          block[i] = (float)buf[i];
        
        // flush old bits:
        speex_bits_reset(&mEncoderBits);

        // encode frame:
        speex_encode(mEncoderState, block, &mEncoderBits);
        
        nsCString data;
        data.SetLength(200);
        int nBytes = speex_bits_write(&mEncoderBits, (char*)data.BeginWriting(), 200);
        data.SetLength(nBytes);

        // send:
        mRtpTransport->Send(data, mPayloadType, mLocalTimestamp);
        mLocalTimestamp += 160;
      }
    }
    
    // handle events:
    PRBool avail;
    eventQ->EventAvailable(avail);
    if (avail) {
      PLEvent *ev;
      //eventQ->WaitForEvent(&ev);
      eventQ->GetEvent(&ev);
      eventQ->HandleEvent(ev);
      continue; // don't sleep
    }

    // poll every millisecond
    // XXX insane
    thread->Sleep(1);   
   }

  mRtpTransport->Close();
  speex_decoder_destroy(mDecoderState);
  speex_bits_destroy(&mDecoderBits);
  
  return NS_OK;
}

//----------------------------------------------------------------------
// zapIRtpHandler methods:

/* void handleRtpPacket (in zapIRtpPacket packet); */
NS_IMETHODIMP
zapMediaStream::HandleRtpPacket(zapIRtpPacket *packet)
{
  if (!mReceiving) return NS_OK;
  
  nsCString data;
  packet->GetPayload(data);

  float output_f[160]; // 20ms 8kHz
  PRUint16 output_16[160];
  speex_bits_read_from(&mDecoderBits, (char *)data.BeginReading(), data.Length());
  speex_decode(mDecoderState, &mDecoderBits, output_f);
  for (int i=0; i<160; ++i) 
    output_16[i] = output_f[i];

  PRUint32 written;
  mOutputStream->Write((char *)output_16, 160*sizeof(PRUint16), &written);
  
//   // decode packet
//   float decblock[BLOCKL_MAX];
//   iLBC_decode(decblock, (unsigned char*)data.BeginReading(), &mDecoderInstance, 1);
//   PRUint16 frame[BLOCKL_MAX];
//   for (int i=0; i<mDecoderInstance.blockl; ++i) {
//     float dtmp = decblock[i];
//     if (dtmp < MIN_SAMPLE)
//       dtmp = MIN_SAMPLE;
//     else if (dtmp > MAX_SAMPLE)
//       dtmp = MAX_SAMPLE;
//     frame[i] = (PRUint16) dtmp;
//   }
  
//   // write to audio output
//   PRUint32 written;
//   mOutputStream->Write((char *)frame, mDecoderInstance.blockl*2, &written);
  return NS_OK;
}


//----------------------------------------------------------------------

zapIMediaStream *
CreateMediaStream(PRInt32 localRtpPort, PRInt32 localRtcpPort)
{
  zapMediaStream* stream = new zapMediaStream();
  if (!stream) return nsnull;
  NS_ADDREF(stream);

  if (!stream->Init(localRtpPort, localRtcpPort)) {
    NS_RELEASE(stream);
    return nsnull;
  }
  return stream;
}
