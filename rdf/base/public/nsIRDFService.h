/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

/*

  The RDF service interface. This is a singleton object, and should be
  obtained from the <tt>nsServiceManager</tt>.

  XXX I'm not particularly happy with the current APIs for named data
  sources. The idea is that you want "rdf:bookmarks" to always map to
  the "bookmarks" data store. The thing that bothers me now is that
  the implementation has to pre-load all of the data sources: that
  means you need to parse the bookmarks file, read the history, etc.,
  rather than doing this on-demand. With a little thought, it may be
  able to enormously improve these APIs so that pre-loading data
  sources is unnecessary.

 */

#ifndef nsIRDFService_h__
#define nsIRDFService_h__
#include "nsRDFInterfaces.h"

extern nsresult
NS_NewRDFService(nsIRDFService** result);

#endif // nsIRDFService_h__
