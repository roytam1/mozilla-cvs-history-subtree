/* Insert copyright and license here 1996 */

#ifndef _MIMEMPKC_H_
#define _MIMEMPKC_H_

#include "nsICMS.h"
#include "mimemsig.h"

/* The MimeMultipartSignedCMS class implements a multipart/signed MIME 
   container with protocol=application/x-CMS-signature, which passes the
   signed object through CMS code to verify the signature.  See mimemsig.h
   for details of the general mechanism on which this is built.
 */

typedef struct MimeMultipartSignedCMSClass MimeMultipartSignedCMSClass;
typedef struct MimeMultipartSignedCMS      MimeMultipartSignedCMS;

struct MimeMultipartSignedCMSClass {
  MimeMultipartSignedClass msigned;

  /* Callback used to access the SEC_CMSContentInfo of this object. */
  void (*get_content_info) (MimeObject *self,
							nsICMSMessage **content_info_ret,
							char **sender_email_addr_return,
							PRInt32 *decode_error_ret,
							PRInt32 *verify_error_ret,
              PRBool * ci_is_encrypted);
};

extern MimeMultipartSignedCMSClass mimeMultipartSignedCMSClass;

struct MimeMultipartSignedCMS {
  MimeMultipartSigned msigned;
};

#endif /* _MIMEMPKC_H_ */
