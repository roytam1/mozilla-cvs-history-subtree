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

#ifndef nsTime_h__
#define nsTime_h__

#include "prtime.h"

/**
 * This class encapsulates full 64-bit time functionality and
 * provides simple arithmetic and conversion operations.
 */
class nsTime
{
private:
    PRTime fValue;

public:
    nsTime(void);
    nsTime(const PRTime t);
    nsTime(const nsTime& t);
    const nsTime& operator =(const nsTime& t);
    const nsTime& operator =(const PRTime t);

    const nsTime& operator -=(const nsTime& t);
    const nsTime& operator -=(const PRTime t);

    const nsTime& operator +=(const nsTime& t);
    const nsTime& operator +=(const PRTime t);

    PRUint32 ToMSec(void);

    friend const nsTime operator +(const nsTime& t1, const nsTime& t2);
    friend const nsTime operator -(const nsTime& t1, const nsTime& t2);

    static const nsTime FromMSec(PRUint32 msec);
};


#endif // nsTime_h__
