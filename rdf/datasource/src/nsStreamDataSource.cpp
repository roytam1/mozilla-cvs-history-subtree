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

  A data source that can read itself from and write itself to an
  RDF/XML stream.

  TO DO
  -----

  1) Right now, the only kind of stream data sources that are writable
     are "file:" URIs. (In fact, <em>all</em> "file:" URIs are
     writable, modulo flie system permissions; this may lead to some
     surprising behavior.) Eventually, it'd be great if we could open
     an arbitrary nsIOutputStream on *any* URL, and Netlib could just
     do the magic.

 */

