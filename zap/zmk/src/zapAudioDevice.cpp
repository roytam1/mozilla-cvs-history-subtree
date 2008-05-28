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

#include "zapAudioDevice.h"
#include "stdio.h"
#include "nsStringAPI.h"

////////////////////////////////////////////////////////////////////////
// zapAudioDevice

zapAudioDevice::zapAudioDevice(PRInt32 index)
    : mDeviceInfo(nsnull),
      mDeviceID(index)
{
  // keep a hold on portaudio until we die:
  Pa_Initialize();
}

zapAudioDevice::~zapAudioDevice()
{
  Pa_Terminate();
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_THREADSAFE_ADDREF(zapAudioDevice)
NS_IMPL_THREADSAFE_RELEASE(zapAudioDevice)

NS_INTERFACE_MAP_BEGIN(zapAudioDevice)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIAudioDevice)
  NS_INTERFACE_MAP_ENTRY(zapIAudioDevice)
  NS_INTERFACE_MAP_ENTRY(zapIPortaudioDevice)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIAudioDevice methods:

/* readonly attribute ACString name; */
NS_IMETHODIMP
zapAudioDevice::GetName(nsACString & aName)
{
  aName = nsDependentCString(GetDeviceInfo()->name);
  return NS_OK;
}

/* readonly attribute unsigned long maxInputChannels; */
NS_IMETHODIMP
zapAudioDevice::GetMaxInputChannels(PRUint32 *aMaxInputChannels)
{
  *aMaxInputChannels = GetDeviceInfo()->maxInputChannels;
  return NS_OK;
}

/* readonly attribute unsigned long maxOutputChannels; */
NS_IMETHODIMP
zapAudioDevice::GetMaxOutputChannels(PRUint32 *aMaxOutputChannels)
{
  *aMaxOutputChannels = GetDeviceInfo()->maxOutputChannels;
  return NS_OK;
}

//----------------------------------------------------------------------
// zapIPortaudioDevice methods:

/* readonly attribute PRInt32 deviceID; */
NS_IMETHODIMP
zapAudioDevice::GetDeviceID(PRInt32 *aDeviceID)
{
  *aDeviceID = mDeviceID;
  return NS_OK;
}


//----------------------------------------------------------------------
// implementation helpers:

const PaDeviceInfo*
zapAudioDevice::GetDeviceInfo() {
  if (!mDeviceInfo) {
    mDeviceInfo = Pa_GetDeviceInfo(mDeviceID);
    NS_ASSERTION(mDeviceInfo, "uh-oh. no device info");
  }
  return mDeviceInfo;
}
