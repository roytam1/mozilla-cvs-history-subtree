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
 * The Initial Developer of this code under the NPL is Arnt
 * Gulbrandsen.  Portions created by Warwick Allison, Kalle Dalheimer, 
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

#ifndef TOOLBARS_H
#define TOOLBARS_H

#include <qtoolbar.h>
#include <qmainwindow.h>
#include <qtoolbutton.h>
#include "icons.h"


class QMovie;
class QtContext;
class QLabel;
class QWidgetStack;
class QProgressBar;
class QComboBox;


class UnknownProgress: public QWidget
{
    Q_OBJECT
public:
    UnknownProgress( QWidget * parent, const char * name );

public slots:
    void progress();
    void done();

protected:
    void paintEvent( QPaintEvent * );
    void resizeEvent( QResizeEvent * );

private:
    int x;
    int d;
    QTimer * t;
};


class MovieToolButton: public QToolButton
{
  Q_OBJECT
public:
    MovieToolButton( QToolBar * parent, const QMovie &, const Pixmap & );
    ~MovieToolButton();

    QSize sizeHint() const;

protected:
    void drawButtonLabel( QPainter * );

public slots:
    void start();
    void stop();

private slots:
    void movieUpdated(const QRect&);
    void movieResized(const QSize&);

private:
    QMovie * m;
    QPixmap pm;
};


class Toolbars: public QObject
{
    Q_OBJECT
public:
    Toolbars( QtContext* context, QMainWindow * parent );
    ~Toolbars();

public slots:
    void setBackButtonEnabled( bool );
    void setForwardButtonEnabled( bool );
    void setLoadImagesButtonEnabled( bool );
    void setStopButtonEnabled( bool );

    void setupProgress( int );
    void signalProgress( int );
    void signalProgress();
    void endProgress();

    void setMessage( const char * );
    void setComboText( const char * );

signals:
    void openURL( const char * );

private slots:
    void visitMozilla();
    void hideProgressBars();

private:
    QToolBar * navigation;
    QToolBar * location;
    QToolBar * personal;
    QLabel * security;

    QToolButton * back, * forward, * stop, * images;
    QComboBox * url;

    QWidgetStack * progressBars;
    QProgressBar * knownProgress;
    UnknownProgress * unknownProgress;
    QLabel * messages;
    QStatusBar * status;

    MovieToolButton * animation;

    bool progressAsPercent;

    QTimer * t;
    QTimer * t2;
};


#endif
