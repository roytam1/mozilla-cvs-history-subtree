/* -*- C -*-
 *
 * Copyright (c) 1998-1999 Innosoft International, Inc.  All Rights Reserved.
 *
 * Acquisition and use of this software and related materials for any 
 * purpose requires a written license agreement from Innosoft International, 
 * Inc. or a written license from an organization licensed by Innosoft
 * International, Inc. to grant such a license.
 *
 *
 * Copyright (c) 1996-1997 Critical Angle Inc. All Rights Reserved.
 * 
 * Acquisition and use of this software and related materials for any 
 * purpose requires a written license agreement from Critical Angle Inc.,
 * or a written license from an organization licensed by Critical Angle
 * Inc. to grant such a license.
 *
 */

/*
 * $RCSfile$ $Revision$ $Date$ $State$
 *
 * $Log$
 * Revision 1.7  1999/03/22 23:58:22  Administrator
 * update Copyright statements in ILC-SDK
 *
 * Revision 1.6  1998/10/14 22:10:32  wahl
 * A9810084A call ldap_error_memory if malloc,calloc,realloc fail
 *
 * Revision 1.5  1998/04/23 16:42:50  wahl
 * A9804174A modify length calculation
 *
 * Revision 1.4  1998/03/06 21:33:30  wahl
 * A9803064B osf1 changes
 *
 * Revision 1.3  1998/01/16 21:48:32  wahl
 * convert European character cases
 *
 * Revision 1.2  1998/01/15 02:18:42  nr
 * add utf8 processing functions
 *
 * Revision 1.1  1997/10/29 19:35:59  wahl
 * add utf8 transformations
 *
 *
 */

#include "ldap-int.h"

unsigned char *ldap_charset_unicode_to_utf8 (
	unsigned short *src,
	int sl
)
{
  int i,j,l;
  unsigned char *dst;
  
  for (i = 0,l = 0; i < sl; i++) {
    unsigned long c = src[i];
    if (c >= 0xD800L && c <= 0xDBFFL && (i+1)<sl) {
      unsigned long c2 = src[i+1];
      if (c2 >= 0xDC00L && c2 <= 0xDFFFL) {
	c = ((c - 0xD800L) << 10)
	  + (c2 - 0xDC00L) + 0x0010000L;
	i++;
      }
    }
    
    if (c < 0x80) {
      l++;
    } else if (c < 0x000800L) {
      l+=2;
    } else if (c < 0x010000L) {
      l+=3;
    } else if (c < 0x200000L) {
      l+=4;
    } else {
      /* not Unicode */
      return NULL;
    }
  }
  l++;
  
  dst = (unsigned char *)malloc(l);
  
  if (dst == NULL) {
    return NULL;
  }
  
  for (i = 0,j = 0; i < sl; i++) {
    unsigned long c = src[i];
    
    if (c >= 0xD800L && c <= 0xDBFFL && (i+1)<sl) {
      unsigned long c2 = src[i+1];
      if (c2 >= 0xDC00L && c2 <= 0xDFFFL) {
	c = ((c - 0xD800L) << 10)
	  + (c2 - 0xDC00L) + 0x0010000L;
	i++;
      }
    }
    
    if (c < 0x80) {
      dst[j] = c;
      j++;
    } else if (c < 0x000800L) {
      dst[j+1] = (c | 0x80) & 0xBF; c >>= 6;
      dst[j] = c | 0xC0;
      j+=2;
    } else if (c < 0x010000L) {
      dst[j+2] = (c | 0x80) & 0xBF; c >>= 6;
      dst[j+1] = (c | 0x80) & 0xBF; c >>= 6;
      dst[j] = c | 0xE0;
      j+=3;
    } else if (c < 0x200000L) {
      dst[j+3] = (c | 0x80) & 0xBF; c >>= 6;
      dst[j+2] = (c | 0x80) & 0xBF; c >>= 6;
      dst[j+1] = (c | 0x80) & 0xBF; c >>= 6;
      dst[j] = c | 0xF0;
      j+=4;
    }
  }
  
  dst[j] = '\0';
  return dst;
}
 
/* ================================================================ */

unsigned short *ldap_charset_utf8_to_unicode (
 	unsigned char *src
)
{
  int l = 0,i;
  unsigned short *dst;

  if (src == NULL) return NULL;
  
  for (i = 0,l = 0; src[i] != '\0'; i++,l++) {
    unsigned long ch0 = *src;
    unsigned short cl = 0;
    
    if (ch0 >= 252) {
      cl = 5;
    } else if (ch0 >= 248) {
      cl = 4;
      l++;  /* in case of surrogates */
    } else if (ch0 >= 240) {
      cl = 3;
      l++;  /* in case of surrogates */
    } else if (ch0 >= 224) {
      cl = 2;
    } else if (ch0 >= 192) {
      cl = 1;
    } else {
      cl = 0;
    }
    
    i += cl;
  }
  
  dst = (unsigned short *)calloc(l+1,sizeof(unsigned short));

  if (dst == NULL) {
    return dst;
  }

  i = 0;

  while (*src != '\0') {
    unsigned long ch = 0;
    unsigned long ch0 = *src;
    unsigned short cl = 0;
    
    if (ch0 >= 252) {
      cl = 5;
    } else if (ch0 >= 248) {
      cl = 4;
    } else if (ch0 >= 240) {
      cl = 3;
    } else if (ch0 >= 224) {
      cl = 2;
    } else if (ch0 >= 192) {
      cl = 1;
    }
    
    switch(cl) {
    case 5: 
      if (*src == 0) {
	free(dst);
	return NULL;
      }
      ch += *src++; ch <<= 6;
    case 4: 
      if (*src == 0) {
	free(dst);
	return NULL;
      }
      ch += *src++; ch <<= 6;
    case 3: 
      if (*src == 0) {
	free(dst);
	return NULL;
      }
      ch += *src++; ch <<= 6;
    case 2: 
      if (*src == 0) {
	free(dst);
	return NULL;
      }
      ch += *src++; ch <<= 6;
    case 1: 
      if (*src == 0) {
	free(dst);
	return NULL;
      }
      ch += *src++; ch <<= 6;
    case 0: 
      if (*src == 0) {
	free(dst);
	return NULL;
      }
      ch += *src++;
    }

    switch(cl) {
    case 5:
      ch -= 0x82082080UL;
      break;
    case 4:
      ch -= 0xFA082080UL;
      break;
    case 3:
      ch -= 0x03C82080UL;
      break;
    case 2:
      ch -= 0x000E2080UL;       
      break;
    case 1:
      ch -= 0x00003080UL;
      break;
    }
    
    if (ch <= 0xFFFFL) {
      dst[i] = ch;
      i++;
    } else if (ch > 0x0010FFFFL) {
      dst[i] = 0xFFFDL;
      i++;
    } else {
      
      ch -= 0x10000L;
      dst[i] = (ch >> 10) + 0xD800L;
      i++;
      dst[i] = (ch & 0x3FFL) + 0xDC00L;
      i++;
    }
  }
  
  dst[i] = 0;
  return dst;
}

unsigned char *ldap_charset_utf8_to_88591 (
	unsigned char *src
)
{
  unsigned char *t;
  int l,i,j;
  
  if (src == NULL) return NULL;

  l = strlen((char *)src);
  
  t = (unsigned char *)calloc(l,sizeof(unsigned char));

  if (t == NULL) {
    return t;
  }

  for (i = 0, j = 0; src[i] != '\0'; i++,j++) {
    unsigned char ch0 = src[i];
    unsigned short cl = 0;
    
    if (ch0 >= 252) {
      cl = 5;
    } else if (ch0 >= 248) {
      cl = 4;
    } else if (ch0 >= 240) {
      cl = 3;
    } else if (ch0 >= 224) {
      cl = 2;
    } else if (ch0 >= 192) {
      cl = 1;
    }
    
    if (cl == 0) {
      t[j] = ch0;
    } else if (cl == 1 && ch0 <= 0xC3) {
      unsigned char ch1;

      t[j] = ((ch0 & 0x3) << 6);
      i++;
      
      ch1 = src[i];
      if (ch1 == 0) break;
      t[j] |= (ch1 & 0x7f);
      
    } else {
      int k;

      t[j] = '?';
      
      for (k = 0; k < cl; k++) {
	i++;
	if (src[i] == '\0') {
	  j++;
	  t[j] = '\0';
	  return t;
	}
      }
    }
  }
  
  t[j] = '\0';
  return t;
}

char *ldap_charset_utf8_to_ascii (
	unsigned char *src
)
{
  char *t;
  int l,i,j;
  
  if (src == NULL) return NULL;

  l = strlen((char *)src);
  
  t = (char *)calloc(l,sizeof(char));

  if (t == NULL) {
    return t;
  }

  for (i = 0, j = 0; src[i] != '\0'; i++,j++) {
    unsigned char ch0 = src[i];
    unsigned short cl = 0;
    
    if (ch0 >= 252) {
      cl = 5;
    } else if (ch0 >= 248) {
      cl = 4;
    } else if (ch0 >= 240) {
      cl = 3;
    } else if (ch0 >= 224) {
      cl = 2;
    } else if (ch0 >= 192) {
      cl = 1;
    }
    
    if (cl == 0) {
      t[j] = ch0;
    } else {
      int k;

      t[j] = '?';
      
      for (k = 0; k < cl; k++) {
	i++;
	if (src[i] == '\0') {
	  j++;
	  t[j] = '\0';
	  return t;
	}
      }
    }
  }
  
  t[j] = '\0';
  return t;
}

unsigned char *ldap_charset_88591_to_utf8 (
	unsigned char *src
)
{
  unsigned char *t;
  int l,i,j;
  
  if (src == NULL) return NULL;
  
  for (l = 0, i = 0; src[i] != '\0'; i++,l++) {
    if (src[i] >= 0x80) l++;
  }
  
  l++;
  
  t = (unsigned char *)calloc(l,sizeof(unsigned char));

  if (t == NULL) {
    return t;
  }
  
  for (i = 0, j = 0; src[i] != '\0'; i++) {
    if (src[i] < 0x80) {
      t[j] = src[i];
      j++;
    } else {
      unsigned char c = src[i];

      t[j+1] = (c | 0x80) & 0xBF;
      c >>= 6;
      t[j] = c | 0xC0;
      j += 2;
    }
  }
  
  t[j] = '\0';
  
  return t;
}


/*
*** ldap_ utf8_charlen - returns the byte length (1, 2 or 3) 
*** of the next utf8 char.  0 indicates end of string or an 
*** error condition
*/
int ldap_utf8_charlen(const unsigned char *str) {
    if ((str==NULL) || (*str=='\0')) {
        return 0;
    }

    /* check for one byte encoding */
    if  (!(*str & 0x80)) {
        return 1;  
    }
   
    /* check for two byte encoding */
    if (((*str>>5)&0x7) == 0x6) {
        str++;
        if (((*str>>6)&0x3) != 0x2) {
            return 0;
        }
        return 2;
    }
 
    /* check for three byte encoding */
    if (((*str>>4)&0xf) == 0xe) {
         str++;
        if (((*str>>6)&0x3) != 0x2) {
            return 0;
        }

        str++;
        if (((*str>>6)&0x3) != 0x2) {
            return 0;
        }

        return 3;
    }

    /* invalid encoding */
    return 0;
}

/*
*** ldap_utf8_nextchar - advance one utf8 character in a 
*** string returns a pointer to the start of the next 
*** character return NULL on encoding errors
*/ 
unsigned char* ldap_utf8_nextchar(const unsigned char *str) {
    int len = ldap_utf8_charlen(str);
    if (len==0) 
        return NULL;
    return (unsigned char *)str+len;
}


/*
*** ldap_utf8_isspace - check if the next utf8 character 
*** is an encoding of a whitespace character
*/ 
int ldap_utf8_isspace(const unsigned char *str) {
    int len=ldap_utf8_charlen(str);
    if (len==0) 
        return 0;
    
    if (len==1) {
        switch(*str) { 
            case 0x09:
            case 0x0A:
            case 0x0B:
            case 0x0C:
            case 0x0D:
            case 0x20:
                return 1;
            default:
                return 0;
        }
    } else if (len==2) {
        if (*str == 0xc2) {
            return *(str+1) == 0x80;
        }
    } else if (len==3) {
        if (*str == 0xE2) {
            str++;
            if (*str == 0x80) {
                str++;
                return (*str>=0x80 && *str<=0x8a);
            }
        } else if (*str == 0xE3) {
            return (*(str+1)==0x80) && (*(str+2)==0x80);
        } else if (*str==0xEF) {
            return (*(str+1)==0xBB) && (*(str+2)==0xBF);
        }
        return 0;
    }

    return 0; /* should never reach this point */
}

/*  11000 x xx 10xxxxxx */

/* Latin-1 sup: 00E0-00F6  --> 00C0-00D6
 *              00F8-00FE --> 00D8-00DE */

/*              0 11 100000- 0 11 110110 --> 0 11 000000- 0 11 010110
		0 11 111000- 0 11 111110 --> 0 11 011000- 0 11 011110 */

/*
*** ldap_utf8_toupper - copy a utf8 character and convert 
*** it to upper case.  currently only converts ASCII and ISO-Latin-1
*** characters.
*** returns the number of bytes in the utf8 character copied
*/

int ldap_utf8_toupper(unsigned char *dst, const unsigned char *src) 
{
  int len;

  if (src == NULL) return 0;

  if (!(src[0] & 0x80)) {
    *dst = TOUPPER(*src);
    return 1;
  }

  len = ldap_utf8_charlen(src);
  if (len==1) {
    *dst = TOUPPER(*src);
  } else if (len == 2 && src[0] == 0xC3 &&
	     src[1] >= 0xA0 && src[1] <= 0xBE) {
    /* Latin-1 supplement */
    /* C3A0-C3B6 ---> C380-C396 */
    /* C3B8-C3BE ---> C398-C39E */
    *dst = *src;
    dst[1] = src[1]; 	
    if (src[1] != 0xB7) dst[1] -= 0x20;
    
  } else if (len == 2 && src[0] == 0xC4 && 
	     src[1] >= 0x80 && src[1] <= 0xB7 &&
	     (src[1] & 1)) {
    /* Latin E-A:   0101-0137 & odd: sub 1 ; 1 00 000001 - 1 00 110111 */
    *dst = *src;
    dst[1] = src[1] - 1;
    
  } else if (len == 2 && src[0] == 0xC4 && 
	     src[1] >= 0xBA && src[1] <= 0xBF && 
	     ((src[1] & 1) == 0)) {
    /* Latin E-A:   013A-0140 & even: sub 1; 1 00 111010 - */
    *dst = *src;
    dst[1] = src[1] - 1;	
    
  } else if (len == 2 && src[0] == 0xC5 && src[1] == 0x80) {
    *dst = 0xC4;
    dst[1] = 0xBF;
  } else if (len == 2 && src[0] == 0xC5 &&
	     src[1] >= 0x82 && src[1] <= 0x88 && 
	     ((src[1] & 1) == 0)) {
    /* Latin E-A:   0142-0148 & even: sub 1; 1 01 000000 - */
    *dst = *src;
    dst[1] = src[1] - 1;
  } else if (len == 2 && src[0] == 0xC5 && 
	     src[1] >= 0x8A && src[1] <= 0xB7 &&
	     (src[1] & 1)) {
    /* Latin E-A:   014B-0177 & odd: sub 1 ; 1 01 001010 - */
    *dst = *src;
    dst[1] = src[1] - 1;
  } else if (len == 2 && src[0] == 0xC5 && 
	     src[1] >= 0xBA && src[1] <= 0xBE && 
	     ((src[1] & 1) == 0)) {
    /* Latin E-A:   017A-017E & even: sub 1; 1 01 111010 - */
    *dst = *src;
    dst[1] = src[1] - 1; 	

  } else {
    memcpy(dst,src,len);
  }
  return len;
}

/*
*** ldap_utf8_charcpy - copy one utf character from the src string to
*** the destination string.
*** returns the number of bytes in the utf8 character copied
*/
int ldap_utf8_charcpy(unsigned char *dst, const unsigned char *src) {
    int len = ldap_utf8_charlen(src);
    memcpy(dst,src,len);
    return len;
}
