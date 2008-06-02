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
#include "nsStringAPI.h"
#include "zapMediaFrame.h"
#include "nsHashPropertyBag.h"
#include <math.h>
#include <ctype.h>

////////////////////////////////////////////////////////////////////////
// zapRtttlPlayer

zapRtttlPlayer::zapRtttlPlayer()
    : mTones(nsnull)
{
}

zapRtttlPlayer::~zapRtttlPlayer()
{
  if (mTones) {
    delete[] mTones;
    mTones = nsnull;
  }
  
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

/* void insertedIntoContainer (in zapIMediaNodeContainer container, in nsIPropertyBag2 node_pars); */
NS_IMETHODIMP
zapRtttlPlayer::InsertedIntoContainer(zapIMediaNodeContainer *container,
                                      nsIPropertyBag2* node_pars)
{
  // unpack node parameters:
  nsCString rtttl;
  ZMK_GetOptionalCString(node_pars,
                         NS_LITERAL_STRING("rtttl"),
                         NS_LITERAL_CSTRING("zap:d=32,o=5,b=140:b4,b4,b4,b4,b4,b4,b4,b4,b4,b4,b4,b4,8p.,f4,f4,8p.,f4,f4,1p"),
                         rtttl);
  mAmplitude = ZMK_GetOptionalDouble(node_pars, NS_LITERAL_STRING("amplitude"), 0.5);
  mLoop = ZMK_GetOptionalBool(node_pars, NS_LITERAL_STRING("loop"), PR_TRUE);

  nsresult rv;
  // init stream parameters:
  rv = mStreamParameters.InitWithProperties(node_pars);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mStreamParameters.sample_rate != 8000 ||
      mStreamParameters.samples != 160 ||
      mStreamParameters.channels != 1 ||
      mStreamParameters.sample_format != sf_float32_32768) {
    NS_ERROR("unsupported sample format! write me!");
    return NS_ERROR_FAILURE;
  }

  // create a new stream info:
  mStreamInfo = mStreamParameters.CreateStreamInfo();
  
  // parse rtttl into Tone structure:
  rv = ParseRTTTL(PromiseFlatCString(rtttl).get());
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* void removedFromContainer (); */
NS_IMETHODIMP
zapRtttlPlayer::RemovedFromContainer()
{
  return NS_OK;
}

/* zapIMediaSource getSource (in nsIPropertyBag2 source_pars); */
NS_IMETHODIMP
zapRtttlPlayer::GetSource(nsIPropertyBag2 *source_pars, zapIMediaSource **_retval)
{
  if (mOutput) {
    NS_ERROR("output end already connected");
    return NS_ERROR_FAILURE;
  }

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

/* void connectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapRtttlPlayer::ConnectSink(zapIMediaSink *sink)
{
  if (mOutput) {
    NS_ERROR("output end already connected");
    return NS_ERROR_FAILURE;
  }
  mOutput = sink;
  return NS_OK;
}

/* void disconnectSink (in zapIMediaSink sink); */
NS_IMETHODIMP
zapRtttlPlayer::DisconnectSink(zapIMediaSink *sink)
{
  mOutput = nsnull;
  return NS_OK;
}

/* zapIMediaFrame produceFrame (); */
NS_IMETHODIMP
zapRtttlPlayer::ProduceFrame(zapIMediaFrame ** _retval)
{
  // construct audio frame:
  zapMediaFrame* frame = new zapMediaFrame();
  frame->AddRef();
  frame->mStreamInfo = mStreamInfo;
  frame->mTimestamp = 0; // XXX
  
  // write frame data:
  frame->mData.SetLength(mStreamParameters.GetFrameLength());
  float* d = (float*)frame->mData.BeginWriting();
  float sample_step = 1.0/mStreamParameters.sample_rate;
  int samples_left = mStreamParameters.samples;
  while (1) {
    int tone_samples_left = (int)((mTones[mTonePointer.current].duration-
                                   mTonePointer.offset)*mStreamParameters.sample_rate);
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
      mTonePointer.offset += samples_left/(float)mStreamParameters.sample_rate;
      break;
    }
  }

  *_retval = frame;
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
