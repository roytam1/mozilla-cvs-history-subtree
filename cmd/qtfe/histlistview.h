/*-*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *  $Id$
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
 * The Initial Developer of this code under the NPL is Jonas
 * Utterstrom.  Portions created by Jonas Utterstrom are
 * Copyright (C) 1998 Jonas Utterstrom.  All 
 * Rights Reserved.
 * 
 * Contributors: Jonas Utterstrom <jonas.utterstrom@vittran.norrnod.se>
 */

#ifndef _HIST_LISTVIEW_H_
#define _HIST_LISTVIEW_H_

#include <qlistview.h>

class HistoryListView : public QListView
{
    Q_OBJECT
    public:
        HistoryListView(QWidget *parent, const char* title);
        virtual void setSorting(int column, bool increasing = true);

    signals:
        void sortColumnChanged(int, bool);
};

#endif
