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
 * The Initial Developer of this code under the NPL is Jonas
 * Utterstrom.  Portions created by Jonas Utterstrom are
 * Copyright (C) 1998 Jonas Utterstrom.  All 
 * Rights Reserved.
 * 
 * Contributors: Jonas Utterstrom <jonas.utterstrom@vittran.norrnod.se>
 */

#ifndef QTHISTORYCONTEXT_H
#define QTHISTORYCONTEXT_H

#include "QtContext.h"
#include <qpopmenu.h>
#include <qmainwindow.h>

class  QMainWindow;
class  QListView;
class  QListViewItem;
class  QtBrowserContext;
class  HistoryListView;

class QtHistoryContext : public QtContext {
/* A History window */
    Q_OBJECT
public:
    QtHistoryContext( MWContext * );
    virtual ~QtHistoryContext();

    static QtHistoryContext *qt( MWContext *c );
    static QtHistoryContext *ptr();

    void show();

public slots:

    void cmdOpenBrowser();
    void cmdComposeMessage();
    void cmdNewBlank();
    void cmdNewTemplate();
    void cmdNewWizard();
    void cmdAddBookmark();


    
    void cmdSaveAs();
    void cmdGotoPage();
    void cmdOpenSelected();
    void cmdOpenAddToToolbar();
    void cmdClose();
    void cmdExit();

    void cmdUndo();
    void cmdRedo();
    void cmdCut();
    void cmdCopy();
    void cmdPaste();
    void cmdDelete();

    void cmdSortByTitle();
    void cmdSortByLocation();
    void cmdSortByDateLastVisited();
    void cmdSortByDateFirstVisited();
    void cmdSortByExpirationDate();
    void cmdSortByVisitCount();
    void cmdSortAscending();
    void cmdSortDescending();
    
    void cmdAboutMozilla();
    void cmdAboutQt();
    void cmdOpenHistory();
    void cmdAddToToolbar();
    void cmdOpenNavCenter();
    void cmdOpenOrBringUpBrowser();
    void cmdOpenEditor();


private slots:
    void activateItem( QListViewItem * );
    void rightClick( QListViewItem *, const QPoint &, int );
    void newSelected( QListViewItem * );
    void sortColumnChanged(int column, bool ascending);

private:

    void changeSorting( int column, bool ascending);
    void createWidget();
    void invalidateListView();
    void gotoEntry (const char* url, const char* target);
    static int notifyHistory(gh_NotifyMsg *msg);
    void populateMenuBar(QMenuBar* menuBar); // implemented in menu.cpp
    void  refreshHistory();
    void  refreshCells(int first, int last);
    gh_SortColumn togh_SortColumn(int column) const;
    void  setColumnText(QListViewItem *item, int col, gh_HistEntry *e);
    void  setHistoryRow(QListViewItem *item, gh_HistEntry *gh_entry);
    

    HistoryListView      *listView;
    QMainWindow	         *historyWidget;
    int                   sortColumn;
    bool                  sortAscending;
    bool                  inBatch;
    GHHANDLE              m_histCursor;
};


#endif
