/* Insert copyright and license here 1995 */
/*
   composec.h --- the crypto interface to MIME generation (see compose.c.)
 */
#include "nspr.h"
#include "msgCore.h"
#include "nsFileStream.h"
extern "C" nsresult mime_begin_crypto_encapsulation (nsOutputFileStream * file,
											void **closure_return,
											PRBool encrypt_p, PRBool sign_p,
											const char *recipients,
											PRBool saveAsDraft);
extern "C" nsresult mime_finish_crypto_encapsulation (void *closure, PRBool abort_p);
extern "C" nsresult mime_crypto_write_block (void *closure, char *buf, PRInt32 size);
