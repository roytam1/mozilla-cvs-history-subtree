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

// zapAudioTransport is modelled after nsSocketTransport, in particular:
// - the reference counting scheme

#include "zapAudioTransport.h"
#include "stdio.h"
#include "nsString.h"

// helper to convert a portaudio error to an nsresult:
nsresult pa_to_nsresult(PaError err)
{
  if (err == paNoError)
    return NS_OK;
#ifdef DEBUG
  printf("PortAudio Error: %s\n", Pa_GetErrorText(err));
#endif
  return NS_ERROR_FAILURE;
}
  
////////////////////////////////////////////////////////////////////////
// zapAudioOutputStream implementation

zapAudioOutputStream::zapAudioOutputStream(zapAudioTransport *transport)
    : mTransport(transport),
      mWriterRefCnt(0)
{
}

zapAudioOutputStream::~zapAudioOutputStream()
{
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMETHODIMP_(nsrefcnt)
zapAudioOutputStream::AddRef()
{
  PR_AtomicIncrement((PRInt32*)&mWriterRefCnt);
  return mTransport->AddRef();
}

NS_IMETHODIMP_(nsrefcnt)
zapAudioOutputStream::Release()
{
  if (PR_AtomicDecrement((PRInt32*)&mWriterRefCnt) == 0)
    Close();
  return mTransport->Release();
}

NS_INTERFACE_MAP_BEGIN(zapAudioOutputStream)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIOutputStream)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// nsIOutputStream methods:

/* void close (); */
NS_IMETHODIMP
zapAudioOutputStream::Close()
{
//  NS_WARNING("Can't close zapAudioOutputStream. Close parent zapAudioTransport instead!");
  return NS_ERROR_FAILURE;
}

/* void flush (); */
NS_IMETHODIMP
zapAudioOutputStream::Flush()
{
  // n/a
  return NS_OK;
}

/* unsigned long write (in string aBuf, in unsigned long aCount); */
NS_IMETHODIMP
zapAudioOutputStream::Write(const char *aBuf, PRUint32 aCount, PRUint32 *_retval)
{
  PaError err = Pa_WriteStream(mTransport->mPortAudioStream, aBuf, aCount/mTransport->mBytesPerOutputFrame);
  if (err != paNoError && err != paOutputUnderflowed) {
#ifdef DEBUG
    printf("Write error: %s\n", Pa_GetErrorText(err));
#endif
    *_retval = 0;
    return NS_ERROR_FAILURE;
  }
  *_retval = aCount;
  return NS_OK;
}

/* unsigned long writeFrom (in nsIInputStream aFromStream, in unsigned long aCount); */
NS_IMETHODIMP
zapAudioOutputStream::WriteFrom(nsIInputStream *aFromStream, PRUint32 aCount, PRUint32 *_retval)
{
  NS_NOTREACHED("WriteFrom");
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] unsigned long writeSegments (in nsReadSegmentFun aReader, in voidPtr aClosure, in unsigned long aCount); */
NS_IMETHODIMP
zapAudioOutputStream::WriteSegments(nsReadSegmentFun aReader, void * aClosure, PRUint32 aCount, PRUint32 *_retval)
{
  NS_NOTREACHED("WriteSegments");
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* boolean isNonBlocking (); */
NS_IMETHODIMP
zapAudioOutputStream::IsNonBlocking(PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

////////////////////////////////////////////////////////////////////////
// zapAudioInputStream implementation

zapAudioInputStream::zapAudioInputStream(zapAudioTransport *transport)
    : mTransport(transport),
      mReaderRefCnt(0)
{
}

zapAudioInputStream::~zapAudioInputStream()
{
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMETHODIMP_(nsrefcnt)
zapAudioInputStream::AddRef()
{
  PR_AtomicIncrement((PRInt32*)&mReaderRefCnt);
  return mTransport->AddRef();
}

NS_IMETHODIMP_(nsrefcnt)
zapAudioInputStream::Release()
{
  if (PR_AtomicDecrement((PRInt32*)&mReaderRefCnt) == 0)
    Close();
  return mTransport->Release();
}

NS_INTERFACE_MAP_BEGIN(zapAudioInputStream)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIInputStream)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// nsIInputStream methods:

/* void close (); */
NS_IMETHODIMP
zapAudioInputStream::Close()
{
  //NS_WARNING("Can't close zapAudioInputStream. Close parent zapAudioTransport instead!");
  return NS_ERROR_FAILURE;
}

/* unsigned long available (); */
NS_IMETHODIMP
zapAudioInputStream::Available(PRUint32 *_retval)
{
  PRInt32 val = Pa_GetStreamReadAvailable(mTransport->mPortAudioStream);
  if (val<0) {
    *_retval = 0;
    return NS_ERROR_FAILURE;
  }
  *_retval = val;
  return NS_OK;
}

/* [noscript] unsigned long read (in charPtr aBuf, in unsigned long aCount); */
NS_IMETHODIMP
zapAudioInputStream::Read(char * aBuf, PRUint32 aCount, PRUint32 *_retval)
{
  PaError err = Pa_ReadStream(mTransport->mPortAudioStream, aBuf, aCount/mTransport->mBytesPerInputFrame);
  if (err != paNoError && err != paInputOverflowed) {
#ifdef DEBUG
    printf("Read error: %s\n", Pa_GetErrorText(err));
#endif
    *_retval = 0;
    return NS_ERROR_FAILURE;
  }
  *_retval = aCount;
  return NS_OK;
}

/* [noscript] unsigned long readSegments (in nsWriteSegmentFun aWriter, in voidPtr aClosure, in unsigned long aCount); */
NS_IMETHODIMP
zapAudioInputStream::ReadSegments(nsWriteSegmentFun aWriter, void * aClosure, PRUint32 aCount, PRUint32 *_retval)
{
  NS_NOTREACHED("ReadSegments");
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* boolean isNonBlocking (); */
NS_IMETHODIMP
zapAudioInputStream::IsNonBlocking(PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

  
////////////////////////////////////////////////////////////////////////
// zapAudioTransport implementation

zapAudioTransport::zapAudioTransport(zapAudioService* service)
    : mService(service),
      mPortAudioStream(nsnull),
      mOutput(this),
      mInput(this),
      mBytesPerOutputFrame(0),
      mBytesPerInputFrame(0)
{
}

zapAudioTransport::~zapAudioTransport()
{
  if (Pa_IsStreamStopped(mPortAudioStream) != 1)
    Close();
}

PaError
zapAudioTransport::Init(const PaStreamParameters *inputParameters,
                        const PaStreamParameters *outputParameters,
                        double sampleRate,
                        unsigned long framesPerBuffer,
                        PaStreamFlags streamFlags)
{
  PaError err = Pa_OpenStream(&mPortAudioStream,
                              inputParameters,
                              outputParameters,
                              sampleRate,
                              framesPerBuffer,
                              streamFlags,
                              nsnull, // we use the blocking api, so 2*nsnull
                              nsnull);
  if (err == paNoError) {
    if (inputParameters)
      mBytesPerInputFrame =
        inputParameters->channelCount*Pa_GetSampleSize(inputParameters->sampleFormat);
    else
      mBytesPerInputFrame = 0;
    
    if (outputParameters)
      mBytesPerOutputFrame =
        outputParameters->channelCount*Pa_GetSampleSize(outputParameters->sampleFormat);
    else
      mBytesPerOutputFrame = 0;
  }

  return err;
}


//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapAudioTransport)
NS_IMPL_RELEASE(zapAudioTransport)

NS_INTERFACE_MAP_BEGIN(zapAudioTransport)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(zapIAudioTransport)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIAudioTransport methods:

/* nsIInputStream openInputStream (); */
NS_IMETHODIMP
zapAudioTransport::OpenInputStream(nsIInputStream **_retval)
{
  *_retval = &mInput;
  NS_ADDREF(*_retval);
  return NS_OK;
}

/* nsIOutputStream openOutputStream (); */
NS_IMETHODIMP
zapAudioTransport::OpenOutputStream(nsIOutputStream **_retval)
{
  *_retval = &mOutput;
  NS_ADDREF(*_retval);
  
  return NS_OK;
}

/* void start (); */
NS_IMETHODIMP zapAudioTransport::Start()
{
  return pa_to_nsresult(Pa_StartStream(mPortAudioStream));
}

/* void stop (); */
NS_IMETHODIMP zapAudioTransport::Stop()
{
  return pa_to_nsresult(Pa_StopStream(mPortAudioStream));
}

/* void abort (); */
NS_IMETHODIMP zapAudioTransport::Abort()
{
  return pa_to_nsresult(Pa_AbortStream(mPortAudioStream));
}

/* void close (); */
NS_IMETHODIMP
zapAudioTransport::Close()
{
  return pa_to_nsresult(Pa_CloseStream(mPortAudioStream));
}

/* boolean isActive (); */
NS_IMETHODIMP zapAudioTransport::IsActive(PRBool *_retval)
{
  *_retval = (Pa_IsStreamActive(mPortAudioStream) == 1);
  return NS_OK;
}

//----------------------------------------------------------------------
// implementation helpers:


