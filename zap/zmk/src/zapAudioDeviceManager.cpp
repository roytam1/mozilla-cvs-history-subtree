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

#include "zapAudioDeviceManager.h"
#include "zapAudioDevice.h"
#include "portaudio.h"
#include "stdio.h"

////////////////////////////////////////////////////////////////////////
// zapAudioDeviceManager

zapAudioDeviceManager::zapAudioDeviceManager()
{
  PaError err;
  if ((err = Pa_Initialize()) != paNoError) {
#ifdef DEBUG
    printf("Failed to initialize portaudio: %s\n", Pa_GetErrorText(err));
#endif
  }
}

zapAudioDeviceManager::~zapAudioDeviceManager()
{
  Pa_Terminate();
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapAudioDeviceManager)
NS_IMPL_RELEASE(zapAudioDeviceManager)

NS_INTERFACE_MAP_BEGIN(zapAudioDeviceManager)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(zapIAudioDeviceManager)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIAudioDeviceManager methods:

/* readonly attribute zapIAudioDevice defaultInputDevice; */
NS_IMETHODIMP
zapAudioDeviceManager::GetDefaultInputDevice(zapIAudioDevice * *aDefaultInputDevice)
{
  PaDeviceID id = Pa_GetDefaultInputDeviceID();
  if (id == paNoDevice) {
    *aDefaultInputDevice = nsnull;
    return NS_ERROR_FAILURE;
  }

  *aDefaultInputDevice = CreateAudioDevice(id);
  return NS_OK;
}

/* readonly attribute zapIAudioDevice defaultOutputDevice; */
NS_IMETHODIMP
zapAudioDeviceManager::GetDefaultOutputDevice(zapIAudioDevice * *aDefaultOutputDevice)
{
  PaDeviceID id = Pa_GetDefaultOutputDeviceID();
  if (id == paNoDevice) {
    *aDefaultOutputDevice = nsnull;
    return NS_ERROR_FAILURE;
  }

  *aDefaultOutputDevice = CreateAudioDevice(id);
  return NS_OK;
}

/* readonly attribute unsigned long deviceCount; */
NS_IMETHODIMP
zapAudioDeviceManager::GetDeviceCount(PRUint32 *aDeviceCount)
{
  *aDeviceCount = Pa_CountDevices();
  return NS_OK;
}

/* zapIAudioDevice getDeviceAt (in unsigned long pos); */
NS_IMETHODIMP
zapAudioDeviceManager::GetDeviceAt(PRUint32 pos, zapIAudioDevice **_retval)
{
  if (pos < 0 || pos >= Pa_CountDevices()) {
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
  }

  *_retval = CreateAudioDevice(pos);
  return NS_OK;  
}
