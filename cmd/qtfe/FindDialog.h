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

#ifndef _FINDDIALOG_H
#define _FINDDIALOG_H

#include <qdialog.h>

class QtContext;
class QLineEdit;
class QCheckBox;

class FindDialog : public QDialog
{
  Q_OBJECT

public:
  FindDialog( QtContext* cx, QWidget* parent,
	      const char* name = 0 );

public slots:
  void find();

private slots:
  void clear();

private:
  void refresh();

  QtContext* context;
  QLineEdit* edit;
  QCheckBox* sensitiveCB;
  QCheckBox* backwardsCB;
};

#endif
