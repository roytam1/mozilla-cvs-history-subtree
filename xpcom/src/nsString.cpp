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

#include "nsString.h"
#include "prlog.h"   // for PR_ASSERT
#include "prmem.h"
#include "plstr.h"

nsString::nsString(const char* cstr)
{
    fBuffer = PL_strdup(cstr);
}


nsString::nsString(const nsString& s)
{
    fBuffer = PL_strdup(s.fBuffer);
}


nsString::~nsString(void)
{
    if (fBuffer)
        PL_strfree(fBuffer);
}



const nsString&
nsString::operator =(const nsString& s)
{
    if (fBuffer)
        PL_strfree(fBuffer);

    fBuffer = PL_strdup(s.fBuffer);
    return *this;
}



const nsString&
nsString::operator =(const char* cstr)
{
    PR_ASSERT(cstr != NULL);

    if (fBuffer)
        PL_strfree(fBuffer);

    fBuffer = PL_strdup(cstr);
    return *this;
}



char&
nsString::operator[](PRUint32 index) const
{
    return CharAt(index);
}


char&
nsString::CharAt(PRUint32 index) const
{
    PR_ASSERT(index >= 0 && index < Length());
    return fBuffer[index];
}


const nsString&
nsString::operator +=(nsString& s)
{
    fBuffer = (char*) PR_REALLOC(fBuffer, Length() + s.Length() + 1);
    PR_ASSERT(fBuffer != NULL);

    PL_strcat(fBuffer, s.fBuffer);
    return *this;
}

const nsString&
nsString::operator +=(const char* cstr)
{
    PR_ASSERT(cstr != NULL);
    if (cstr == NULL)
        return *this;

    PRUint32 len = Length();
    len += PL_strlen(cstr);
    len += 1;

    fBuffer = (char*) PR_REALLOC(fBuffer, len);
    PR_ASSERT(fBuffer != NULL);

    PL_strcat(fBuffer, cstr);
    return *this;
}

const nsString&
nsString::operator +=(const char c)
{
    PRUint32 len = Length();

    fBuffer = (char*) PR_REALLOC(fBuffer, len + 2);
    PR_ASSERT(fBuffer != NULL);

    fBuffer[len]     = c;
    fBuffer[len + 1] = '\0';
    return *this;
}

nsString::operator char*(void) const
{
    return fBuffer;
}


char*
nsString::ToCString(char* buf, PRUint32 len) const
{
    if (Length() >= len)
        return NULL;

    PL_strcpy(buf, fBuffer);
    return buf;
}


char*
nsString::ToNewCString(void) const
{
    return PL_strdup(fBuffer);
}


PRUint32
nsString::Length(void) const
{
    return PL_strlen(fBuffer);
}


PRBool
nsString::Equals(nsString& s) const
{
    return PL_strcmp(fBuffer, s.fBuffer) == 0;
}


PRBool
nsString::Equals(const char* cstr) const
{
    PR_ASSERT(cstr != 0);
    return PL_strcmp(fBuffer, cstr) == 0;
}


PRBool
nsString::EqualsIgnoreCase(nsString& s) const
{
    return PL_strcasecmp(fBuffer, s.fBuffer) == 0;
}


PRBool
nsString::EqualsIgnoreCase(const char* cstr) const
{
    PR_ASSERT(cstr != 0);
    return PL_strcasecmp(fBuffer, cstr) == 0;
}


PRUint32
nsString::Find(char c, PRUint32 offset /* = 0 */) const
{
    PR_ASSERT(offset >= 0);
    if (offset >= Length())
        return NS_STRING_NO_INDEX;

    char* p = PL_strchr(fBuffer + offset, c);
    if (p == NULL)
        return NS_STRING_NO_INDEX;

    return (PRUint32) (p - fBuffer);
}



PRUint32
nsString::RFind(char c) const
{
    char* p = PL_strrchr(fBuffer, c);
    if (p == NULL)
        return NS_STRING_NO_INDEX;

    return (PRUint32) (p - fBuffer);
}


const nsString&
nsString::Cut(PRUint32 start, PRUint32 count)
{
    if (start < 0)
        return *this;

    if (start + count > Length())
        return *this;

    char* buffer = (char*) PR_MALLOC(count + 1);
    PL_strncpy(buffer, fBuffer + start, count);
    buffer[count] = '\0';

    PL_strfree(fBuffer);
    fBuffer = buffer;

    return *this;
}


PRUint32
nsString::Split(nsString* result[], char splitter) const
{
    PRUint32 count = 0;
    PRUint32 offset = 0;
    while ((offset = Find(splitter, offset)) != NS_STRING_NO_INDEX) {
        ++offset;
        ++count;
    }

    if (count == 0) {
        (*result) = new nsString[1];
        (*result)[0] = fBuffer; // The _entire_ string...
        return 1;
    }

    (*result) = new nsString[count];

    count = offset = 0;
    while (1) {
        PRUint32 end = Find(splitter, offset);
        if (end == NS_STRING_NO_INDEX)
            break;

        PRUint32 len = end - offset;

        nsString& s = (*result)[count];
        s.fBuffer = (char*) PR_MALLOC(len + 1);
        PL_strncpy(s.fBuffer, fBuffer + offset, len);
        s.fBuffer[len] = '\0';

        offset = end + 1;
        ++count;
    }

    return count;
}
