/* Insert copyright and license here 1996 */

#ifndef _MIMECMS_H_
#define _MIMECMS_H_

#include "mimecryp.h"
#include "nsICMS.h"

/* The MimeEncryptedCMS class implements a type of MIME object where the
   object is passed through a CMS decryption engine to decrypt or verify
   signatures.  That module returns a new MIME object, which is then presented
   to the user.  See mimecryp.h for details of the general mechanism on which
   this is built.
 */

typedef struct MimeEncryptedCMSClass MimeEncryptedCMSClass;
typedef struct MimeEncryptedCMS      MimeEncryptedCMS;

struct MimeEncryptedCMSClass {
  MimeEncryptedClass encrypted;

  /* Callback used to access the SEC_PKCS7ContentInfo of this object. */
  void (*get_content_info) (MimeObject *self,
							nsICMSMessage **content_info_ret,
							char **sender_email_addr_return,
							PRInt32 *decode_error_ret,
              PRInt32 *verify_error_ret,
              PRBool * ci_is_encrypted);
};

extern MimeEncryptedCMSClass mimeEncryptedCMSClass;

struct MimeEncryptedCMS {
  MimeEncrypted encrypted;		/* superclass variables */
};

#endif /* _MIMEPKCS_H_ */
