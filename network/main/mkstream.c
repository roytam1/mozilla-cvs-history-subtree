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
/*
 *  Do stream related stuff like push data down the
 *  stream and determine which converter to use to
 *  set up the stream.
 *
 *  Montulli@netscape.com
 */
#include "mkutils.h"
#include "mkgeturl.h"
#include "netstream.h"
#include "cstream.h"

struct _NET_VoidStreamClass {
	NET_StreamClass *cstream;
};

/* Find a converter routine to create a stream and return the stream struct
*/
PUBLIC NET_VoidStreamClass * 
NET_VoidStreamBuilder  (FO_Present_Types format_out,
                    URL_Struct  *URL_s,
                    MWContext   *context)
{
	NET_VoidStreamClass *rv = PR_NEW(NET_VoidStreamClass);

	if(rv)
	{
		rv->cstream = NET_StreamBuilder(format_out, URL_s, context);

		if(!rv->cstream)
		{
			PR_Free(rv);
			return NULL;
		}
	}

	return rv;
}

PUBLIC NET_VoidStreamClass * 
NET_CStreamToVoidStream(void *cstream)
{
	NET_VoidStreamClass *rv;

	if(!cstream)
		return NULL;

	rv = PR_NEW(NET_VoidStreamClass);

	if(rv)
	{
		rv->cstream = (NET_StreamClass*)cstream;
	}

	return rv;
}

PUBLIC uint32
NET_StreamIsWriteReady(NET_VoidStreamClass *stream)
{
	if(!stream || !stream->cstream)
	{
		PR_ASSERT(0);
		return 0;
	}

	return (*stream->cstream->is_write_ready)(stream->cstream);	
}

PUBLIC int32
NET_StreamPutBlock(NET_VoidStreamClass *stream, char *block, int32 l)
{
	if(!stream || !stream->cstream)
	{
		PR_ASSERT(0);
		return 0;
	}
	return (*stream->cstream->put_block)(stream->cstream, block, l);	
}

PUBLIC void
NET_StreamComplete(NET_VoidStreamClass *stream)
{
	if(!stream || !stream->cstream)
	{
		PR_ASSERT(0);
		return;
	}
	(*stream->cstream->complete)(stream->cstream);	
}

PUBLIC void
NET_StreamAbort(NET_VoidStreamClass *stream, int32 status)
{
	if(!stream || !stream->cstream)
	{
		PR_ASSERT(0);
		return;
	}
	(*stream->cstream->abort)(stream->cstream, status);	
}

PUBLIC char *
NET_StreamName(NET_VoidStreamClass *stream)
{
	if(!stream || !stream->cstream || !stream->cstream->name)
	{
		PR_ASSERT(0);
		return NULL;
	}
	return stream->cstream->name;
}

/* returns -1 on error 0 on success */
PUBLIC int 
NET_SetStreamData(NET_VoidStreamClass *stream, void *data)
{
	if(!stream || !stream->cstream)
	{
		PR_ASSERT(0);
		return -1;
	}

	stream->cstream->data_object = data;
	return 0;
}

PUBLIC void *
NET_GetStreamData(NET_VoidStreamClass *stream)
{
        if(!stream || !stream->cstream)
        {
                PR_ASSERT(0);
                return NULL;
        }

        return stream->cstream->data_object;
}

PUBLIC void
NET_StreamFree(NET_VoidStreamClass *stream)
{
	if(stream)
	{
		PR_FREEIF(stream->cstream);
		PR_Free(stream);
	}
}
