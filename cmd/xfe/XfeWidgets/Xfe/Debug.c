/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is the Xfe Widgets.
 * 
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

/*----------------------------------------------------------------------*/
/*																		*/
/* Name:		<Xfe/Debug.c>											*/
/* Description:	Xfe widgets functions for debugging.					*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifdef DEBUG									/* ifdef DEBUG			*/

#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>

#include <Xfe/XfeP.h>

#include <Xfe/PrimitiveP.h>
#include <Xfe/ManagerP.h>

#include <Xm/RepType.h>

#define DEBUG_BUFFER_SIZE 2048

/*
 * This memory will leak, but only in debug builds.
 */
static String
DebugGetBuffer(void)
{
	static String _debug_buffer = NULL;

	if (!_debug_buffer)
	{
		_debug_buffer = (String) XtMalloc(sizeof(char) * DEBUG_BUFFER_SIZE);
	}

	assert( _debug_buffer != NULL );

	return _debug_buffer;
}

/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/* extern */ String
XfeDebugXmStringToStaticPSZ(XmString xmstr)
{
	String		result = DebugGetBuffer();
	String		psz_string;

	result[0] = '\0';

    assert( xmstr != NULL );

	if (xmstr != NULL)
	{
		psz_string = XfeXmStringGetPSZ(xmstr,XmFONTLIST_DEFAULT_TAG);

		if (psz_string != NULL)
		{
			strcpy(result,psz_string);

			XtFree(psz_string);
		}
	}

	return result;
}
/*----------------------------------------------------------------------*/
/* extern */ void
XfeDebugPrintArgVector(FILE *		fp,
					   String		prefix,
					   String		suffix,
					   ArgList		av,
					   Cardinal		ac)
{
	Cardinal i;

	for(i = 0; i < ac; i++)
	{
		fprintf(fp,"%s: av[%d] = '%s'%s",
				prefix,
				i,
				av[i].name,
				suffix);
	}
}
/*----------------------------------------------------------------------*/
/* extern */ String
XfeDebugRepTypeValueToName(String rep_type,unsigned char value)
{
	String		result = DebugGetBuffer();
	Boolean		found = False;

	if (rep_type)
	{
		XmRepTypeId id = XmRepTypeGetId(rep_type);

		if (id != XmREP_TYPE_INVALID)
		{
			XmRepTypeEntry entry = XmRepTypeGetRecord(id);

			if (entry)
			{
				Cardinal i = 0;

				while ((i < entry->num_values) && !found)
				{
					if (entry->values[i] == value)
					{
						found = True;

						strcpy(result,entry->value_names[i]);
					}

					i++;
				}

 				XtFree((char *) entry);
			}
		}
	}
	
	return result;
}
/*----------------------------------------------------------------------*/
/* extern */ unsigned char
XfeDebugRepTypeNameToValue(String rep_type,String name)
{
	unsigned char result = 99;
	Boolean found = False;

	if (rep_type)
	{
		XmRepTypeId id = XmRepTypeGetId(rep_type);

		if (id != XmREP_TYPE_INVALID)
		{
			XmRepTypeEntry entry = XmRepTypeGetRecord(id);

			if (entry)
			{
				Cardinal i = 0;

				while ((i < entry->num_values) && !found)
				{
					if (strcmp(entry->value_names[i],name) == 0)
					{
						result = entry->values[i];

						found = True;
					}

					i++;
				}

 				XtFree((char *) entry);
			}
		}
	}
	
	return result;
}
/*----------------------------------------------------------------------*/
/* extern */ unsigned char
XfeDebugRepTypeIndexToValue(String rep_type,Cardinal i)
{
	unsigned char result = 99;

	if (rep_type)
	{
		XmRepTypeId id = XmRepTypeGetId(rep_type);

		if (id != XmREP_TYPE_INVALID)
		{
			XmRepTypeEntry entry = XmRepTypeGetRecord(id);

			if (entry)
			{
				result = entry->values[i];

 				XtFree((char *) entry);
			}
		}
	}
	
	return result;
}
/*----------------------------------------------------------------------*/
/* extern */ String
XfeDebugGetStaticWidgetString(Widget w,String name)
{
	String		str = NULL;
	XmString	xmstr = (XmString) XfeGetValue(w,name);

    assert( xmstr != NULL );

	if (xmstr != NULL)
	{
		str = XfeDebugXmStringToStaticPSZ(xmstr);
		
		XmStringFree(xmstr);
	}

	return str;
}
/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/*																		*/
/* The following XfeDebugWidgets*() API allows for a list of 'debug'	*/
/* widget to be maintained.  Extra debugging info can then be printed	*/
/* for such widgets.													*/
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ Boolean
XfeDebugIsEnabled(Widget w)
{
	Boolean result = False;

	assert( XfeIsManager(w) || XfeIsPrimitive(w) );
	
	if (XfeIsPrimitive(w))
	{
		result = _XfeDebugTrace(w);
	}
	else
	{
		result = _XfemDebugTrace(w);
	}

	return result;
}
/*----------------------------------------------------------------------*/
/* extern */ void
XfeDebugPrintf(Widget w,char * format, ...)
{
	if (XfeDebugIsEnabled(w))
	{
		va_list arglist;

		va_start(arglist,format);
		
		vprintf(format,arglist);
		
		va_end(arglist);
	}
}
/*----------------------------------------------------------------------*/
/* extern */ void
XfeDebugPrintfFunction(Widget w,char * func_name,char * format, ...)
{
	if (XfeDebugIsEnabled(w))
	{
		assert( func_name != NULL );

		printf("%s(%s",func_name,XtName(w));

		if (format != NULL)
		{
			va_list arglist;
			
			va_start(arglist,format);
			
			vprintf(format,arglist);
			
			va_end(arglist);
		}

		printf(")\n");
	}
}
/*----------------------------------------------------------------------*/

#endif											/* endif DEBUG			*/
