/******************************************************
 *
 *  Copyright (c) 1996 Netscape Communications Corp.
 *  This code is proprietary and is a trade secret of
 *  Netscape Communications Corp.
 *
 *  ntuserpin.c - Prompts for the key
 *  database passphrase.
 *
 ******************************************************/

#if defined( _WIN32 ) && defined ( NET_SSL )

#include <conio.h>
#include "ntuserpin.h"

#undef Debug
#undef OFF
#undef LITTLE_ENDIAN

#include <stdio.h>
#include <string.h>
#include <sys/types.h>


static int i=0;
static int cbRemotePassword = 0;
static const char nt_retryWarning[] =
"Warning: You entered an incorrect PIN.\nIncorrect PIN may result in disabling the token";
static const char prompt[] = "Enter PIN for";


#define SZ_LOCAL_PWD 1024
static char loclpwd[SZ_LOCAL_PWD] = "";
struct SVRCORENTUserPinObj
{
    SVRCOREPinObj base;
};
static const struct SVRCOREPinMethods vtable;
/* ------------------------------------------------------------ */
SVRCOREError
SVRCORE_CreateNTUserPinObj(SVRCORENTUserPinObj **out)
{
    SVRCOREError err = 0;
    SVRCORENTUserPinObj *obj = 0;
    do {
	obj = (SVRCORENTUserPinObj*)malloc(sizeof (SVRCORENTUserPinObj));
	if (!obj) { err = 1; break; }
	obj->base.methods = &vtable;
    } while(0);
    if (err)
    {
	SVRCORE_DestroyNTUserPinObj(obj);
	obj = 0;
    }
    *out = obj;
    return err;
}
void
SVRCORE_DestroyNTUserPinObj(SVRCORENTUserPinObj *obj)
{
  if (obj) free(obj);
}
static void destroyObject(SVRCOREPinObj *obj)
{
  SVRCORE_DestroyNTUserPinObj((SVRCORENTUserPinObj*)obj);
}
static char *getPin(SVRCOREPinObj *obj, const char *tokenName, PRBool retry)
{
    char *pwd;
    int ch;
    if (retry)
	printf("%s\n",nt_retryWarning);
    printf("%s %s:", prompt, tokenName);
    pwd = &loclpwd[0];
    do
    {
        ch = _getch();
	*pwd++ = (char )ch;
    } while( ch != '\r' && (pwd < &loclpwd[SZ_LOCAL_PWD - 1]));
    *(pwd-1)='\0';
    printf("\n");

    /* test for zero length password.  if zero length, return null */
    if ('\0' == loclpwd[0])
	return NULL;

    return &loclpwd[0];
}

/*
 * VTable
 */
static const SVRCOREPinMethods vtable =
{ 0, 0, destroyObject, getPin };
#endif /* defined( _WIN32 ) && defined ( NET_SSL ) */

