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

#include "QtContext.h"
#include "shist.h"


static void useArgs( const char *fn, ... )
{
    if (0&&fn) printf( "%s\n", fn );
}  

/* From ./lay.c: */
void QtContext::layoutNewDocument(URL_Struct *url,
		      int32 *iWidth, int32 *iHeight,
		       int32 *mWidth, int32 *mHeight)
{
  *iWidth = visibleWidth();
  *iHeight = visibleHeight();

  // crazy defaults...... inspired by the xfe
  if (mwContext()->is_grid_cell){
    *mWidth = 7;
    *mHeight = 4;
  } else {
    *mWidth = 8;
    *mHeight = 8;
  }

  //  fprintf( stderr, "QTFE_LayoutNewDocument %p, %dx%d, %d,%d \n", 
  //	  url, *iWidth, *iHeight, *mWidth, *mHeight );
  
  SHIST_AddDocument( context, SHIST_CreateHistoryEntry( url, "" ) );
}

/* From ./lay.c: */
void QtContext::setDocTitle(char *title)
{
    useArgs( "QTFE_SetDocTitle %s \n", title );
}

/* From ./lay.c: */
void QtContext::finishedLayout()
{
    useArgs( "QTFE_FinishedLayout  \n" );
}

/* From ./lay.c: */
void QtContext::beginPreSection()
{
    useArgs( "QTFE_BeginPreSection  \n" );
}

/* From ./lay.c: */
void QtContext::endPreSection()
{
    useArgs( "QTFE_EndPreSection  \n" );
}
