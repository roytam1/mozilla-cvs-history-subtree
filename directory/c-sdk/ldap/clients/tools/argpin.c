/******************************************************
 *
 *  Copyright (c) 1996 Netscape Communications Corp.
 *  This code is proprietary and is a trade secret of
 *  Netscape Communications Corp.
 *
 *  argpin.c - Returns pin for token specified in a
 *  command line paramenter.
 *
 ******************************************************/

#include <stdio.h>
#include <string.h>

#include "argpin.h"

struct SVRCOREArgPinObj
{
  SVRCOREPinObj base;

  char *tokenName;
  char *password;
  SVRCOREPinObj *alt;
};
static const struct SVRCOREPinMethods vtable;

/*  XXXceb these are two hacks to fix a problem with the debug builds
 *  of svrcore.  With the optimizer turned off, there is a situation
 *  in user.c, where these two functions need to be available for the
 *  linker (they are imported, and no lib exports them, since they are
 *  declared static on XP_UNIX platforms) The short term hack solution
 *  is to define them here.  Yeah, it is ugly but, it will need to be
 *  here, until a new version of svrcore is done. 
 */


void echoOff(int fd)
{
}
 
void echoOn(int fd)
{
}


/* ------------------------------------------------------------ */
SVRCOREError
SVRCORE_CreateArgPinObj(SVRCOREArgPinObj **out, const char * tokenName, const char *password, SVRCOREPinObj *pinObj)
{
  SVRCOREError err = 0;
  SVRCOREArgPinObj *obj = 0;

  do {
    obj = (SVRCOREArgPinObj*)malloc(sizeof (SVRCOREArgPinObj));
    if (!obj) { err = 1; break; }

    obj->base.methods = &vtable;
    obj->tokenName=NULL;
    obj->password=NULL;
    obj->alt=pinObj;

    if ( tokenName == NULL) {
      PK11SlotInfo *slot = PK11_GetInternalKeySlot();

      obj->tokenName = strdup(PK11_GetTokenName(slot));
      PK11_FreeSlot(slot);
    }
    else
    {
      obj->tokenName = strdup(tokenName);
    }
    if (obj->tokenName == NULL) { err = 1; break; }

    obj->password = strdup(password);
    if (obj->password == NULL) { err = 1; break; }
  } while(0);

  if (err)
  {
    SVRCORE_DestroyArgPinObj(obj);
    obj = 0;
  }

  *out = obj;
  return err;
}

void
SVRCORE_DestroyArgPinObj(SVRCOREArgPinObj *obj)
{
  if (obj->tokenName) free(obj->tokenName);
  if (obj->password)
  {
    memset(obj->password, 0, strlen(obj->password));
    free(obj->password);
  }
  if (obj) free(obj);
}

static void destroyObject(SVRCOREPinObj *obj)
{
  SVRCORE_DestroyArgPinObj((SVRCOREArgPinObj*)obj);
}

static char *getPin(SVRCOREPinObj *obj, const char *tokenName, PRBool retry)
{
    SVRCOREArgPinObj *p = (SVRCOREArgPinObj*)obj;

    if (tokenName == NULL) return NULL;

    /* On first attempt, return the password if the token name
     * matches.
     */
    if (!retry && strcmp(p->tokenName, tokenName) == 0)
    {
      return strdup(p->password);
    }

    if (p->alt != NULL) return SVRCORE_GetPin(p->alt, tokenName, retry);

    return (NULL);
}

/*
 * VTable
 */
static const SVRCOREPinMethods vtable =
{ 0, 0, destroyObject, getPin };

