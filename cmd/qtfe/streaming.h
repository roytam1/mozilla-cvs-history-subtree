/* $Id$
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  Portions
 * created by Warwick Allison, Kalle Dalheimer, Eirik Eng, Matthias
 * Ettrich, Arnt Gulbrandsen, Haavard Nord and Paul Olav Tvete are
 * Copyright (C) 1998 Warwick Allison, Kalle Dalheimer, Eirik Eng,
 * Matthias Ettrich, Arnt Gulbrandsen, Haavard Nord and Paul Olav
 * Tvete.  All Rights Reserved.
 *
 * Contributors: Warwick Allison
 *               Kalle Dalheimer
 *               Eirik Eng
 *               Matthias Ettrich
 *               Arnt Gulbrandsen
 *               Haavard Nord
 *               Paul Olav Tvete
 */

#ifndef _STREAMING_H
#define _STREAMING_H

#include <fe_proto.h>

class QtContext;

struct save_as_data
{
  MWContext *context;
  char *name;
  FILE *file;
  int type;
  int insert_base_tag;
  bool use_dialog_p;
  void (*done)(struct save_as_data *);
  void* data;
  int content_length;
  int bytes_read;
  URL_Struct *url;
};

NET_StreamClass *
fe_MakeSaveAsStream ( int /*format_out*/, void * /*data_obj*/,
		      URL_Struct *url_struct, MWContext *context );

NET_StreamClass *
fe_MakeSaveAsStreamNoPrompt( int format_out, void *data_obj,
			     URL_Struct *url_struct, MWContext *context );

struct save_as_data *
make_save_as_data (MWContext *context, bool allow_conversion_p,
		   int type, URL_Struct *url, const char *output_file);

void
fe_save_as_stream_abort_method (NET_StreamClass *stream, int /*status*/);

void
fe_save_as_stream_complete_method (NET_StreamClass *stream);

unsigned int
fe_save_as_stream_write_ready_method (NET_StreamClass *stream);

int
fe_save_as_stream_write_method (NET_StreamClass *stream, const char
				*str, int32 len);  

void fe_save_as_nastiness( QtContext *context, URL_Struct *url,
			   struct save_as_data *sad, int synchronous );

#endif
