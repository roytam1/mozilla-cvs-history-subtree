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

#include "nsIGenericFactory.h"
#include "zapMediaGraph.h"
#include "zapAudioDeviceManager.h"
#include "zapAudioIn.h"
#include "zapAudioOut.h"
#include "zapPacketBuffer.h"
#include "zapSpeexEncoder.h"
#include "zapSpeexDecoder.h"
#include "zapSpeexRTPPacketizer.h"
#include "zapSpeexRTPDepacketizer.h"
#include "zapUDPSocket.h"
#include "zapUDPSocketPair.h"
#include "zapRTPTransmitter.h"
#include "zapRTPReceiver.h"
#include "zapRTPSession.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(zapAudioDeviceManager)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapAudioIn)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapAudioOut)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapPacketBuffer)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapSpeexEncoder)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapSpeexDecoder)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapSpeexRTPPacketizer)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapSpeexRTPDepacketizer)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapUDPSocket)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapUDPSocketPair)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapRTPTransmitter)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapRTPReceiver)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapRTPSession)
  
static const nsModuleComponentInfo gComponents[] =
{
  { "Mozilla ZMK Media Graph",
    ZAP_MEDIAGRAPH_CID,
    ZAP_MEDIAGRAPH_CONTRACTID,
    ConstructMediaGraph
  },
  { "Mozilla ZMK Audio Device Manager",
    ZAP_AUDIODEVICEMANAGER_CID,
    ZAP_AUDIODEVICEMANAGER_CONTRACTID,
    zapAudioDeviceManagerConstructor
  },
  { "Mozilla ZMK Audio In",
    ZAP_AUDIOIN_CID,
    ZAP_AUDIOIN_CONTRACTID,
    zapAudioInConstructor
  },
  { "Mozilla ZMK Audio Out",
    ZAP_AUDIOOUT_CID,
    ZAP_AUDIOOUT_CONTRACTID,
    zapAudioOutConstructor
  },
  { "Mozilla ZMK Media Packet Buffer",
    ZAP_PACKETBUFFER_CID,
    ZAP_PACKETBUFFER_CONTRACTID,
    zapPacketBufferConstructor
  },
  { "Mozilla ZMK Speex Encoder",
    ZAP_SPEEXENCODER_CID,
    ZAP_SPEEXENCODER_CONTRACTID,
    zapSpeexEncoderConstructor
  },
  { "Mozilla ZMK Speex Decoder",
    ZAP_SPEEXDECODER_CID,
    ZAP_SPEEXDECODER_CONTRACTID,
    zapSpeexDecoderConstructor
  },
  { "Mozilla ZMK Speex RTP Packetizer",
    ZAP_SPEEXRTPPACKETIZER_CID,
    ZAP_SPEEXRTPPACKETIZER_CONTRACTID,
    zapSpeexRTPPacketizerConstructor
  },
  { "Mozilla ZMK Speex RTP Depacketizer",
    ZAP_SPEEXRTPDEPACKETIZER_CID,
    ZAP_SPEEXRTPDEPACKETIZER_CONTRACTID,
    zapSpeexRTPDepacketizerConstructor
  },
  { "Mozilla ZMK UDP Socket",
    ZAP_UDPSOCKET_CID,
    ZAP_UDPSOCKET_CONTRACTID,
    zapUDPSocketConstructor
  },
  { "Mozilla ZMK UDP Socket Pair",
    ZAP_UDPSOCKETPAIR_CID,
    ZAP_UDPSOCKETPAIR_CONTRACTID,
    zapUDPSocketPairConstructor
  },
  { "Mozilla ZMK RTP Transmitter",
    ZAP_RTPTRANSMITTER_CID,
    ZAP_RTPTRANSMITTER_CONTRACTID,
    zapRTPTransmitterConstructor
  },
  { "Mozilla ZMK RTP Receiver",
    ZAP_RTPRECEIVER_CID,
    ZAP_RTPRECEIVER_CONTRACTID,
    zapRTPReceiverConstructor
  },
  { "Mozilla ZMK RTP Session",
    ZAP_RTPSESSION_CID,
    ZAP_RTPSESSION_CONTRACTID,
    zapRTPSessionConstructor
  }
};

NS_IMPL_NSGETMODULE(zapMediaService, gComponents)
  
