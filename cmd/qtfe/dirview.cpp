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

#include "dirview.h"
#include <qdir.h>
#include <qfile.h>
#include <qfileinf.h>

RCSTAG("$Id$");


Directory::Directory( Directory * parent, const char * filename )
    : QListViewItem( parent ), f(filename )
{
    p = parent;
    readable = TRUE;
}


Directory::Directory( QListView * parent )
    : QListViewItem( parent ), f("/")
{
    p = 0;
    readable = TRUE;
}


void Directory::setOpen( bool o )
{
    if ( o && !childCount() ) {
	QString s( fullName() );
	QDir thisDir( s );
	if ( !thisDir.isReadable() ) {
	    readable = FALSE;
	    return;
	}

	const QFileInfoList * files = thisDir.entryInfoList();
	if ( files ) {
	    QFileInfoListIterator it( *files );
	    QFileInfo * f;
	    while( (f=it.current()) != 0 ) {
		++it;
		if ( f->fileName() == "." || f->fileName() == ".." )
		    ; // nothing
		else if ( f->isSymLink() )
		    new QListViewItem( this, (const char *)f->fileName(),
				       "Symbolic Link", 0 );
		else if ( f->isDir() )
		    new Directory( this, f->fileName() );
		else
		    new QListViewItem( this, (const char *)f->fileName(),
				       f->isFile() ? "File" : "Special", 0 );
	    }
	}
    }
    QListViewItem::setOpen( o );
}


void Directory::setup()
{
    setExpandable( TRUE );
    QListViewItem::setup();
}


QString Directory::fullName()
{
    QString s;
    if ( p ) {
	s = p->fullName();
	s.append( f.name() );
	s.append( "/" );
    } else {
	s = "/";
    }
    return s;
}


const char * Directory::text( int column ) const
{
    if ( column == 0 )
	return f.name();
    else if ( readable )
	return "Directory";
    else
	return "Unreadable Directory";
}
