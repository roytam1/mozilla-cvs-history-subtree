/* Insert copyright and license here 1996 */

#ifndef _MIMEMPKC_H_
#define _MIMEMPKC_H_

#include "mimemsig.h"
#include "secpkcs7.h"

/* The MimeMultipartSignedPKCS7 class implements a multipart/signed MIME 
   container with protocol=application/x-pkcs7-signature, which passes the
   signed object through PKCS7 code to verify the signature.  See mimemsig.h
   for details of the general mechanism on which this is built.
 */

typedef struct MimeMultipartSignedPKCS7Class MimeMultipartSignedPKCS7Class;
typedef struct MimeMultipartSignedPKCS7      MimeMultipartSignedPKCS7;

struct MimeMultipartSignedPKCS7Class {
  MimeMultipartSignedClass msigned;

  /* Callback used to access the SEC_PKCS7ContentInfo of this object. */
  void (*get_content_info) (MimeObject *self,
							SEC_PKCS7ContentInfo **content_info_ret,
							char **sender_email_addr_return,
							int32 *decode_error_ret,
							int32 *verify_error_ret,
                            XP_Bool * ci_is_encrypted);
};

extern MimeMultipartSignedPKCS7Class mimeMultipartSignedPKCS7Class;

struct MimeMultipartSignedPKCS7 {
  MimeMultipartSigned msigned;
};

#endif /* _MIMEMPKC_H_ */
