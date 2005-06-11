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

#ifndef __ZAP_AUDIOTRANSPORT_H__
#define __ZAP_AUDIOTRANSPORT_H__

#include "zapIAudioTransport.h"
#include "zapAudioService.h"
#include "nsAutoPtr.h"
#include "portaudio.h"
#include "stdio.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"

class zapAudioTransport;

////////////////////////////////////////////////////////////////////////
// class zapAudioOutputStream

class zapAudioOutputStream : public nsIOutputStream
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIOUTPUTSTREAM

  zapAudioOutputStream(zapAudioTransport *transport);
  ~zapAudioOutputStream();

private:
  zapAudioTransport *mTransport;
  nsrefcnt mWriterRefCnt;
};

////////////////////////////////////////////////////////////////////////
// class zapAudioInputStream

class zapAudioInputStream : public nsIInputStream
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIINPUTSTREAM

  zapAudioInputStream(zapAudioTransport *transport);
  ~zapAudioInputStream();

private:
  zapAudioTransport *mTransport;
  nsrefcnt mReaderRefCnt;
};

////////////////////////////////////////////////////////////////////////
// class zapAudioTransport

class zapAudioTransport : public zapIAudioTransport
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_ZAPIAUDIOTRANSPORT
    
  zapAudioTransport(zapAudioService* service);
  ~zapAudioTransport();

  PaError Init(const PaStreamParameters *inputParameters,
               const PaStreamParameters *outputParameters,
               double sampleRate,
               unsigned long framesPerBuffer,
               PaStreamFlags streamFlags);
  
private:
  friend class zapAudioOutputStream;
  friend class zapAudioInputStream;
  
  zapAudioOutputStream mOutput;
  zapAudioInputStream mInput;
  
  PaStream *mPortAudioStream;
  
  PRUint32 mBytesPerOutputFrame;
  PRUint32 mBytesPerInputFrame;
  
  // strong reference to keep alive audio service:
  nsRefPtr<zapAudioService> mService;

};

//----------------------------------------------------------------------
// helper to create an add-refed audio transport object:
inline zapIAudioTransport *
CreateAudioTransport(const PaStreamParameters *inputParameters,
                    const PaStreamParameters *outputParameters,
                    double sampleRate,
                    unsigned long framesPerBuffer,
                    PaStreamFlags streamFlags,
                    zapAudioService* service) {
  zapAudioTransport* transport = new zapAudioTransport(service);
  if (!transport) return nsnull;
  NS_ADDREF(transport);
  
  PaError err;
  if ((err = transport->Init(inputParameters, outputParameters, sampleRate,
                            framesPerBuffer, streamFlags)) != paNoError) {
#ifdef DEBUG
    printf("Error initializing audio transport: %s\n", Pa_GetErrorText(err));
#endif
    NS_RELEASE(transport);
    return nsnull;
  }
  return transport;
}

#endif // __ZAP_AUDIOTRANSPORT_H__
