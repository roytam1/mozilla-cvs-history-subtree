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

#ifndef __ZAP_SPEEXENCODER_H__
#define __ZAP_SPEEXENCODER_H__

#include "zapFilterNode.h"
#include "speex/speex.h"
#include "nsIWritablePropertyBag2.h"
#include "zapISpeexEncoder.h"
#include "nsStringAPI.h"

////////////////////////////////////////////////////////////////////////
// zapSpeexEncoder

// {68A90E36-5D20-4F90-8CA1-7ECC41953F48}
#define ZAP_SPEEXENCODER_CID                             \
  { 0x68a90e36, 0x5d20, 0x4f90, { 0x8c, 0xa1, 0x7e, 0xcc, 0x41, 0x95, 0x3f, 0x48 } }

#define ZAP_SPEEXENCODER_CONTRACTID ZAP_MEDIANODE_CONTRACTID_PREFIX "speex-encoder"

class zapSpeexEncoder : public zapFilterNode,
                        public zapISpeexEncoder
{
public:
  zapSpeexEncoder();
  ~zapSpeexEncoder();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_ZAPISPEEXENCODER
  
  NS_IMETHOD InsertedIntoContainer(zapIMediaNodeContainer *container,
                                   nsIPropertyBag2 *node_pars);
  NS_IMETHOD RemovedFromContainer();
  virtual nsresult ValidateNewStream(nsIPropertyBag2* streamInfo);
  virtual nsresult Filter(zapIMediaFrame* input, zapIMediaFrame** output);

private:
  PRUint32 mSampleRate; // encoder sample rate
  SpeexBits mEncoderBits;
  void* mEncoderState;
  PRUint32 mInputBufferLength; // expected length of input buffers (bytes)
  nsCOMPtr<nsIWritablePropertyBag2> mStreamInfo;
};

#endif // __ZAP_SPEEXENCODER_H__
