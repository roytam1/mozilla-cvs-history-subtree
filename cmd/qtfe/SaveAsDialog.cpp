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
 * The Initial Developer of this code under the NPL is Kalle
 * Dalheimer.  Portions created by Warwick Allison, Kalle Dalheimer, 
 * Eirik Eng, Matthias Ettrich, Arnt Gulbrandsen, Haavard Nord and 
 * Paul Olav Tvete are Copyright (C) 1998 Warwick Allison, Kalle Dalheimer,
 * Eirik Eng, Matthias Ettrich, Arnt Gulbrandsen, Haavard Nord, and
 * Paul Olav Tvete. All Rights Reserved.
 *
 * Contributors: Warwick Allison
 *               Kalle Dalheimer
 *               Eirik Eng
 *               Matthias Ettrich
 *               Arnt Gulbrandsen
 *               Haavard Nord
 *               Paul Olav Tvete
 */

#include "SaveAsDialog.h"
#include "QtContext.h"

#include <qlabel.h>
#include <qpushbt.h>
#include <qradiobt.h>
#include <qtbuttonrow.h>
#include <qbttngrp.h>

SaveAsDialog::SaveAsDialog( QtContext* cx, QWidget* parent,
			    const char* name ) :
    QFileDialog( parent, name, true ),
    context( cx )
{
    QLabel* label = new QLabel( tr( "Save As" ), this );

    QButtonGroup* bg = new QButtonGroup( this );
    bg->hide();
    QtButtonRow* br = new QtButtonRow( this );
    textRB = new QRadioButton( tr( "Text" ), br );
    bg->insert( textRB );
    sourceRB = new QRadioButton( tr( "Source" ), br );
    bg->insert( sourceRB );
    postscriptRB = new QRadioButton( tr( "PostScript" ), br );
    bg->insert( postscriptRB );

    textRB->setChecked( false );
    sourceRB->setChecked( true );
    postscriptRB->setChecked( false );

    // connect
    connect( bg, SIGNAL(clicked(int)), SLOT(radioClicked(int)) );

    addWidgets( label, br, 0 );

    setCaption( tr( "Save As" ) );
}


SaveAsDialog::SaveAsTypes SaveAsDialog::type() const
{
    if( textRB->isChecked() )
	return Text;
    else if( sourceRB->isChecked() )
	return Source;
    else
	return PostScript;
}

void SaveAsDialog::radioClicked(int id)
{
   int index;
   static const char* extensions[] =   {
      ".txt",
      ".html",
      ".ps"
   };
   QString path;
   QString selstr  = selectedFile();
   if (!selstr.isNull() && !selstr.isEmpty())
   {
      index = selstr.findRev('/');
      if (index!=-1)
      {
         QString path = selstr.left(index);
         if (strcmp(path, dirPath())!=0)
            setDir(path);
         selstr.remove(0, index+1);
      }
      index = selstr.findRev('.');
      if (index!=-1)
         selstr.replace(index, selstr.length()-index, extensions[id]);
      setSelection(selstr);
   }
}
