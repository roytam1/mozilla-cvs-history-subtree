/* Insert copyright and license here 1995 */
/*
   composec.h --- the crypto interface to MIME generation (see compose.c.)
 */

#include "xp.h"

XP_BEGIN_PROTOS

extern int mime_begin_crypto_encapsulation (MWContext *context,
											XP_File file,
											void **closure_return,
											PRBool encrypt_p, PRBool sign_p,
											const char *recipients,
											PRBool saveAsDraft);
extern int mime_finish_crypto_encapsulation (void *closure, PRBool abort_p);
extern int mime_crypto_write_block (void *closure, char *buf, PRInt32 size);

XP_END_PROTOS
