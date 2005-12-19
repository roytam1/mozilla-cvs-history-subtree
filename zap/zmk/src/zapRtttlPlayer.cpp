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

#include "zapRtttlPlayer.h"
#include "stdio.h"
#include "nsString.h"
#include "zapMediaFrame.h"
#include "nsHashPropertyBag.h"
#include <math.h>

////////////////////////////////////////////////////////////////////////
// zapRtttlPlayer

zapRtttlPlayer::zapRtttlPlayer()
    : mPlay(PR_TRUE),
      mWaiting(PR_FALSE),
      mTones(nsnull),
      mAmplitude(0.5),
      mLoop(PR_TRUE)
{
#ifdef DEBUG_afri_zmk
  printf("zapRtttlPlayer::zapRtttlPlayer()\n");
#endif
}

zapRtttlPlayer::~zapRtttlPlayer()
{
  if (mTones) {
    delete[] mTones;
    mTones = nsnull;
  }
  
#ifdef DEBUG_afri_zmk
  printf("zapRtttlPlayer::~zapRtttlPlayer()\n");
#endif
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(zapRtttlPlayer)
NS_IMPL_RELEASE(zapRtttlPlayer)

NS_INTERFACE_MAP_BEGIN(zapRtttlPlayer)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaNode)
  NS_INTERFACE_MAP_ENTRY(zapIMediaSource)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// zapIMediaNode methods:

/* void addedToGraph (in zapIMediaGraph graph, in ACString id, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapRtttlPlayer::AddedToGraph(zapIMediaGraph *graph,
                             const nsACString & id,
                             nsIPropertyBag2* node_pars)
{
  // node parameter defaults:
  nsCString rtttl = NS_LITERAL_CSTRING("zap:d=32,o=5,b=140:b4,b4,b4,b4,b4,b4,b4,b4,b4,b4,b4,b4,8p.,f4,f4,8p.,f4,f4,1p");
  // unpack node parameters:
  if (node_pars) {
    node_pars->GetPropertyAsACString(NS_LITERAL_STRING("rtttl"),
                                     rtttl);
    node_pars->GetPropertyAsBool(NS_LITERAL_STRING("loop"), &mLoop);
    node_pars->GetPropertyAsDouble(NS_LITERAL_STRING("amplitude"), &mAmplitude);
  }
  
  // parse rtttl into Tone structure:
  return ParseRTTTL(PromiseFlatCString(rtttl).get());
}

/* void removedFromGraph (in zapIMediaGraph graph); */
NS_IMETHODIMP
zapRtttlPlayer::RemovedFromGraph(zapIMediaGraph *graph)
{
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapRtttlPlayer::GetSource(nsIPropertyBag2 *source_pars, zapIMediaSource **_retval)
{
  if (mSink) {
    NS_ERROR("already connected");
    return NS_ERROR_FAILURE;
  }

  // source parameter defaults:
  mSampleRate = 8000; // 8000Hz
  mFrameDuration = 0.02; // 20ms
  mNumChannels = 1; // mono
  mSampleFormat = sf_float32_32768;

  // unpack source parameters:
  if (source_pars) {
    source_pars->GetPropertyAsDouble(NS_LITERAL_STRING("sample_rate"),
                                     &mSampleRate);
    source_pars->GetPropertyAsDouble(NS_LITERAL_STRING("frame_duration"),
                                     &mFrameDuration);
    source_pars->GetPropertyAsUint32(NS_LITERAL_STRING("channels"),
                                     &mNumChannels);
    nsCString sampleformat_string;
    if (NS_SUCCEEDED(source_pars->GetPropertyAsACString(NS_LITERAL_STRING("sample_format"),
                                                        sampleformat_string))) {
      mSampleFormat = StrToZapAudioSampleFormat(sampleformat_string);
      if (mSampleFormat == sf_unknown) {
        NS_ERROR("unknown sample format");
        return NS_ERROR_FAILURE;
      }
    }
  }

  if (mSampleRate != 8000 ||
      mFrameDuration != 0.02 ||
      mNumChannels != 1 ||
      mSampleFormat != sf_float32_32768) {
    NS_ERROR("unsupported sample format! write me!");
    return NS_ERROR_FAILURE;
  }
      

  mSamplesPerFrame = (PRUint32)(mSampleRate*mFrameDuration*mNumChannels);

  // create a new streaminfo:
  nsCOMPtr<nsIWritablePropertyBag> bag;
  NS_NewHashPropertyBag(getter_AddRefs(bag));
  mStreamInfo = do_QueryInterface(bag);

  mStreamInfo->SetPropertyAsACString(NS_LITERAL_STRING("type"),
                                     NS_LITERAL_CSTRING("audio"));
  mStreamInfo->SetPropertyAsDouble(NS_LITERAL_STRING("sample_rate"),
                                   mSampleRate);
  mStreamInfo->SetPropertyAsDouble(NS_LITERAL_STRING("frame_duration"),
                                   mFrameDuration);
  mStreamInfo->SetPropertyAsUint32(NS_LITERAL_STRING("channels"),
                                   mNumChannels);

  nsCString format_string;
  ZapAudioSampleFormatToStr(mSampleFormat, format_string);
  mStreamInfo->SetPropertyAsACString(NS_LITERAL_STRING("sample_format"),
                                     format_string);
  
  *_retval = this;
  NS_ADDREF(*_retval);
  return NS_OK;
}

/* zapIMediaSink getSink (in nsIPropertyBag2 sink_pars); */
NS_IMETHODIMP
zapRtttlPlayer::GetSink(nsIPropertyBag2 *sink_pars, zapIMediaSink **_retval)
{
  NS_ERROR("rtttl-player is a source-only node");
  return NS_ERROR_FAILURE;
}

//----------------------------------------------------------------------
// zapIMediaSource methods:

/* void connectSink (in zapIMediaSink sink, in ACString connection_id); */
NS_IMETHODIMP
zapRtttlPlayer::ConnectSink(zapIMediaSink *sink,
                            const nsACString & connection_id)
{
  NS_ASSERTION(!mSink, "sink already connected");
  mSink = sink;
  return NS_OK;
}

/* void disconnectSink (in zapIMediaSink sink, in ACString connection_id); */
NS_IMETHODIMP
zapRtttlPlayer::DisconnectSink(zapIMediaSink *sink,
                               const nsACString & connection_id)
{
  mWaiting = PR_FALSE;
  mSink = nsnull;
  return NS_OK;
}

/* void requestFrame (); */
NS_IMETHODIMP
zapRtttlPlayer::RequestFrame()
{
  if (mWaiting) {
    NS_ASSERTION(!mPlay, "inconsistent state");
    return NS_OK; // already waiting
  }
  // construct audio frame:
  zapMediaFrame* frame = new zapMediaFrame();
  frame->AddRef();
  frame->mStreamInfo = mStreamInfo;
  frame->mTimestamp = 0; // XXX

  // write frame data:
  frame->mData.SetLength(mSamplesPerFrame * GetZapAudioSampleSize(mSampleFormat));
  float* d = (float*)frame->mData.BeginWriting();
  float sample_step = 1/mSampleRate;
  int samples_left = mSamplesPerFrame;
  while (1) {
    int tone_samples_left = (int)((mTones[mTonePointer.current].duration-
                                   mTonePointer.offset)*mSampleRate);
    int c = PR_MIN(samples_left, tone_samples_left);
    float sample_time = mTonePointer.offset;
    float omega = 6.28318530718*mTones[mTonePointer.current].frequency;
    while (c--) {
      *d++ = (float)(mAmplitude*32768.0*sin(omega*sample_time));
      sample_time += sample_step;
    }
    if (samples_left > tone_samples_left) {
      // advance to next tone:
      mTonePointer.offset = 0.0f;
      if (mTonePointer.current < mToneCount-1) {
        ++mTonePointer.current;
      }
      else {
        mTonePointer.current = 0;
        if (!mLoop) {
          ///XX add silence
          ///XXX need to send EOF frame
        }
      }
      samples_left -= tone_samples_left;
    }
    else {
      // advance offset within tone:
      mTonePointer.offset += samples_left/mSampleRate;
      break;
    }
  }
    
  mWaiting = PR_FALSE;
  if (mSink)
    mSink->ProcessFrame(frame);
  frame->Release();
  return NS_OK;
}

//----------------------------------------------------------------------
// Implementation helpers


/*
ParseRTTTL: parse an RTTL string into an array of Note

RTTP syntax:

<name> ":" [<control section>] ":" <tone-commands>

	<name> := <char> ; maximum name length 10 characters

	<control-section> := <control-pair> ["," <control-section>]

		<control-pair> := <control-name> ["="] <control-value>

		<control-name> := "o" | "d" | "b"
		; Valid in control section: o=default scale, d=default duration, b=default beats per minute. 
		; if not specified, defaults are 4=duration, 6=scale, 63=beats-per-minute
		; any unknown control-names must be ignored

		<tone-commands> := <tone-command> ["," <tone-commands>]

		<tone-command> :=<note> | <control-pair>

		<note> := [<duration>] <note> [<scale>] [<special-duration>] <delimiter>

			<duration> := "1" | "2" | "4" | "8" | "16" | "32" 
			; duration is divider of full note duration, eg. 4 represents a quarter note

			<note> := "P" | "C" | "C#" | "D" | "D#" | "E" | "F" | "F#" | "G" | "G#" | "A" | "A#" | "B" 

			<scale> :="4" | "5" | "6" | "7"
			; octave 4: A=440Hz, 5: A=880Hz, 6: A=1.76 kHz, 7: A=3.52 kHz

			<special-duration> := "." ; Dotted note
*/

// helper to parse "<letter>=<int>"
bool zapRtttlPlayer::ParseControlValue(const char **bufp,  float*result)
{
  ++(*bufp);
  if (**bufp == '=') ++(*bufp);
  if (**bufp == '\0') return false;

  *result = (float)strtod(*bufp, (char**)bufp);
  return true;
}

// helper to parse a tone-command
bool zapRtttlPlayer::ParseToneCommand(const char **bufp, Tone* tone,
                                      float base_octave,
                                      float base_duration,
                                      float base_bpm)
{
  float divider = (float)strtol(*bufp, (char**)bufp, 10);
  if (divider == 0.0f) divider = base_duration;
  tone->duration = 240 / base_bpm / divider;
  if (**bufp == '\0') return false;

  // well-tempered American Standard pitch (2^(1/12) ratios, 440Hz A4) scale
  switch (tolower(**bufp)) {
    case 'p':
      tone->frequency = 0;
      break;
    case 'c':
      tone->frequency = 32.7031956625;
      break;
    case 'd':
      tone->frequency = 36.7080959896;
      break;
    case 'e':
      tone->frequency = 41.2034446141;
      break;
    case 'f':
      tone->frequency = 43.6535289291;
      break;
    case 'g':
      tone->frequency = 48.9994294977;
      break;
    case 'a':
      tone->frequency = 55.0;
      break;
    case 'h':
    case 'b':
      tone->frequency = 61.7354126571;
      break;
    default:
      return false;
      break;
      
  }
  ++(*bufp);
  if (**bufp == '#') {
    tone->frequency *= 1.05946309436;
    ++(*bufp);
  }
  if (**bufp == '.') {
    // dotted note
    tone->duration *= 1.5f;
    ++(*bufp);
  }
  int octave = strtol(*bufp, (char**)bufp, 10);
  if (octave == 0) octave = (int)base_octave;
  if (octave < 1 || octave > 10) {
    return false;
  }
  tone->frequency *= (1<<(octave-1));
  if (**bufp == '.') {
    // dotted note
    tone->duration *= 1.5f;
    ++(*bufp);
  }
  // skip past deliminators:
  while (**bufp == ',' || isspace(**bufp)) {
    ++(*bufp);
  }

  return true;
}

nsresult
zapRtttlPlayer::ParseRTTTL(const char *buf)
{
  if (mTones) {
    delete[] mTones;
    mTones = nsnull;
  }
  mToneCount = 0;
  mTonePointer.current = 0;
  mTonePointer.offset = 0.0f;

  // hunt for first ':'
  bool done = false;
  while (!done) {
    switch (*buf) {
      case '\0':
        return NS_ERROR_FAILURE;
        break;
      case ':':
        done = true;
        break;
    }
    ++buf;
  }

  // parse control section:
  done = false;
  float base_octave = 6.0;
  float base_duration = 4.0;
  float base_bpm = 63.0;
  while (!done) {
    switch (*buf) {
      case '\0':
        return NS_ERROR_FAILURE;
        break;
      case ':':
        done = true;
        break;
      case 'o':
        if (!ParseControlValue(&buf, &base_octave))
          return NS_ERROR_FAILURE;
        break;
      case 'd':
        if (!ParseControlValue(&buf, &base_duration))
          return NS_ERROR_FAILURE;
        break;
      case 'b':
        if (!ParseControlValue(&buf, &base_bpm))
          return NS_ERROR_FAILURE;
        break;
      default:
        ++buf;
        break;
    }
  }

  ++buf;
  if (*buf == '\0') return NS_ERROR_FAILURE; // no tone commands
  
  // count number of tone commands
  mToneCount = 1;
  const char* buf2 = buf;
  while (*buf2) {
    if (*buf2 == ',') ++mToneCount;
    ++buf2;
  }

  // skip past deliminators:
  while (*buf == ',' || *buf == ' ') {
    ++buf;
  }
  
  // initialize tones array:
  mTones = new Tone[mToneCount];
  
  // parse tone commands:
  PRInt32 tone_index = 0;
  while (*buf != '\0') {
    if (!ParseToneCommand(&buf, mTones+tone_index,
                          base_octave, base_duration, base_bpm))
    {
      delete[] mTones;
      mTones = nsnull;
      mToneCount = 0;
      return NS_ERROR_FAILURE;
    }
    ++tone_index;
  }
  NS_ASSERTION(tone_index == mToneCount, "tone parser error");
  return NS_OK;
}
