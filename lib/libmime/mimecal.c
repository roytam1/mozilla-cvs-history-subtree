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

/* mimecal.c --- definition of the MimeInlineTextCalendar class (see mimei.h)
   Created: Terry Weissman <terry@netscape.com>, 29-Jan-98.
 */

#include "mimecal.h"
#include "xp_core.h"
#include "xp_mem.h"

#ifdef DEBUG_terry
#include "netcburl.h"
#endif /* DEBUG_terry */

#define MIME_SUPERCLASS mimeInlineTextClass
MimeDefClass(MimeInlineTextCalendar, MimeInlineTextCalendarClass,
	     mimeInlineTextCalendarClass, &MIME_SUPERCLASS);

static int MimeInlineTextCalendar_parse_line (char *, int32, MimeObject *);
static int MimeInlineTextCalendar_parse_eof (MimeObject *, XP_Bool);
static int MimeInlineTextCalendar_parse_begin (MimeObject *obj);

extern int MK_OUT_OF_MEMORY;


static int
MimeInlineTextCalendarClassInitialize(MimeInlineTextCalendarClass *class)
{
  MimeObjectClass *oclass = (MimeObjectClass *) class;
  XP_ASSERT(!oclass->class_initialized);
  oclass->parse_begin = MimeInlineTextCalendar_parse_begin;
  oclass->parse_line  = MimeInlineTextCalendar_parse_line;
  oclass->parse_eof   = MimeInlineTextCalendar_parse_eof;

  return 0;
}

extern int mime_TranslateCalendar(char*, char**);

static int
MimeInlineTextCalendar_parse_begin(MimeObject *obj)
{
    MimeInlineTextCalendarClass *class;
    int status = ((MimeObjectClass*)&mimeLeafClass)->parse_begin(obj);
    if (status < 0) return status;

    if (!obj->output_p) return 0;
    if (!obj->options || !obj->options->write_html_p) return 0;

    /* This is a fine place to write out any HTML before the real meat begins.
       */

    class = ((MimeInlineTextCalendarClass *) obj->class);
    /* initialize buffered string to empty; */

    class->bufferlen = 0;
    class->buffermax = 512;
    class->buffer = (char*) XP_ALLOC(class->buffermax);
    if (class->buffer == NULL) return MK_OUT_OF_MEMORY;
    return 0;
}


static int
MimeInlineTextCalendar_parse_line(char *line, int32 length, MimeObject *obj)
{
    /* This routine gets fed each line of data, one at a time. We just buffer
       it all up, to be dealt with all at once at the end. */

    MimeInlineTextCalendarClass *class =
	((MimeInlineTextCalendarClass *) obj->class);
 
    if (!obj->output_p) return 0;
    if (!obj->options || !obj->options->output_fn) return 0;
    if (!obj->options->write_html_p) {
	return MimeObject_write(obj, line, length, TRUE);
    }

    if (class->bufferlen + length >= class->buffermax) {
	do {
	    class->buffermax += 512;
	} while (class->bufferlen + length >= class->buffermax);
	class->buffer = XP_REALLOC(class->buffer, class->buffermax);
	if (class->buffer == NULL) return MK_OUT_OF_MEMORY;
    }
    XP_MEMCPY(class->buffer + class->bufferlen, line, length);
    class->bufferlen += length;
}



#if 0

/* Temporary hack to test the send-a-file-of-mail hack. */

static int
TestMailHack(MimeDisplayOptions* opts)
{
    return MimeSendMessage(opts, "terry@netscape.com", "Here is a test hack",
		       "Content-Type: text/calendar",
		       "BEGIN:VCALENDAR\n\
PRODID:-//ACME/DesktopCalendar//EN\n\
METHOD:REQUEST\n\
VERSION:2.0\n\
BEGIN:VEVENT\n\
ATTENDEE;ROLE=OWNER;STATUS=ACCEPTED:MAILTO:\n\
 MAILTO:sman@netscape.com\n\
ATTENDEE;RSVP=YES;EXPECT=REQUEST:MAILTO:\n\
 MAILTO:stevesil@microsoft.com\n\
DTSTAMP:19970611T190000Z\n\
DTSTART:19970701T100000-0700\n\
DTEND:19970701T103000-0700\n\
SUMMARY:Phone Conference\n\
DESCRIPTION:Please review the attached document.\n\
UID:www.acme.com-873970198738777\n\
ATTACH:ftp://ftp.bar.com/pub/docs/foo.doc\n\
SEQUENCE:0\n\
STATUS:CONFIRMED\n\
END:VEVENT\n\
END:VCALENDAR");

}

#endif /* 0 */

#ifdef DEBUG_terry
void DoTestClick(void* closure, const char* url)
{
	char* tmp;
	tmp = PR_smprintf("Callback: %s (URL: %s)", (char*) closure, url);
	FE_Alert(XP_FindSomeContext(), tmp);
	XP_FREE(tmp);
}
#endif /* DEBUG_terry */

static int
MimeInlineTextCalendar_parse_eof (MimeObject *obj, XP_Bool abort_p)
{
    int status = 0;
    MimeInlineTextCalendarClass *class =
	((MimeInlineTextCalendarClass *) obj->class);
    char* html = NULL;

    if (obj->closed_p) return 0;

    /* Run parent method first, to flush out any buffered data. */
    status = ((MimeObjectClass*)&MIME_SUPERCLASS)->parse_eof(obj, abort_p);
    if (status < 0) return status;

    if (!class->buffer || class->bufferlen == 0) return 0;

    class->buffer[class->bufferlen] = '\0';
    
    status = mime_TranslateCalendar(class->buffer, &html);
    XP_FREE(class->buffer);
    class->buffer = NULL;
    if (status < 0) return status;

    status = MimeObject_write(obj, html, XP_STRLEN(html), TRUE);
    XP_FREE(html);
    if (status < 0) return status;

#ifdef DEBUG_terry
	/* Add an extra button, just to see if callback magic is working. */
	html = 
		PR_smprintf("<p><br><p><form action=\"%s\">"
					"<input type=hidden name=foo value=baz>"
					"<input type=submit value=\"Click me\">",
					NET_CallbackURLCreate(DoTestClick,
										  "Wow, you clicked it."));
	status = MimeObject_write(obj,  html, XP_STRLEN(html), TRUE);
    XP_FREE(html);
    if (status < 0) return status;
#endif /* DEBUG_terry */


    return 0;
}
