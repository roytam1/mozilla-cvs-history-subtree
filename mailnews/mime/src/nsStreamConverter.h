/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#ifndef nsStreamConverter_h_
#define nsStreamConverter_h_

#include "nsCOMPtr.h"
#include "nsIStreamConverter.h" 
#include "nsIOutputStream.h"
#include "nsIMimeEmitter.h" 
#include "nsIURI.h"

class nsStreamConverter : public nsIStreamConverter { 
public: 
  nsStreamConverter();
  virtual ~nsStreamConverter();
   
  /* this macro defines QueryInterface, AddRef and Release for this class */
  NS_DECL_ISUPPORTS 

  //
  // 
  // This is the output stream where the stream converter will write processed data after 
  // conversion. 
  // 
  NS_IMETHOD SetOutputStream(nsIOutputStream *aOutStream, nsIURI *aURI, nsMimeOutputType aType,
                             nsMimeOutputType *aOutFormat, char **aOutputContentType);

  // 
  // This is the type of output operation that is being requested by libmime. The types
  // of output are specified by nsIMimeOutputType enum
  // 
  NS_IMETHOD SetOutputType(nsMimeOutputType aType); 

  // 
  // The output listener can be set to allow for the flexibility of having the stream converter 
  // directly notify the listener of the output stream for any processed/converter data. If 
  // this output listener is not set, the data will be written into the output stream but it is 
  // the responsibility of the client of the stream converter to handle the resulting data. 
  // 
  NS_IMETHOD SetOutputListener(nsIStreamListener *aOutListener); 

  // 
  // This is needed by libmime for MHTML link processing...this is the URI associated
  // with this input stream
  // 
  NS_IMETHOD SetStreamURI(nsIURI *aURI); 

  /////////////////////////////////////////////////////////////////////////////
  // Methods for nsIStreamListener...
  /////////////////////////////////////////////////////////////////////////////
  //
  // Notify the client that data is available in the input stream.  This
  // method is called whenver data is written into the input stream by the
  // networking library...
  //
  NS_IMETHOD OnDataAvailable(nsIChannel * aChannel, 
							 nsISupports    *ctxt, 
                             nsIInputStream *inStr, 
                             PRUint32       sourceOffset, 
                             PRUint32       count);

  /////////////////////////////////////////////////////////////////////////////
  // Methods for nsIStreamObserver 
  /////////////////////////////////////////////////////////////////////////////
  //
  // Notify the observer that the URL has started to load.  This method is
  // called only once, at the beginning of a URL load.
  //
  NS_IMETHOD OnStartRequest(nsIChannel * aChannel, nsISupports *ctxt);

  //
  // Notify the observer that the URL has finished loading.  This method is 
  // called once when the networking library has finished processing the 
  //
  NS_IMETHOD OnStopRequest(nsIChannel * aChannel, nsISupports *ctxt, nsresult status, const PRUnichar *errorMsg);


  ////////////////////////////////////////////////////////////////////////////
  // nsStreamConverter specific methods:
  ////////////////////////////////////////////////////////////////////////////
  NS_IMETHOD          InternalCleanup(void);
  NS_IMETHOD          DetermineOutputFormat(const char *url, nsMimeOutputType *newType);

private:
  nsCOMPtr<nsIOutputStream>     mOutStream;     // output stream
  nsCOMPtr<nsIStreamListener>   mOutListener;   // output stream listener

  nsCOMPtr<nsIMimeEmitter>      mEmitter;       // emitter being used...
  nsCOMPtr<nsIURI>              mURI;           // URI being processed
  nsMimeOutputType              mOutputType;    // the output type we should use for the operation

  void                          *mBridgeStream; // internal libmime data stream
  PRInt32                       mTotalRead;     // Counter variable

  // Type of output, entire message, header only, body only
  char                          *mOutputFormat;
  char                          *mOverrideFormat; // this is a possible override for emitter creation
  PRBool                        mWrapperOutput;   // Should we output the frame split message display 
  PRBool                        mDoneParsing;     // If this is true, we've already been told by libmime to stop sending
                                                  // data so don't feed the parser any more!
}; 

/* this function will be used by the factory to generate an class access object....*/
extern nsresult NS_NewStreamConverter(nsIStreamConverter **aInstancePtrResult);

#endif /* nsStreamConverter_h_ */
