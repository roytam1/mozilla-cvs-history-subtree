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

#ifndef __ZAP_MEDIASTREAM_H__
#define __ZAP_MEDIASTREAM_H__

#include "zapIMediaStream.h"
#include "zapIAudioTransport.h"
#include "nsIRunnable.h"
#include "zapIRtpTransport.h"
#include "nsIOutputStream.h"
#include "nsIInputStream.h"
#include "nsCOMPtr.h"
//#include "iLBC_decode.h"
//#include "iLBC_encode.h"
#include "nsString.h"
#include "speex/speex.h"

class zapMediaStream : public zapIMediaStream,
                       public zapIRtpHandler
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIMEDIASTREAM
  NS_DECL_NSIRUNNABLE
  NS_DECL_ZAPIRTPHANDLER
  
  zapMediaStream();
  ~zapMediaStream();
  PRBool Init(PRInt32 localRtpPort, PRInt32 localRtcpPort);
  
private:
  nsCOMPtr<zapIAudioTransport> mAudioTransport;
  nsCOMPtr<nsIOutputStream> mOutputStream;
  nsCOMPtr<nsIInputStream> mInputStream;
  nsCOMPtr<zapIRtpTransport> mRtpTransport;
  
//  iLBC_Dec_Inst_t mDecoderInstance;
//  iLBC_Enc_Inst_t mEncoderInstance;
  SpeexBits mDecoderBits;
  void *mDecoderState;

  SpeexBits mEncoderBits;
  void *mEncoderState;
  
  nsCString mRemoteHost;
  PRUint32 mRemoteRtpPort;
  PRUint32 mRemoteRtcpPort;

  PRUint32 mLocalRtpPort;
  PRUint32 mLocalRtcpPort;
  
  PRBool mSending;
  PRBool mReceiving;
  PRBool mClosed;

  PRUint32 mLocalTimestamp;
  PRUint16 mPayloadType;
};

//----------------------------------------------------------------------
// helper to create an add-refed media stream object:
zapIMediaStream *
CreateMediaStream(PRInt32 localRtpPort, PRInt32 localRtcpPort);

#endif // __ZAP_MEDIASTREAM_H__
