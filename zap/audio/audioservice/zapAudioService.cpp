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

#include "zapAudioService.h"
#include "portaudio.h"
#include "stdio.h"
#include "zapAudioDevice.h"
#include "zapAudioTransport.h"

////////////////////////////////////////////////////////////////////////
// zapAudioService

zapAudioService::zapAudioService()
    : mInitialized(PR_FALSE)
{
}

zapAudioService::~zapAudioService()
{
  if (mInitialized)
    Pa_Terminate();
}

nsresult zapAudioService::Init() {
  PaError err;
  if ((err = Pa_Initialize()) != paNoError) {
#ifdef DEBUG
    printf("Failed to initialize portaudio: %s\n", Pa_GetErrorText(err));
#endif
    return NS_ERROR_FAILURE;
  }
#ifdef DEBUG
  printf("Successfully initialized %s\n", Pa_GetVersionText());
#endif
  mInitialized = PR_TRUE;
  return NS_OK;
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapAudioService)
NS_IMPL_RELEASE(zapAudioService)

NS_INTERFACE_MAP_BEGIN(zapAudioService)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(zapIAudioService)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIAudioService methods:

/* readonly attribute zapIAudioDevice defaultInputDevice; */
NS_IMETHODIMP
zapAudioService::GetDefaultInputDevice(zapIAudioDevice * *aDefaultInputDevice)
{
  PaDeviceIndex index = Pa_GetDefaultInputDevice();
  if (index == paNoDevice) {
    *aDefaultInputDevice = nsnull;
    return NS_ERROR_FAILURE;
  }

  *aDefaultInputDevice = CreateAudioDevice(index, this);
  
  return NS_OK;
}

/* readonly attribute zapIAudioDevice defaultOutputDevice; */
NS_IMETHODIMP
zapAudioService::GetDefaultOutputDevice(zapIAudioDevice * *aDefaultOutputDevice)
{
  PaDeviceIndex index = Pa_GetDefaultOutputDevice();
  if (index == paNoDevice) {
    *aDefaultOutputDevice = nsnull;
    return NS_ERROR_FAILURE;
  }

  *aDefaultOutputDevice = CreateAudioDevice(index, this);
  
  return NS_OK;
}

/* zapIAudioTransport
   openAudioTransport(in zapIAudioDevice inputDevice,
                      in unsigned long inputChannelCount,
                      in unsigned long inputSampleFormat,
                      in double suggestedInputLatency,
                      in zapIAudioDevice outputDevice,
                      in unsigned long outputChannelCount,
                      in unsigned long outputSampleFormat,
                      in double suggestedOutputLatency,
                      in double sampleRate,
                      in unsigned long framesPerBuffer,
                      in unsigned long flags); */
NS_IMETHODIMP
zapAudioService::OpenAudioTransport(zapIAudioDevice *inputDevice,
                                    PRUint32 inputChannelCount,
                                    PRUint32 inputSampleFormat,
                                    double suggestedInputLatency,
                                    zapIAudioDevice *outputDevice,
                                    PRUint32 outputChannelCount,
                                    PRUint32 outputSampleFormat,
                                    double suggestedOutputLatency,
                                    double sampleRate,
                                    PRUint32 framesPerBuffer,
                                    PRUint32 flags,
                                    zapIAudioTransport **_retval)
{
  // prepare input & output parameters:
  PaStreamParameters inputParameters, outputParameters;
  PaStreamParameters *input, *output;
  if (inputDevice) {
    // XXX potentially dangerous static cast
    inputParameters.device = NS_STATIC_CAST(zapAudioDevice*, inputDevice)->mIndex;
    inputParameters.channelCount = inputChannelCount;
    inputParameters.sampleFormat = inputSampleFormat;
    inputParameters.suggestedLatency = suggestedInputLatency;
    inputParameters.hostApiSpecificStreamInfo = nsnull;
    input = &inputParameters;
  }
  else {
    input = nsnull;
  }
  if (outputDevice) {
    // XXX potentially dangerous static cast
    outputParameters.device =
      NS_STATIC_CAST(zapAudioDevice*, outputDevice)->mIndex;
    outputParameters.channelCount = outputChannelCount;
    outputParameters.sampleFormat = outputSampleFormat;
    outputParameters.suggestedLatency = suggestedOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = nsnull;
    output = &outputParameters;
  }
  else {
    output = nsnull;
  }

  // open transport:                                        
  *_retval = CreateAudioTransport(input, output,
                                  sampleRate, framesPerBuffer,
                                  flags, this);
  
  return NS_OK;
}
