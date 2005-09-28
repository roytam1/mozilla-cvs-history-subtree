#ifndef __G711_H__
#define __G711_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "prtypes.h"

  PRUint8 linear2ulaw(PRInt32 pcm_val);
  PRInt32 ulaw2linear(PRUint8 u_val);
  PRUint8 linear2alaw(PRUint32 pcm_val);
  PRInt32 alaw2linear(PRUint8 a_val);

#ifdef __cplusplus
};
#endif

#endif /* __G711_H__ */
