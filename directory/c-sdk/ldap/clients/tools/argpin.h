#ifndef __ARGPIN_H__
#define __ARGPIN_H__

#include <svrcore.h>

typedef struct SVRCOREArgPinObj SVRCOREArgPinObj;

SVRCOREError
SVRCORE_CreateArgPinObj(SVRCOREArgPinObj **out, const char * tokenName, const char *password, SVRCOREPinObj *pinObj);

void
SVRCORE_DestroyArgPinObj(SVRCOREArgPinObj *obj);

#endif
