/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
#include "nsCOMPtr.h"
#include "stdio.h"
#include "mimecom.h"
#include "modmimee.h"
#include "nscore.h"
#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsStreamConverter.h"
#include "comi18n.h"
#include "prmem.h"
#include "prprf.h"
#include "plstr.h"
#include "mimemoz2.h"
#include "nsMimeTypes.h"
#include "nsRepository.h"
#include "nsIURL.h"
#include "nsString.h"
#include "nsIServiceManager.h"

//
// Utility routines needed by this interface...
//

nsresult
nsStreamConverter::DetermineOutputFormat(const char *url,  nsMimeOutputType *aNewType)
{
  // Default to html the entire document...
	*aNewType = nsMimeOutput::nsMimeMessageQuoting;

  // Do sanity checking...
  if ( (!url) || (!*url) )
  {
    mOutputFormat = PL_strdup("text/html");
    return NS_OK;
  }

  char *format = PL_strcasestr(url, "?outformat=");
  char *part   = PL_strcasestr(url, "?part=");
  char *header = PL_strcasestr(url, "?header=");

  if (!format) format = PL_strcasestr(url, "&outformat=");
  if (!part) part = PL_strcasestr(url, "&part=");
  if (!header) header = PL_strcasestr(url, "&header=");

  // First, did someone pass in a desired output format. They will be able to
  // pass in any content type (i.e. image/gif, text/html, etc...but the "/" will
  // have to be represented via the "%2F" value
  if (format)
  {
    format += PL_strlen("?outformat=");
    while (*format == ' ')
      ++format;

    if ((format) && (*format))
    {
      char *ptr;
      PR_FREEIF(mOutputFormat);
      mOutputFormat = PL_strdup(format);
      mOverrideFormat = PL_strdup("raw");
      ptr = mOutputFormat;
      do
      {
        if ( (*ptr == '?') || (*ptr == '&') || 
             (*ptr == ';') || (*ptr == ' ') )
        {
          *ptr = '\0';
          break;
        }
        else if (*ptr == '%')
        {
          if ( (*(ptr+1) == '2') &&
               ( (*(ptr+2) == 'F') || (*(ptr+2) == 'f') )
              )
          {
            *ptr = '/';
            memmove(ptr+1, ptr+3, PL_strlen(ptr+3));
            *(ptr + PL_strlen(ptr+3) + 1) = '\0';
            ptr += 3;
          }
        }
      } while (*ptr++);
  
      // Don't muck with this data!
      *aNewType = nsMimeOutput::nsMimeMessageRaw;
      return NS_OK;
    }
  }

  if (!part)
  {
    if (header)
    {
      char *ptr2 = PL_strcasestr ("only", (header+PL_strlen("?header=")));
      char *ptr3 = PL_strcasestr ("quote", (header+PL_strlen("?header=")));
      if (ptr2)
      {
        PR_FREEIF(mOutputFormat);
        mOutputFormat = PL_strdup("text/xml");
        *aNewType = nsMimeOutput::nsMimeMessageHeaderDisplay;
      }
      else if (ptr3)
      {
        PR_FREEIF(mOutputFormat);
        mOutputFormat = PL_strdup("text/html");
        *aNewType = nsMimeOutput::nsMimeMessageQuoting;
      }
    }
    else
    {
      mWrapperOutput = PR_TRUE;
      PR_FREEIF(mOutputFormat);
      mOutputFormat = PL_strdup("text/html");
    }
  }
  else // this is a part that should just come out raw!
  {
    PR_FREEIF(mOutputFormat);
    mOutputFormat = PL_strdup("raw");
    *aNewType = nsMimeOutput::nsMimeMessageRaw;
  }

  return NS_OK;
}

nsresult 
nsStreamConverter::InternalCleanup(void)
{
  PR_FREEIF(mOutputFormat);
  if (mBridgeStream)
  {
    mime_bridge_destroy_stream(mBridgeStream);
    mBridgeStream = nsnull;
  }

  PR_FREEIF(mOverrideFormat);
  return NS_OK;
}

/* 
 * Inherited methods for nsMimeConverter
 */
nsStreamConverter::nsStreamConverter()
{
  /* the following macro is used to initialize the ref counting data */
  NS_INIT_REFCNT();

  // Init member variables...
  mOutStream = nsnull;
  mOutListener = nsnull;
  mOverrideFormat = nsnull;

  mWrapperOutput = PR_FALSE;
  mBridgeStream = NULL;
  mTotalRead = 0;
  mOutputFormat = PL_strdup("text/html");
  mEmitter = NULL;
  mURI = nsnull;
  mDoneParsing = PR_FALSE;
}

nsStreamConverter::~nsStreamConverter()
{
  InternalCleanup();
}

/* 
 * This function will be used by the factory to generate an 
 * mime object class object....
 */
nsresult 
NS_NewStreamConverter(nsIStreamConverter ** aInstancePtrResult)
{
	/* note this new macro for assertions...they can take 
     a string describing the assertion */
	//nsresult result = NS_OK;
	NS_PRECONDITION(nsnull != aInstancePtrResult, "nsnull ptr");
	if (nsnull != aInstancePtrResult)
	{
		nsStreamConverter *obj = new nsStreamConverter();
		if (obj)
			return obj->QueryInterface(nsIStreamConverter::GetIID(), (void**) aInstancePtrResult);
		else
			return NS_ERROR_OUT_OF_MEMORY; /* we couldn't allocate the object */
	}
	else
		return NS_ERROR_NULL_POINTER; /* aInstancePtrResult was NULL....*/
}

/* 
 * The following macros actually implement addref, release and 
 * query interface for our component. 
 */
/* the following macro actually implement addref, release and query interface for our component. */
NS_IMPL_ISUPPORTS(nsStreamConverter, nsIStreamConverter::GetIID());

///////////////////////////////////////////////////////////////
// nsStreamConverter definitions....
///////////////////////////////////////////////////////////////
//
// 
// This is the output stream where the stream converter will write processed data after 
// conversion. 
// 
nsresult 
nsStreamConverter::SetOutputStream(nsIOutputStream *aOutStream, nsIURI *aURI, nsMimeOutputType aType,
                                   nsMimeOutputType * aOutFormat, char **aOutputContentType)
{
  nsresult            res;
  nsMimeOutputType    newType;

  if (!aURI)
    return NS_ERROR_OUT_OF_MEMORY;

  newType = aType;
  switch (aType) 
  {
  case nsMimeOutput::nsMimeMessageSplitDisplay:    // the wrapper HTML output to produce the split header/body display
      mWrapperOutput = PR_TRUE;
      PR_FREEIF(mOutputFormat);
      mOutputFormat = PL_strdup("text/html");
      break;

  case nsMimeOutput::nsMimeMessageHeaderDisplay:   // the split header/body display
      PR_FREEIF(mOutputFormat);
      mOutputFormat = PL_strdup("text/xml");
      break;

  case nsMimeOutput::nsMimeMessageBodyDisplay:   // the split header/body display
      PR_FREEIF(mOutputFormat);
      mOutputFormat = PL_strdup("text/html");
      break;

  case nsMimeOutput::nsMimeMessageQuoting:   // all HTML quoted output
      PR_FREEIF(mOutputFormat);
      mOutputFormat = PL_strdup("text/html");
      break;

  case nsMimeOutput::nsMimeMessageRaw:       // the raw RFC822 data (view source) and attachments
      PR_FREEIF(mOutputFormat);
      mOutputFormat = PL_strdup("raw");
      break;

    default:   // case nsMimeUnknown (// Don't know the format, figure it out from the URL)
    {
      char *url;
      if (NS_FAILED(aURI->GetSpec(&url)))
        return NS_ERROR_OUT_OF_MEMORY;
      DetermineOutputFormat(url, &newType);
      PR_FREEIF(url);
    }
  }

  // 
  // We will first find an appropriate emitter in the repository that supports 
  // the requested output format.
  //
  nsAutoString progID (eOneByte);
  progID = "component://netscape/messenger/mimeemitter;type=";
  if (mOverrideFormat)
    progID += mOverrideFormat;
  else
    progID += mOutputFormat;

  res = nsComponentManager::CreateInstance(progID.GetBuffer(), nsnull,
										                       nsIMimeEmitter::GetIID(),
										                       (void **) getter_AddRefs(mEmitter));
  if ((NS_FAILED(res)) || (!mEmitter))
  {
#ifdef NS_DEBUG
	  printf("Unable to create the correct converter!\n");
#endif
    return NS_ERROR_OUT_OF_MEMORY;
  }

  // make sure to set these!
  mOutStream = aOutStream;
  SetStreamURI(aURI);

  mEmitter->Initialize(aURI);
  mEmitter->SetOutputStream(aOutStream);
  mBridgeStream = mime_bridge_create_stream(mEmitter, this, aURI, newType);

  // What type is libmime going to produce?
  *aOutFormat = newType;
  if (PL_strcasecmp(mOutputFormat, "raw") == 0)
    *aOutputContentType = PL_strdup(UNKNOWN_CONTENT_TYPE);
  else
    *aOutputContentType = PL_strdup(mOutputFormat);

  if (!mBridgeStream)
    return NS_ERROR_OUT_OF_MEMORY;
  else
    return NS_OK;
}

// 
// This is the type of output operation that is being requested by libmime. The types
// of output are specified by nsIMimeOutputType enum
// 
nsresult 
nsStreamConverter::SetOutputType(nsMimeOutputType aType)
{
  mOutputType = aType;
  if (mBridgeStream)
    mime_bridge_set_output_type(mBridgeStream, aType);
  return NS_OK;
}

// 
// The output listener can be set to allow for the flexibility of having the stream converter 
// directly notify the listener of the output stream for any processed/converter data. If 
// this output listener is not set, the data will be written into the output stream but it is 
// the responsibility of the client of the stream converter to handle the resulting data. 
// 
nsresult 
nsStreamConverter::SetOutputListener(nsIStreamListener *aOutListener)
{
  mOutListener = aOutListener;
  return mEmitter->SetOutputListener(aOutListener);
}

// 
// This is needed by libmime for MHTML link processing...this is the URI associated
// with this input stream
// 
nsresult 
nsStreamConverter::SetStreamURI(nsIURI *aURI)
{
  mURI = aURI;
  if (mBridgeStream)
    return mime_bridge_new_new_uri((nsMIMESession *)mBridgeStream, aURI);
  else
    return NS_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Methods for nsIStreamListener...
/////////////////////////////////////////////////////////////////////////////
//
// Notify the client that data is available in the input stream.  This
// method is called whenver data is written into the input stream by the
// networking library...
//
nsresult 
nsStreamConverter::OnDataAvailable(nsIChannel * /* aChannel */, nsISupports    *ctxt, 
                                   nsIInputStream *aIStream, 
                                   PRUint32       sourceOffset, 
                                   PRUint32       aLength)
{
  nsresult        rc;
  PRUint32        readLen = aLength;

  // If this is the first time through and we are supposed to be 
  // outputting the wrapper two pane URL, then do it now.
  if (mWrapperOutput)
  {
    PRUint32    written;
    char        outBuf[1024];
char *output = "\
<HTML>\
<FRAMESET ROWS=\"30%%,70%%\">\
<FRAME NAME=messageHeader SRC=\"%s?header=only\">\
<FRAME NAME=messageBody SRC=\"%s?header=none\">\
</FRAMESET>\
</HTML>";

    char *url = nsnull;
    if (NS_FAILED(mURI->GetSpec(&url)))
      return NS_ERROR_FAILURE;
  
    PR_snprintf(outBuf, sizeof(outBuf), output, url, url);
    PR_FREEIF(url);
    if (mEmitter)
      mEmitter->Write(outBuf, PL_strlen(outBuf), &written);
    mTotalRead += written;

    // RICHIE - will this stop the stream???? Not sure.    
    return NS_ERROR_FAILURE;
  }

  if (!mOutStream)
    return NS_ERROR_FAILURE;

  char *buf = (char *)PR_Malloc(aLength);
  if (!buf)
    return NS_ERROR_OUT_OF_MEMORY; /* we couldn't allocate the object */

  mTotalRead += aLength;
  aIStream->Read(buf, aLength, &readLen);
  rc = mime_display_stream_write((nsMIMESession *) mBridgeStream, buf, readLen);
  PR_FREEIF(buf);
  if (NS_FAILED(rc))
    mDoneParsing = PR_TRUE;
  return rc;
}

/////////////////////////////////////////////////////////////////////////////
// Methods for nsIStreamObserver 
/////////////////////////////////////////////////////////////////////////////
//
// Notify the observer that the URL has started to load.  This method is
// called only once, at the beginning of a URL load.
//
nsresult 
nsStreamConverter::OnStartRequest(nsIChannel * /* aChannel */, nsISupports *ctxt)
{
#ifdef NS_DEBUG
    printf("nsStreamConverter::OnStartRequest()\n");
#endif

  return NS_OK;
}

//
// Notify the observer that the URL has finished loading.  This method is 
// called once when the networking library has finished processing the 
//
nsresult 
nsStreamConverter::OnStopRequest(nsIChannel * /* aChannel */, nsISupports *ctxt, nsresult status, const PRUnichar *errorMsg)
{
#ifdef NS_DEBUG
    printf("nsStreamConverter::OnStopRequest()\n");
#endif

  // 
  // Now complete the emitter and do necessary cleanup!
  //
  if (mEmitter)
  {
    mEmitter->Complete();
  }

  //
  // Now complete the stream!
  //
  if (mBridgeStream)
    //mime_display_stream_complete((nsMIMESession *)mBridgeStream, mDoneParsing);
    mime_display_stream_complete((nsMIMESession *)mBridgeStream);

  // First close the output stream...
  mOutStream->Close();

  // Make sure to do necessary cleanup!
  InternalCleanup();

  // Time to return...
  return NS_OK;
}
