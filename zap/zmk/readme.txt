Mozilla ZAP Media Kit
=====================

Built-in media frame types:
===========================

I) audio/pcm
------------

PCM audio frame.

Interfaces: zapIMediaFrame

zapIMediaFrame::streamInfo :
- ACString "type" : = "audio/pcm"
- double "sample_rate" : sample rate in Hz
- double "frame_duration" : duration of frame in s
- unsigned long "channels" : number of channels
- ACString "sample_format" : "float32_1" | "float32_32768" | "int16" | "int32" 

zapIMediaFrame::data :

"sample_format" == "float32_1" | "float32_32768" | "int32":
(max amplitude = +/-1 | +/- 32768 | +/- 2^31)

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                      sample 0 channel 0                       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                      sample 0 channel 1                       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                             ...                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                      sample 0 channel n-1                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                      sample 1 channel 0                       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                             ...                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|       sample 'sample_rate*frame_duration-1' channel n-1       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

"sample_format" == "int16":

As above but with 16bit samples.

----------------------------------------------------------------------

II) datagram
------------

Generic datagram frame for transmission over a network socket.

Interfaces: zapIMediaFrame, 
            nsIDatagram (see netwerk/base/public/nsIUDPSocket.idl)

zapIMediaFrame::streamInfo :
- ACString "type" : = "datagram"

zapIMediaFrame::data :
- same as nsIDatagram::data

----------------------------------------------------------------------

III) audio/speex
----------------

Speex encoded audio frame.

Interfaces: zapIMediaFrame

zapIMediaFrame::streamInfo :
- ACString "type" : = "audio/speex"

zapIMediaFrame::data :
- binary data as described in speex specs

----------------------------------------------------------------------

IV) rtp
-------

RTP packet.

Interfaces: zapIRTPFrame (spec of zapIMediaFrame)

zapIMediaFrame::streamInfo :
- ACString "type" : = "rtp"

zapIMediaFrame::data : 
- raw rtp packet data including header

----------------------------------------------------------------------

V) audio/pcmu
-------------

Mu-law encoded pcm audio frame.

Interfaces: zapIMediaFrame

zapIMediaFrame::streamInfo :
- ACString "type" : = "audio/pcmu"

zapIMediaFrame::data :

8 bit pcmu samples.
Framing is currently fixed to 8000Hz, 0.02s, 1 channel

----------------------------------------------------------------------

VI) audio/pcma
--------------

Alpha-law encoded pcm audio frame.

Interfaces: zapIMediaFrame

zapIMediaFrame::streamInfo :
- ACString "type" : = "audio/pcma"

zapIMediaFrame::data :

8 bit pcma samples.
Framing is currently fixed to 8000Hz, 0.02s, 1 channel

----------------------------------------------------------------------

VII) audio/tone
---------------

Telephony tone frame as described in RFC2833.

Interfaces: zapIAudioToneFrame (spec of zapIMediaFrame)

zapIMediaFrame::streamInfo :
- ACString "type" : = "audio/tone"

zapIMediaFrame::data :

- raw audio/tone payload as described in RFC2833 with implied 
  8000Hz sample rate.

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|    modulation   |T|  volume   |          duration             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|0 0 0 0|       frequency       |0 0 0 0|       frequency       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| ....                                                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|0 0 0 0|       frequency       |0 0 0 0|       frequency       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

----------------------------------------------------------------------

VIII) audio/telephone-event
---------------------------

Telephone events (e.g. DTMF digits) as defined in RFC2833.

Interfaces: zapITelephoneEventFrame (spec of zapIMediaFrame)

zapIMediaFrame::streamInfo :
- ACString "type" : = "audio/telephone-event"

zapIMediaFrame::data :

- raw audio/telephone-event payload as described in RFC2833 with 
  implied 8000Hz sample rate.

 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     event     |E|0|  volume   |          duration             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

----------------------------------------------------------------------

IX) stun
--------

A udp datagram containing a RFC3489 STUN request or response.

Interfaces: zapIMediaFrame, 
            zapIStunMessage (see zap/netutils/src/zapIStunMessage.idl),
            nsIDatagram (see netwerk/base/public/nsIUDPSocket.idl)

zapIMediaFrame::streamInfo :
- ACString "type" : = "stun"

zapIMediaFrame::data :
- same as zapIStunMessage::serialize()


======================================================================
======================================================================

Built-in node types:
====================

----------------------------------------------------------------------

1) audioin 
----------

Audio source.

Sinks: none
Sources: 1 (active)

Control interfaces: zapIAudioIn

Node parameters: 
- zapIAudioDevice "device" (default: default audio in)
- int "buffers" : number of buffer mediating between zmk and device
                  (default: 2, must be >=2)
- double "sample_rate" : sample rate in Hz (default: 8000)
- double "frame_duration" : duration of one frame in s (default: 0.02)
- unsigned long "channels" : number of channels (default: 1)
- ACString "sample_format" : "float32_1" | "float32_32768" | "int16" | "int32"
                             (default: "float32_32768")

Output stream:
audio/pcm frames with
- double "sample_rate" == corresponding node parameter
- double "frame_duration" == corresponding node parameter
- unsigned long "channels" == corresponding node parameter
- unsigned long "sample_format" == corresponding node parameter


----------------------------------------------------------------------

2) audioout
-----------

Audio sink.

Sinks: 1 (active)
Sources: none

Control interfaces: zapIAudioOut

Node parameters: 
- zapIAudioDevice "device" (default: default audio out)
- int "buffers" : number of buffer mediating between zmk and device
                  (default: 4, must be >=2)
- double "sample_rate" : sample rate in Hz (default: 8000)
- double "frame_duration" : duration of one frame in s (default: 0.02)
- unsigned long "channels" : number of channels (default: 1)
- ACString "sample_format" : "float32_1" | "float32_32768" | "int16" | "int32"
                             (default: "float32_32768")

Input stream: 
audio/pcm frames with
- double "sample_rate" == corresponding node parameter
- double "frame_duration" == corresponding node parameter
- unsigned long "channels" == corresponding node parameter
- unsigned long "sample_format" == corresponding node parameter

----------------------------------------------------------------------

3) audio-mixer
--------------

Audio mixer node, mixes n audio sinks to one audio source.

Control interfaces:
- 

Sinks: n (active)
Sources: 1 (passive)

Node parameters:
- double "sample_rate" : sample rate in Hz (default: 8000)
- double "frame_duration" : duration of one frame in s (default: 0.02)
- unsigned long "channels" : number of channels (default: 1)
- ACString "sample_format" : "float32_32768" (default: "float32_32768")

Input streams:
audio/pcm frames with
- double "sample_rate" == corresponding node parameter
- double "frame_duration" == corresponding node parameter
- unsigned long "channels" == corresponding node parameter
- unsigned long "sample_format" == "float32_32768"

Output stream:
audio/pcm frames with
- double "sample_rate" == corresponding node parameter
- double "frame_duration" == corresponding node parameter
- unsigned long "channels" == corresponding node parameter
- unsigned long "sample_format" == "float32_32768"

----------------------------------------------------------------------

4) buffer
---------

General purpose packet buffer.

Sinks: 1 (passive)
Sources: 1 (passive)

Node parameters:
- unsigned long "max_size" : maximum number of packets to buffer (10)
                             Must be >= 1.
- unsigned long "min_size" : minimum number of packets that need to be in 
                             the buffer before a packet can be taken out (1). 
                             Must be >= 1.
- unsigned long "lift_count" : whenever the size of the buffer drops to 0, 
                               lift_count packets will be pre-buffered before
                               the attached sink is allowed to take any packets
                               out of the buffer (0). A lift_count of 0 has the 
                               same effect as a lift_count of 1.
- unsigned int "drop_count" : number of packets to drop at the front of the buffer 
                              (older packets) when a new packet arrives for a 
                              buffer that has reached its maximum size (0). 
                              If this is zero, no new packet will be added to 
                              a buffer that has reached its maximum size, 
                              i.e. newly arriving packets will be dropped.

Control interfaces: zapIPacketBuffer


Input stream:
any

Output stream:
any

----------------------------------------------------------------------

5) udp-socket
-------------

A UDP socket for sending and receiving datagrams.

Control interfaces:
- zapIUDPSocket

Sinks: 1 (passive)
Sources: 1 (active)

Node parameters:
- nsIUDPSocket "socket" : An initialized udp socket (nsIUDPSocket). 
                          The socket will be closed on removal from 
                          the graph.
- unsigned short "port" : local port of udp socket (default: 0)
- unsigned short "portbase" : bind to first available port >= portbase.
- (XXX ACString "address" currently we bind to 0.0.0.0)

If both "socket" and "port" or "portbase" are given, "port" and
"portbase" will be ignored.

If both "port" and "portbase" are given, "portbase" will be ignored.

If neither "port", "portbase" or "socket" are given, the behaviour
will be as if a "port"==0 had been specified (allocate a random free
port).

Input stream:
datagram frames

Output stream:
datagram frames

----------------------------------------------------------------------

6) udp-socket-pair
------------------

A UDP socket pair (for rtp/rtcp). port_a and port_b will be chosen
such that portbase <= port_a, port_a = 2*n, and port_b = 2*n+1

Control interfaces:
- zapIUDPSocketPair

Sinks: 2
- ACString "name" == "socket-a" (passive)
- ACString "name" == "socket-b" (passive)

Sources: 2
- ACString "name" == "socket-a" (active)
- ACString "name" == "socket-b" (active)

Node parameters:
- unsigned short "portbase" : port at which to start searching for a 
                              free pair (default: 49152)

Input streams:
- as in udp-socket

Output streams:
- as in udp-socket

----------------------------------------------------------------------

7) speex-encoder
----------------

Filter for encoding audio streams into speex streams.

Control interfaces: zapISpeexEncoder

Node parameters:
- double "sample_rate" : 8000|16000|32000 (default: 8000)

Input stream:
audio/pcm frames with
- double "sample_rate" == 8000, 16000, or 32000; must match node parameter
- double "frame_duration" == 0.02
- unsigned long "channels" == 1
- unsigned long "sample_format" == "float32_32768"

Output stream:
audio/speex frames

----------------------------------------------------------------------

8) speex-decoder
----------------

Filter for decoding speex streams as encoded using speex-encoder

Control interfaces: zapISpeexDecoder

Node parameters:
-double "sample_rate" : 8000|16000|32000 (default:8000)

Input stream:
audio/speex frames

Output stream:
audio/pcm frames with
- double "sample_rate" == 8000, 16000, or 32000; matches node parameter
- double "frame_duration" == 0.02
- unsigned long "channels" == 1
- unsigned long "sample_format" == "float32_32768"

----------------------------------------------------------------------

9) speex-rtp-packetizer
-----------------------

Filter for converting speex streams into rtp streams.

Node parameters:
- unsigned short "payload_type" (default: 96)

Input stream:
audio/speex frames

Output stream:
rtp frames (SSRC and sequence number NOT set).

----------------------------------------------------------------------

10) speex-rtp-depacketizer
--------------------------

Filter for converting rtp streams into speex streams.

Input stream:
rtp frames (with assumed speex payload)

Output stream:
audio/speex frames

----------------------------------------------------------------------

11) g711-encoder
----------------

Filter for encoding audio streams into g.711 (pcmu or pcma) streams.

Control interfaces: 

Node parameters:
- ACString "type" == "audio/pcmu" | "audio/pcma" (default: "audio/pcmu")

Input stream:
audio/pcm frames with
- double "sample_rate" == 8000
- double "frame_duration" == 0.02
- unsigned long "channels" == 1
- unsigned long "sample_format" == "float32_32768"

Output frames:
audio/pcmu or audio/pcma frames; determined by node parameter

----------------------------------------------------------------------

12) g711-decoder
----------------

Filter for decoding g.711 streams as encoded using g711-encoder

Control interfaces: 

Node parameters:

Input stream:
audio/pcmu or audio/pcma frames

Output stream:
audio/pcm frames with
- double "sample_rate" == 8000
- double "frame_duration" == 0.02
- unsigned long "channels" == 1
- unsigned long "sample_format" == "float32_32768"

----------------------------------------------------------------------

13) g711-rtp-packetizer
-----------------------

Filter for converting g.711 streams into rtp streams.

Node parameters:
- unsigned short "payload_type" (default: 0)

Input stream:
audio/pcmu or audio/pcma frames 

Output stream:
rtp frames (SSRC and sequence number NOT set).

----------------------------------------------------------------------

14) g711-rtp-depacketizer
-------------------------

Filter for converting rtp streams into g.711 streams.

Node parameters:
- ACString "type" == "audio/pcmu" | "audio/pcma" (default: "audio/pcmu")

Input stream:
rtp frames with assumed g711 payload

Output stream:
audio/pcmu or audio/pcma frames (determined by node parameter)

----------------------------------------------------------------------

15) rtp-session
---------------

Maintains a unicast RTP session.
RTP-session sets the SSRC and sequence number of outbound rtp frames
and generates/processes RTCP.

Node parameters:
(XXX - unsigned long "SSRC" )

Sinks: 3 

ACString "name" == "local2remote-rtp"   : outbound rtp frames (neutral)
ACString "name" == "remote2local-rtp"  : inbound rtp frames  (neutral)
ACString "name" == "remote2local-rtcp" : inbound rtcp frames (neutral)

Sources: 3 

ACString "name" == "remote2local-rtp"   : inbound rtp frames (neutral)
ACString "name" == "local2remote-rtp"  : outbound rtp frames (neutral)
ACString "name" == "local2remote-rtcp" : outbound rtcp frames (neutral)

Input streams:
"local-rtp": rtp frames w/o ssrc or seq no.
...XXX

Output streams:
"local2remote-rtp": rtp frames with ssrc and seq number set.

----------------------------------------------------------------------

16) rtp-transmitter
-------------------

Filter for transmitting RTP packets over UDP.

Node parameters:
- ACString "address" : destination address for datagrams
- unsigned short "port" : destination port for datagrams

Input stream:
rtp frames.

Output stream:
datagram frames.

----------------------------------------------------------------------

17) rtp-receiver
----------------

Filter for receiving RTP datagrams. 

Input stream:
datagram frames

Output stream:
rtp frames


----------------------------------------------------------------------

18) splitter
------------

Sends one input to several outputs.
XXX The input frames are not cloned! Unexpected things might happen if
a mutable stream (like e.g. rtp) is being split!

Sinks: 1 (passive)
Sources: n (active)

Node parameters:

Input stream:
any

Output stream:
any


----------------------------------------------------------------------

19) rtttl-player 
----------------

Audio source for playing RTTTL-encoded ringtones.
RTTTL is the "Ringing Tones Text Transfer Language", a popular format
for Nokia mobile phone ringtones.

Sinks: none
Sources: 1 (passive)

Control interfaces:

Node parameters: 
- ACString "rtttl" : The rtttl source code to be played
- boolean "loop" : true: the sample should be looped indefinitely (default: true)
- double "amplitude" : 0.0 <= amplitude <= 1.0 (default: 0.5)

- double "sample_rate" : sample rate in Hz (default: 8000)
- double "frame_duration" : duration of one frame in s (default: 0.02)
- unsigned long "channels" : number of channels (default: 1)
- ACString "sample_format" : "float32_1" | "float32_32768" | "int16" | "int32"
                             (default: "float32_32768")

Output stream:
audio/pcm frames (layout determined by source parameters).

----------------------------------------------------------------------

20) ttone->pcm
--------------

Decoder for generating pcm from audio/tone telephony tones (RFC2833).

Sinks: 1 (active)
Sources: 1 (passive)

Control interfaces: 

Node parameters:
double "ztlp" : zero dBm0 tranmission level point as a fraction of the 
                maximum output stream amplitude (Default: 1.0==maximum amplitude
                of output stream)
double "sample_rate" : sample rate in Hz (default: 8000)
double "frame_duration" : duration of one frame in s (default: 0.02)
unsigned long "channels" : number of channels (default: 1)
ACString "sample_format" : "float32_1" | "float32_32768" | "int16" | "int32"
                               (default: "float32_32768")

Input stream:
audio/tone frames

Output stream:
audio/pcm frames (layout determined by source parameters).

----------------------------------------------------------------------

21) dtmf-generator
------------------

Generates RFC2833 dtmf audio/telephone-event frames from user input.
Will generate a new stream based (timebase=0) for each new synchronous
'run' of digits.
Note: Doesn't generate end frame retransmissions. Frames will be played
without inter-frame pauses; the consumer has to reconstruct the timing from
the frame timestamps.

Sinks: none
Sources: 1 (passive)

Control interfaces: zapIDTMFGenerator

Node parameters:
- double "tone_duration" : duration of tones in s (0.09). 
                           x <= (2^16-1)/sample_rate
- double "pause_duration" : duration of pauses in s (0.07).
                            x <= (2^16-1)/sample_rate
- double "volume" : volume in dBm0 (-8)
- double "sample_rate" : XXX 8000Hz hardcoded atm.
- double "max_frame_duration" : minimum packetization interval in 
                                seconds (0.02).
                                1/sample_rate <= x <= (2^16-1)/sample_rate

Output stream:
audio/telephone-event frames WITHOUT end frame retransmissions

----------------------------------------------------------------------

22) tevent->ttone
-----------------

Filter for generating audio/tone frames from audio/telephone-event
frames (RFC2833). Note: This is currently implemented as a passive
filter. One implication is that it might not deal gracefully with end
frame retransmissions that don't stick to the pipeline's packetization
period. Inter-frame pauses will not be converted to packets; the
consumer has to reconstruct the timing from the frame timestamps.

Sinks: 1 (neutral)
Sources: 1 (neutral)

Control interfaces: 

Node parameters:

Input stream:
audio/telephone-event frames

Output stream:
audio/tone frames

----------------------------------------------------------------------

23) ttone-generator
-------------------

Generates RFC2833 telephony tone frames from user input

Sinks: none
Sources: 1 (passive)

Control interfaces: zapITToneGenerator

Node parameters:
- double "sample_rate" : XXX 8000Hz hardcoded atm.
- double "max_frame_duration" : minimum packetization interval in seconds 
                                (default: 0.02s).
                                1/sample_rate <= x <= (2^16-1)/sample_rate


Output stream:
audio/tone frames

----------------------------------------------------------------------

24) pump
--------

General purpose packet pump. Pumps an unaltered "input" packet to the
output whenever a "clock" frame is received. The timestamp of the
frame is NOT modified.

Sinks: 2 

ACString "name" == "input" : pumped input frames; any stream type (active)
ACString "name" == "clock" : reference clock; any stream type (passive)
Sources: 1 (active)

Node parameters:
- unsigned long "clock_divider" : integer n determining the relative
                                  rate at which packets will be pumped:
                                  For every n'th clock frame an input 
                                  frame will be pumped (default: 1)

Output stream:
same as "input" stream

----------------------------------------------------------------------

25) tevent-rtp-packetizer
-------------------------

Filter for converting telephone-event streams into rtp streams.

Node parameters:
- unsigned short "payload_type" (default: 101)

Input stream:
audio/telephone-event frames

Output stream:
rtp frames (SSRC and sequence number NOT set).

----------------------------------------------------------------------

26) stream-merger
-----------------

Sends unaltered packets from several input streams to the output
stream as soon as they arrive.

Sinks: n (passive)
Sources: 1 (active)

Control interfaces: 
-

Node parameters:

Input streams:
any

Output stream:
as input streams

----------------------------------------------------------------------

27) stream-syncer
-----------------

Adds an offset to the timestamp of input frames, so that the timestamp
of the first frame of an input stream coincides with the last
timestamp seen at the 'timebase' input. Whenever the input stream or
timebase stream changes (i.e. when the frameinfo is different from one
frame to the next), the offset will be recalculated.
Note: Warning: this node mutates frames. XXX maybe we need cloning.

Sinks: 1

- ACString "name" == "input" : input stream (any type) (neutral)
- ACString "name" == "timebase" : timebase stream (any type) (passive)

Sources: 1 

Output stream:
as input stream, with frame timestamps adjusted.

----------------------------------------------------------------------

28) rtp-demuxer
---------------

Dispatches input frames from one input to several outputs based on rtp
payload type number.

Sinks: 1 (passive)
Sources: n (active). 
Source parameters: ACString "payload_type" == payload type of this
output (no default; parameter MUST be specified)

Input stream:
rtp frames

Output streams:
rtp frames; as input

----------------------------------------------------------------------

29) stream-switch
-----------------

Switches one input stream to one of n outputs.

Sinks: 1 (neutral)
Sources: n (neutral)
Source parameters:
ACString "id" : id by which a particular output can be selected with
zapISwitch::selectOutput(). Must be unique among all sources


Control interfaces: zapIStreamSwitch

Input stream:
any

Output stream:
as input stream

----------------------------------------------------------------------

30) stream-tap
--------------

Similar to a splitter, but one output is passive ("type"=="master")
and the other outputs active ("type"=="tap"), i.e. they receive a
packet whenever the master output requests one.
XXX The input frames are not cloned! Unexpected things might happen if
a mutable stream (like e.g. rtp) is being tapped!

Sinks: 1 (active)
Sources: n (1 passive , n-1 active)

Source parameters:
passive master source: ACString "type" == "master"
active tap sources: ACString "type" == "tap"

Node parameters:

Input stream:
any

Output stream:
any

----------------------------------------------------------------------

31) speex-audio-processor
-------------------------

Speex acoustic echo canceller and audio preprocessor


Sinks: 2
ACString "name" == "input" (neutral): Audio as captured by the microphone
ACString "name" == "echo" (active): Audio played through speaker

Sources: 1 (neutral): Audio with echo removed

Node parameters:
- boolean "aec" : toggles acoustic echo cancellation (default: false)
- boolean "aec_2_stage" : toggles 2 stage AEC (default: false)
- float "aec_tail" : AEC tail in ms (default: 300)
- boolean "denoise" : toggles denoising (default: false)
- boolean "agc" : toggles automatic gain control (default: false)
- float "agc_level" : (default: 8000)
- boolean "vad" : toggles voice activity detection (default: false)
- boolean "dereverb" : toggles dereverb (default: false)
- float "dereverb_level" : (default: 0.2)
- float "dereverb_decay" : (default: 0.5)

Control interfaces: zapISpeexAudioProcessor

Input streams:
audio/pcm frames with
- double "sample_rate" == 8000
- double "frame_duration" == 0.02
- unsigned long "channels" == 1
- unsigned long "sample_format" == "float32_32768"

Output stream:
audio/pcm frames (as input streams)

----------------------------------------------------------------------

32) audioio
-----------

Combined audio source/sink. Provides lower resource use and better
synchronization than having separate audioin and audioout. The latter
is important for things like echo cancellation, where the audio output
needs to be correlated with the input.
For each sampling period, the audioio node will call "consumeFrame" on
the attached sink before calling "produceFrame" on the attached
source.

Note: Under some circumstances the combined audioio can lead to choppy
audio (observed on Windows XP when other apps using the sound hardware
are active).  In these cases using separate audioin/audioout might be
better.

Sinks: 1 (active)
Sources: 2 (active)

Source parameters:
ACString "name" == "ain" : audio in.
ACString "name" == "monitor" : receives every frame played on audioout,
                               including missing frames.

Control interfaces: zapIAudioIO

Node parameters:
- zapIAudioDevice "input_device" (default: default audio in)
- zapIAudioDevice "output_device" (default: default audio out)
- int "buffers" : number of buffers mediating between zmk and audio hardware
                  (default: 4, must be >=2)
- double "sample_rate" : sample rate in Hz (default: 8000)
- double "frame_duration" : duration of one frame in s (default: 0.02)
- unsigned long "channels" : number of channels (default: 1)
- ACString "sample_format" : "float32_1" | "float32_32768" | "int16" | "int32"
                             (default: "float32_32768")

Input stream:
audio/pcm frames with
- double "sample_rate" == corresponding node parameter
- double "frame_duration" == corresponding node parameter
- unsigned long "channels" == corresponding node parameter
- unsigned long "sample_format" == corresponding node parameter

Output stream:
audio/pcm frames with
- double "sample_rate" == corresponding node parameter
- double "frame_duration" == corresponding node parameter
- unsigned long "channels" == corresponding node parameter
- unsigned long "sample_format" == corresponding node parameter

----------------------------------------------------------------------

33) stun-client
---------------

Sinks: 1 (passive)
Sources: 1 (active)

Control interfaces: zapIStunClient

Node parameters: - 

Input stream: datagram frames | stun frames

Output stream: datagram frames

----------------------------------------------------------------------

34) stun-server
---------------

Sinks: 1 (passive)
Source: 1 (active)

Control interfaces: zapIStunServer

Node parameters:
- ACString "source_addr" : Source address to insert into binding responses.
                           (no default; mandatory)
- unsigned short "source_port" : Source port to insert into binding 
                                 responses. (no default; mandatory)

Input stream: datagram frames | stun frames

Output stream: datagram frames


----------------------------------------------------------------------

35) stun-demuxer
----------------

Dispatches input frames from one input to a stun server, stun client
or other output, depending on the input frame.

Sinks: 1 (passive)
Sources: 3 (active)

Source parameters:
ACString "name" == "stun-req" : stun requests
ACString "name" == "stun-res" : stun responses
ACString "name" == "other" : other udp traffic

Input stream:
udp frames

Output streams:
stun frames ("stun-req"/"stun-res")
udp frames ("other")

