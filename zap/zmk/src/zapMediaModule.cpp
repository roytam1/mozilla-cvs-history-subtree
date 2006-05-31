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
#include "zapAudioMixer.h"
#include "zapPacketBuffer.h"
#include "zapSpeexEncoder.h"
#include "zapSpeexDecoder.h"
#include "zapSpeexRTPPacketizer.h"
#include "zapSpeexRTPDepacketizer.h"
#include "zapG711Encoder.h"
#include "zapG711Decoder.h"
#include "zapG711RTPPacketizer.h"
#include "zapG711RTPDepacketizer.h"
#include "zapUDPSocket.h"
#include "zapUDPSocketPair.h"
#include "zapRTPTransmitter.h"
#include "zapRTPReceiver.h"
#include "zapRTPSession.h"
#include "zapSplitter.h"
#include "zapRtttlPlayer.h"
#include "zapTToneToPCMConverter.h"
#include "zapDTMFGenerator.h"
#include "zapTToneGenerator.h"
#include "zapTEventToTToneConverter.h"
#include "zapPacketPump.h"
#include "zapStreamMerger.h"
#include "zapTEventRTPPacketizer.h"
#include "zapStreamSyncer.h"
#include "zapRTPDemuxer.h"
#include "zapStreamSwitch.h"
#include "zapStreamTap.h"
#include "zapSpeexAudioProcessor.h"
// #include "zapAudioIO.h"
#include "zapStunClient.h"
#include "zapStunServer.h"
#include "zapStunDemuxer.h"
#include "zapFileIn.h"
#include "zapClientSink.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(zapAudioDeviceManager)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapAudioIn)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapAudioOut)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapAudioMixer)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapPacketBuffer)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapSpeexEncoder)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapSpeexDecoder)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapSpeexRTPPacketizer)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapSpeexRTPDepacketizer)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapG711Encoder)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapG711Decoder)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapG711RTPPacketizer)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapG711RTPDepacketizer)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapUDPSocket)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapUDPSocketPair)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapRTPTransmitter)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapRTPReceiver)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapRTPSession)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapSplitter)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapRtttlPlayer)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapTToneToPCMConverter)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapDTMFGenerator)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapTToneGenerator)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapTEventToTToneConverter)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapPacketPump)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapStreamMerger)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapTEventRTPPacketizer)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapStreamSyncer)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapRTPDemuxer)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapStreamSwitch)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapStreamTap)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapSpeexAudioProcessor)
//NS_GENERIC_FACTORY_CONSTRUCTOR(zapAudioIO)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapStunClient)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapStunServer)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapStunDemuxer)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapFileIn)
NS_GENERIC_FACTORY_CONSTRUCTOR(zapClientSink)

  
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
  { "Mozilla ZMK Audio Mixer",
    ZAP_AUDIOMIXER_CID,
    ZAP_AUDIOMIXER_CONTRACTID,
    zapAudioMixerConstructor
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
  { "Mozilla ZMK G.711 Encoder",
    ZAP_G711ENCODER_CID,
    ZAP_G711ENCODER_CONTRACTID,
    zapG711EncoderConstructor
  },
  { "Mozilla ZMK G.711 Decoder",
    ZAP_G711DECODER_CID,
    ZAP_G711DECODER_CONTRACTID,
    zapG711DecoderConstructor
  },
  { "Mozilla ZMK G.711 RTP Packetizer",
    ZAP_G711RTPPACKETIZER_CID,
    ZAP_G711RTPPACKETIZER_CONTRACTID,
    zapG711RTPPacketizerConstructor
  },
  { "Mozilla ZMK G.711 RTP Depacketizer",
    ZAP_G711RTPDEPACKETIZER_CID,
    ZAP_G711RTPDEPACKETIZER_CONTRACTID,
    zapG711RTPDepacketizerConstructor
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
  },
  { "Mozilla ZMK Splitter",
    ZAP_SPLITTER_CID,
    ZAP_SPLITTER_CONTRACTID,
    zapSplitterConstructor
  },
  { "Mozilla ZMK RTTTL player",
    ZAP_RTTTLPLAYER_CID,
    ZAP_RTTTLPLAYER_CONTRACTID,
    zapRtttlPlayerConstructor
  },
  { "Mozilla ZMK audio/tone to audio/pcm converter",
    ZAP_TTONETOPCMCONVERTER_CID,
    ZAP_TTONETOPCMCONVERTER_CONTRACTID,
    zapTToneToPCMConverterConstructor
  },
  { "Mozilla ZMK DTMF Generator",
    ZAP_DTMFGENERATOR_CID,
    ZAP_DTMFGENERATOR_CONTRACTID,
    zapDTMFGeneratorConstructor
  },
  { "Mozilla ZMK Telephony Tone Generator",
    ZAP_TTONEGENERATOR_CID,
    ZAP_TTONEGENERATOR_CONTRACTID,
    zapTToneGeneratorConstructor
  },
  { "Mozilla ZMK audio/telephone-event to audio/tone converter",
    ZAP_TEVENTTOTTONECONVERTER_CID,
    ZAP_TEVENTTOTTONECONVERTER_CONTRACTID,
    zapTEventToTToneConverterConstructor
  },
  { "Mozilla ZMK packet pump",
    ZAP_PACKETPUMP_CID,
    ZAP_PACKETPUMP_CONTRACTID,
    zapPacketPumpConstructor
  },
  { "Mozilla ZMK stream merger",
    ZAP_STREAMMERGER_CID,
    ZAP_STREAMMERGER_CONTRACTID,
    zapStreamMergerConstructor
  },
  { "Mozilla ZMK TEvent RTP packetizer",
    ZAP_TEVENTRTPPACKETIZER_CID,
    ZAP_TEVENTRTPPACKETIZER_CONTRACTID,
    zapTEventRTPPacketizerConstructor
  },
  { "Mozilla ZMK stream syncer",
    ZAP_STREAMSYNCER_CID,
    ZAP_STREAMSYNCER_CONTRACTID,
    zapStreamSyncerConstructor
  },
  { "Mozilla ZMK RTP demuxer",
    ZAP_RTPDEMUXER_CID,
    ZAP_RTPDEMUXER_CONTRACTID,
    zapRTPDemuxerConstructor
  },
  { "Mozilla ZMK stream switch",
    ZAP_STREAMSWITCH_CID,
    ZAP_STREAMSWITCH_CONTRACTID,
    zapStreamSwitchConstructor
  },
  { "Mozilla ZMK stream tap",
    ZAP_STREAMTAP_CID,
    ZAP_STREAMTAP_CONTRACTID,
    zapStreamTapConstructor
  },
  { "Mozilla ZMK speex audio processor",
    ZAP_SPEEXAUDIOPROCESSOR_CID,
    ZAP_SPEEXAUDIOPROCESSOR_CONTRACTID,
    zapSpeexAudioProcessorConstructor
  },
//   { "Mozilla ZMK Audio IO",
//     ZAP_AUDIOIO_CID,
//     ZAP_AUDIOIO_CONTRACTID,
//     zapAudioIOConstructor
//   },
  { "Mozilla ZMK STUN client",
    ZAP_STUNCLIENT_CID,
    ZAP_STUNCLIENT_CONTRACTID,
    zapStunClientConstructor
  },
  { "Mozilla ZMK STUN server",
    ZAP_STUNSERVER_CID,
    ZAP_STUNSERVER_CONTRACTID,
    zapStunServerConstructor
  },
  { "Mozilla ZMK STUN demuxer",
    ZAP_STUNDEMUXER_CID,
    ZAP_STUNDEMUXER_CONTRACTID,
    zapStunDemuxerConstructor
  },
  { "Mozilla ZMK file input source",
    ZAP_FILEIN_CID,
    ZAP_FILEIN_CONTRACTID,
    zapFileInConstructor
  },
  { "Mozilla ZMK client sink",
    ZAP_CLIENTSINK_CID,
    ZAP_CLIENTSINK_CONTRACTID,
    zapClientSinkConstructor
  }
};

NS_IMPL_NSGETMODULE(zapMediaService, gComponents)
  
