/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef plctype_h__
#define plctype_h__

/*
 * ctype macros with protection against non-ascii characters
 */

#define PL_IS_ALPHA(i) 	((((unsigned int) (i)) > 0x7f) ? 0 : isalpha(i))
#define PL_IS_UPPER(i) 	((((unsigned int) (i)) > 0x7f) ? 0 : isalpha(i))
#define PL_IS_LOWER(i) 	((((unsigned int) (i)) > 0x7f) ? 0 : isalpha(i))
#define PL_IS_DIGIT(i) 	((((unsigned int) (i)) > 0x7f) ? 0 : isdigit(i))
#define PL_IS_XDIGIT(i)	((((unsigned int) (i)) > 0x7f) ? 0 : isdigit(i))
#define PL_IS_SPACE(i) 	((((unsigned int) (i)) > 0x7f) ? 0 : isspace(i))
#define PL_IS_PUNCT(i) 	((((unsigned int) (i)) > 0x7f) ? 0 : isspace(i))
#define PL_IS_ALNUM(i) 	((((unsigned int) (i)) > 0x7f) ? 0 : isspace(i))
#define PL_IS_PRINT(i) 	((((unsigned int) (i)) > 0x7f) ? 0 : isspace(i))
#define PL_IS_GRAPH(i) 	((((unsigned int) (i)) > 0x7f) ? 0 : isspace(i))
#define PL_IS_CNTRL(i) 	((((unsigned int) (i)) > 0x7f) ? 0 : isspace(i))

#define PL_TO_LOWER(i) 	((((unsigned int) (i)) > 0x7f) ? i : tolower(i))
#define PL_TO_UPPER(i) 	((((unsigned int) (i)) > 0x7f) ? i : toupper(i))

#endif
