/******************************************************
 *
 *  Copyright (c) 1996 Netscape Communications Corp.
 *  This code is proprietary and is a trade secret of
 *  Netscape Communications Corp.
 *
 *  ntuserpin.h - Prompts for the key
 *  database passphrase.
 *
 ******************************************************/
#ifndef _NTUSERPIN_H_
#define _NTUSERPIN_H_
#include "svrcore.h"

typedef struct SVRCORENTUserPinObj SVRCORENTUserPinObj;

SVRCOREError
SVRCORE_CreateNTUserPinObj(SVRCORENTUserPinObj **out);

void
SVRCORE_SetNTUserPinInteractive(SVRCORENTUserPinObj *obj, PRBool interactive);

void
SVRCORE_DestroyNTUserPinObj(SVRCORENTUserPinObj *obj);
#endif
