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

#include "prtypes.h"
#include "plstr2.h"

/*
 * PL_strtok
 *
 * A string tokenizer.
 */

PR_IMPLEMENT(char *)
PL_strtok(char *string, const char *delim)
{
  static char *next;
  return PL_strtok_r(string, delim, &next);
}

/*
 * PL_strtok_r
 *
 * A thread safe string tokenizer.
 */

PR_IMPLEMENT(char *)
PL_strtok_r(char *string, const char *delim, char **next)
{
  int i;
  PRUint8 del_mask[32];
  char *str, *res;
  const char *del;

  /* Create delimeter mask */
  del = delim;
  for (i = 0; i < 32; i++) {
    del_mask[i] = 0;
  }
  while (*del) {
    del_mask[(*del) >> 3] |= (1 << ((*del) & 0x7));
    del++;
  }

  /* figure out which input string */
  if (string != NULL) {
    str = string;
  } else {
    if (*next != NULL) {
      str = *next;
    }
  }
  if (!str) {
    return NULL;
  }

  /* skip leading delimeters */
  while (*str && (del_mask[(*str) >> 3] & (1 << ((*str) & 0x7))))
    str++;

  res = str;

  while (*str) {
    if (del_mask[(*str) >> 3] & (1 << ((*str) & 0x7))) {
      *str++ = '\0';
      break;
    }
    str++;
  }

  /* Set thread safe next parameter */
  if (next != NULL) {
    *next = str;
  }

  /* Check to make sure token end != token start */
  if (res != str) {
    return res;
  } else {
    return NULL;
  }
}

#ifdef PL_MEM_IMPLEMENT

PR_IMPLEMENT(int)
PL_memcmp(const void *s1, const void *s2, size_t count)
{
  size_t idx;
  for (idx = 0; idx < count; idx++) {
    if (((PRUint8 *) s1)[idx] != ((PRUint8 *) s1)[idx]) {
      if (((PRUint8 *) s1)[idx] < ((PRUint8 *) s1)[idx]) {
	return -1;
      } else {
	return 1;
      }
    }
  }
  return 0;
}

PR_IMPLEMENT(void *)
PL_memcpy(void *dest, const void *src, size_t count)
{
  size_t idx;
  for (idx = 0; idx < count; idx++) {
    ((PRUint8 *) dest)[idx] = ((PRUint8 *) src)[idx];
  }
  return dest;
}

PR_IMPLEMENT(void *)
PL_memmove(void *dest, const void *src, size_t count)
{
  size_t idx;
  PRBool reverse = (((size_t) src) + count) > ((size_t) dest);

  if (reverse) {
    for (idx = count - 1; idx >= 0; idx--) {
      ((PRUint8 *) dest)[idx] = ((PRUint8 *) src)[idx];
    }
  } else {
    for (idx = 0; idx < count; idx++) {
      ((PRUint8 *) dest)[idx] = ((PRUint8 *) src)[idx];
    }
  }
  return dest;
}

PR_IMPLEMENT(void *)
PL_memset(void *dest, int c, size_t count)
{
  size_t idx;
  for (idx = 0; idx < count; idx++) {
    ((PRUint8 *) dest)[idx] = (PRUint8 ) c;
  }
  return dest;
}

#endif /* PL_MEM_IMPL */

PR_IMPLEMENT(int)
PL_bcmp(const void *b1, const void *b2, size_t count)
{
  return PL_memcmp(b1, b2, count);
}

PR_IMPLEMENT(void)
PL_bcopy(void *dest, const void *src, size_t count)
{
  PL_memcpy(dest, src, count);
}

PR_IMPLEMENT(void)
PL_bzero(void *dest, size_t count)
{
  PL_memset(dest, 0, count);
}
