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

#ifndef _SAVEASDIALOG_H
#define _SAVEASDIALOG_H

#include <qfiledlg.h>

class QtContext;
class QRadioButton;

class SaveAsDialog : public QFileDialog
{
    Q_OBJECT

public:	
    enum SaveAsTypes { Text, Source, PostScript };

    SaveAsDialog( QtContext* cx, QWidget* parent, const char* name = 0 );
    
    SaveAsTypes type() const;

private slots:
     void radioClicked(int);
private:
    QtContext* context;
    QRadioButton* textRB;
    QRadioButton* sourceRB;
    QRadioButton* postscriptRB;
};


#endif
