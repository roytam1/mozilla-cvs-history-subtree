/* $Id$
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
 * The Initial Developer of this code under the NPL is Eirik
 * Eng.  Portions created by Warwick Allison, Kalle Dalheimer, 
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

#ifndef QTBOOKMARKSCONTEXT_H
#define QTBOOKMARKSCONTEXT_H

#include "QtContext.h"
#include <qpopmenu.h>
#include <qmainwindow.h>

class  QMainWindow;
class  QListView;
class  QListViewItem;

class  QtBrowserContext;
struct QtBMClipboard;
struct QtBookmarkEditDialog;
struct BM_FindInfo;

class QtBookmarksContext : public QtContext {
/* A Bookmark window */
    Q_OBJECT
public:
    virtual ~QtBookmarksContext();

    static QtBookmarksContext *qt( MWContext *c );

    static QtBookmarksContext *ptr();
    BM_Entry *rootEntry();

public slots:
    void cmdEditBookmarks();

    void cmdNewBookmark();
    void cmdNewFolder();
    void cmdNewSeparator();
    void cmdOpenBookmarkFile();
    void cmdImport();
    void cmdSaveAs();
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
    void cmdSelectAll();
    void cmdFindInObject();
    void cmdFindAgain();
    void cmdSearch();
    void cmdSearchAddress();
    void cmdBookmarkProperties();

    void cmdSortByTitle();
    void cmdSortByLocation();
    void cmdSortByDateLastVisited();
    void cmdSortByDateCreated();
    void cmdSortAscending();
    void cmdSortDescending();
    void cmdBookmarkUp();
    void cmdBookmarkDown();
    void cmdBookmarkWhatsNew();
    void cmdSetToolbarFolder();
    void cmdSetNewBookmarkFolder();
    void cmdSetBookmarkMenuFolder();

public:
    // ***** BMFE_ funcs:

    void  refreshCells ( int32 first, int32 last, bool now );
    void  syncDisplay ();
    void  measureEntry ( BM_Entry* entry, uint32* width, uint32* height );
    void  setClipContents ( void* buffer, int32 length );
    void* getClipContents ( int32* length );
    void  freeClipContents ();
    void  openBookmarksWindow ();
    void  editItem ( BM_Entry* entry );
    void  entryGoingAway ( BM_Entry* entry );
    void  gotoBookmark ( const char* url, const char* target );
    void* openFindWindow ( BM_FindInfo* findInfo );
    void  scrollIntoView ( BM_Entry* entry );
    void  bookmarkMenuInvalid ();
    void  updateWhatsChanged ( const char* url, int32 done, int32 total,
			       const char* totaltime );
    void  finishedWhatsChanged ( int32 totalchecked,
				 int32 numreached, int32 numchanged );
    void  startBatch ();
    void  endBatch ();

    // ***** End BMFE_ funcs:

private slots:
    void activateItem( QListViewItem * );
    void rightClick( QListViewItem *, const QPoint &, int );
    void newSelected( QListViewItem * );

private:
    QtBookmarksContext( MWContext * );

    void changeSorting( int column, bool ascending );
    void createWidget();
    void invalidateListView();
    void createEditDialog();
    void initBMData();
    bool loadBookmarks( const char *fileName = 0 );
    BM_Entry *currentEntry();
    bool bmSelectHighlighted();
    void editNewEntry( BM_Entry* );

    // this method is not implemented in QtBrowserContext.cpp, but in
    // menus.cpp
    void populateMenuBar( QMenuBar* );

    QListView            *listView;
    QMainWindow	         *bookmarkWidget;
    QtBookmarkEditDialog *editDialog;
    QtBMClipboard        *clip;
    int	                  sortColumn;
    bool                  sortAscending;
    bool                  bmDataLoaded;
    bool                  inBatch;
};

enum  QtBMItemType { QtBMFolderItem, QtBMBookmarkItem };

QPixmap *getBMPixmap( QtBMItemType type, bool isOpen );

#endif
