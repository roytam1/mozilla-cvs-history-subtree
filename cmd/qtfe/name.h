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

#define QTFE_NAME      QtMozilla
#define QTFE_PROGNAME  qtMozilla
#define QTFE_PROGCLASS QtMozilla
#define QTFE_LEGALESE "(c) 1998 Troll Tech AS"

/* I don't pretend to understand this. */
#define cpp_stringify_noop_helper(x)#x
#define cpp_stringify(x) cpp_stringify_noop_helper(x)

#define QTFE_NAME_STRING      cpp_stringify(QTFE_NAME)
#define QTFE_PROGNAME_STRING  cpp_stringify(QTFE_PROGNAME)
#define QTFE_PROGCLASS_STRING cpp_stringify(QTFE_PROGCLASS)
