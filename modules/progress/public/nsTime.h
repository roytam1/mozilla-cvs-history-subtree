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

#include "prtime.h"

class nsTime
{
private:
    PRTime fValue;

public:
    nsTime(void);
    nsTime(PRTime t);
    nsTime(nsTime& t);
    nsTime& operator =(nsTime& t);
    nsTime& operator =(PRTime t);

    nsTime& operator -=(nsTime& t);
    nsTime& operator -=(PRTime t);

    nsTime& operator +=(nsTime& t);
    nsTime& operator +=(PRTime t);

    PRUint32 ToMSec(void);

    friend nsTime operator +(nsTime& t1, nsTime& t2);
    friend nsTime operator -(nsTime& t1, nsTime& t2);

    static nsTime FromMSec(PRUint32 msec);
};
