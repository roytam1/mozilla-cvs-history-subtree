/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#ifndef ns4xPluginStream_h__
#define ns4xPluginStream_h__

#include "jri.h"                 // XXX should be jni.h
#include "nsIPlug.h"
#include "ns4xPluginInstance.h"

////////////////////////////////////////////////////////////////////////

/**
 * A 5.0 wrapper for a 4.x plugin stream.
 */
class ns4xPluginStream : public NPIPluginStream {
public:

    ////////////////////////////////////////////////////////////////////////
    // NPIPluginStream methods

    // (Corresponds to NPP_WriteReady.)
    NS_IMETHOD_(PRInt32)
    WriteReady(void);

    // (Corresponds to NPP_Write and NPN_Write.)
    NS_IMETHOD_(PRInt32)
    Write(PRInt32 len, const char* buffer);

    // (Corresponds to NPP_NewStream's stype return parameter.)
    NS_IMETHOD_(NPStreamType)
    GetStreamType(void);

    // (Corresponds to NPP_StreamAsFile.)
    NS_IMETHOD_(void)
    AsFile(const char* fname);

    ////////////////////////////////////////////////////////////////////////
    // Methods specific to ns4xPluginStream

    /**
     * Construct a new 4.x plugin stream associated with the specified
     * instance and stream peer.
     */
    ns4xPluginStream(ns4xPluginInstance* instance, NPIPluginStreamPeer* peer);

    /**
     * Do internal initialization. This actually calls into the 4.x plugin
     * to create the stream, and may fail (which is why it's separate from
     * the constructor).
     */
    NS_METHOD_(NPPluginError)
    Initialize(void);

    NS_DECL_ISUPPORTS

protected:
    virtual ~ns4xPluginStream();

    /**
     * The plugin instance to which this stream belongs.
     */
    ns4xPluginInstance* fInstance;

    /**
     * The peer associated with this stream.
     */
    NPIPluginStreamPeer* fPeer;

    /**
     * The type of stream, for the peer's use.
     */
    NPStreamType fStreamType;

    /**
     * The 4.x-style structure used to contain stream information.
     * This is what actually gets used to communicate with the plugin.
     */
    NPStream fNPStream;

    /** 
     * Set to <b>TRUE</b> if the peer implements
     * <b>NPISeekablPluginStreamPeer</b>.
     */
    PRBool fSeekable;

    /** 
     * Tracks the position in the content that is being
     * read. 4.x-style plugins expect to be told the offset in the
     * buffer that they should read <i>to</i>, even though it's always
     * done serially.
     */
    PRUint32 fPosition;
};


#endif // ns4xPluginStream_h__


