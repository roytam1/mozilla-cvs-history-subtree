/* Insert copyright and license here 1997 */

/*
   composec.c --- the crypto interface to MIME generation (see compose.c.)
 */


#include "composec.h"
#include "nspr.h"
#include "nsCOMPtr.h"
#include "nsICMSDecoder.h"
#include "nsIX509Cert.h"
#include "nsIMimeConverter.h"
#include "nsMsgEncoders.h"
#include "nsMimeStringResources.h"
#include "nsMimeTypes.h"
#include "nsIX509Cert.h"
#include "nsIX509CertDB.h"
#include "nsMsgBaseCID.h"
#include "nsIMsgHeaderParser.h"

static NS_DEFINE_CID(kMsgHeaderParserCID, NS_MSGHEADERPARSER_CID); 

#define MIME_MULTIPART_SIGNED_BLURB "test"
#define MIME_SMIME_ENCRYPTED_CONTENT_DESCRIPTION "test"
#define MIME_SMIME_SIGNATURE_CONTENT_DESCRIPTION "test"
#define MK_MIME_ERROR_WRITING_FILE -1

#define SEC_ERROR_NO_EMAIL_CERT -100

typedef enum {
  mime_crypto_none,				/* normal unencapsulated MIME message */
  mime_crypto_clear_signed,		/* multipart/signed encapsulation */
  mime_crypto_opaque_signed,	/* application/x-pkcs7-mime (signedData) */
  mime_crypto_encrypted,		/* application/x-pkcs7-mime */
  mime_crypto_signed_encrypted	/* application/x-pkcs7-mime */
} mime_delivery_crypto_state;


typedef struct mime_crypto_closure {
  mime_delivery_crypto_state crypto_state;
  nsOutputFileStream *file;

  /* Slots used when using multipart/signed
	 (clearsigning, or signing and encrypting)
   */
  PRInt16 hash_type;
  nsCOMPtr<nsIHash> data_hash_context;

  MimeEncoderData *sig_encoder_data;
  char *multipart_signed_boundary;


  /* Slots used when encrypting (or signing and encrypting)
   */
  nsCOMPtr<nsIX509Cert> self_signing_cert;
  nsCOMPtr<nsIX509Cert> self_encryption_cert;
  nsCOMPtr<nsISupportsArray> certs;	/* ist of the certs to which this (encrypted) message is being addressed. */

  nsCOMPtr<nsICMSMessage> encryption_cinfo;
  nsCOMPtr<nsICMSEncoder> encryption_context;

  MimeEncoderData *crypto_encoder_data;

  PRBool isDraft;

} mime_crypto_closure;


static void
free_crypto_closure (mime_crypto_closure *state)
{
  if (state->encryption_context) {
    state->encryption_context->Finish();
    state->encryption_context = 0;
  }
  if (state->encryption_cinfo) {
    state->encryption_cinfo = 0;
  }
  if (state->sig_encoder_data) {
	  MIME_EncoderDestroy (state->sig_encoder_data, PR_TRUE);
  }
  if (state->crypto_encoder_data) {
	  MIME_EncoderDestroy (state->crypto_encoder_data, PR_TRUE);
  }

  // XXX Free the 'certs' array. Fix Me XXX //
  PR_FREEIF(state->multipart_signed_boundary);
  PR_Free(state);
}

#if 0
extern void msg_SetCompositionSecurityError(MWContext *, int status);

#endif

static void
msg_remember_security_error(mime_crypto_closure *state, int status)
{
  if (status >= 0) return;
  if (!state) return;
//   msg_SetCompositionSecurityError(state->context, status); XXX Fix this XXX //
}

extern "C" char *mime_make_separator(const char *prefix);


/* Returns a string consisting of a Content-Type header, and a boundary
   string, suitable for moving from the header block, down into the body
   of a multipart object.  The boundary itself is also returned (so that
   the caller knows what to write to close it off.)
 */
static int
make_multipart_signed_header_string(PRBool outer_p,
									char **header_return,
									char **boundary_return)
{
  *header_return = 0;
  *boundary_return = mime_make_separator("ms");
  const char * crypto_multipart_blurb = nsnull;

  if (!*boundary_return)
	return NS_ERROR_OUT_OF_MEMORY;

  if (outer_p) {
	  crypto_multipart_blurb = MIME_MULTIPART_SIGNED_BLURB;
  }

  *header_return =
	PR_smprintf("Content-Type: " MULTIPART_SIGNED "; "
				"protocol=\"" APPLICATION_XPKCS7_SIGNATURE "\"; "
				"micalg=" PARAM_MICALG_SHA1 "; "
				"boundary=\"%s\"" CRLF
				CRLF
				"%s%s"
				"--%s" CRLF,

				*boundary_return,
				(crypto_multipart_blurb ? crypto_multipart_blurb : ""),
				(crypto_multipart_blurb ? CRLF CRLF : ""),
				*boundary_return);

  if (!*header_return)
	{
	  PR_Free(*boundary_return);
	  *boundary_return = 0;
	  return NS_ERROR_OUT_OF_MEMORY;
	}

  return 0;
}


static int mime_init_multipart_signed(mime_crypto_closure *, PRBool outer_p);
static int mime_init_encryption(mime_crypto_closure *state, PRBool sign_p);
static int mime_crypto_hack_certs(mime_crypto_closure *,
				  const char *recipients,
				  PRBool encrypt_p,
				  PRBool sign_p);


/* Called by compose.c / msgsend.cpp, after the main headers have been written,
   but before the Content-Type or message body have been written.  This returns
   a mime_crypto_closure object, which will signal the caller to write data
   into the message via mime_crypto_write_block() rather than writing it
   directly to the file.
 */
nsresult
mime_begin_crypto_encapsulation (nsOutputFileStream *file, void **closure_return,
								 PRBool encrypt_p, PRBool sign_p,
								 const char *recipients,
								 PRBool saveAsDraft)
{
  int status = 0;
  mime_crypto_closure *state;

  *closure_return = 0;

  PR_ASSERT(encrypt_p || sign_p);
  if (!encrypt_p && !sign_p) return 0;

  state = (mime_crypto_closure *) PR_MALLOC(sizeof(*state));
  if (!state) return NS_ERROR_OUT_OF_MEMORY;

  nsCRT::memset(state, 0, sizeof(*state));
  state->file = file;

  state->isDraft = saveAsDraft;

  if (encrypt_p && sign_p)
	  state->crypto_state = mime_crypto_signed_encrypted;
  else if (encrypt_p)
	  state->crypto_state = mime_crypto_encrypted;
  else if (sign_p)
	  state->crypto_state = mime_crypto_clear_signed;
  else
	  PR_ASSERT(0);

  status = mime_crypto_hack_certs(state, recipients, encrypt_p, sign_p);
  if (status < 0) goto FAIL;

  if (status > 0) status = 0;  /* We use >0 to mean "send with no crypto." */

  switch (state->crypto_state)
	{
	case mime_crypto_clear_signed:
	  status = mime_init_multipart_signed(state, PR_TRUE);
	  break;
	case mime_crypto_opaque_signed:
	  PR_ASSERT(0);    /* #### no api for this yet */
	  status = -1;
	  break;
	case mime_crypto_signed_encrypted:
	  status = mime_init_encryption(state, PR_TRUE);
	  break;
	case mime_crypto_encrypted:
	  status = mime_init_encryption(state, PR_FALSE);
	  break;
	case mime_crypto_none:
	  /* This can happen if mime_crypto_hack_certs() decided to turn off
		 encryption (by asking the user.) */
	  status = 1;
	  break;
	default:
	  PR_ASSERT(0);
	  status = -1;
	  break;
	}

FAIL:
  msg_remember_security_error(state, status);

  if (status != 0) {
	  free_crypto_closure(state);
  } else {
	  *closure_return = state;
  }

  return status;
}


/* Helper function for mime_begin_crypto_encapsulation() to start a
   multipart/signed object (for signed or signed-and-encrypted messages.)
 */
static int
mime_init_multipart_signed(mime_crypto_closure *state, PRBool outer_p)
{

  /* First, construct and write out the multipart/signed MIME header data.
   */
  nsresult rv;
  char *header = 0;
  PRInt32 L;
  int status =
	make_multipart_signed_header_string(outer_p, &header,
										&state->multipart_signed_boundary);
  if (status < 0) goto FAIL;

  L = nsCRT::strlen(header);

  if (outer_p){
	  /* If this is the outer block, write it to the file. */
    if (PRInt32(state->file->write(header, L)) < L) {
		  status = MK_MIME_ERROR_WRITING_FILE;
    }
  } else {
	  /* If this is an inner block, feed it through the crypto stream. */
	  status = mime_crypto_write_block (state, header, L);
	}

  PR_Free(header);
  if (status < 0) goto FAIL;

  /* Now initialize the crypto library, so that we can compute a hash
	 on the object which we are signing.
   */

  state->hash_type = nsIHash::HASH_AlgSHA1;

  PR_SetError(0,0);
  state->data_hash_context = do_CreateInstance(NS_HASH_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return 0;

  rv = state->data_hash_context->Create(state->hash_type);
  if (NS_FAILED(rv))
	{
	  status = PR_GetError();
	  PR_ASSERT(status < 0);
	  if (status >= 0) status = -1;
	  goto FAIL;
	}


  PR_SetError(0,0);
  rv = state->data_hash_context->Begin();
  status = PR_GetError();
 FAIL:
  msg_remember_security_error(state, status);
  return status;
}


static void mime_crypto_write_base64 (void *closure, const char *buf,
									  unsigned long size);
static nsresult mime_encoder_output_fn(const char *buf, PRInt32 size, void *closure);
static nsresult mime_nested_encoder_output_fn (const char *, PRInt32, void *closure);


/* Helper function for mime_begin_crypto_encapsulation() to start an
   an opaque crypto object (for encrypted or signed-and-encrypted messages.)
 */
static int
mime_init_encryption(mime_crypto_closure *state, PRBool sign_p)
{
  int status = 0;
  nsresult rv;

  /* First, construct and write out the opaque-crypto-blob MIME header data.
   */

  char *s =
	PR_smprintf("Content-Type: " APPLICATION_XPKCS7_MIME
				  "; name=\"smime.p7m\"" CRLF
				"Content-Transfer-Encoding: " ENCODING_BASE64 CRLF
				"Content-Disposition: attachment"
				  "; filename=\"smime.p7m\"" CRLF
				"Content-Description: %s" CRLF
				CRLF,
				MIME_SMIME_ENCRYPTED_CONTENT_DESCRIPTION);
  PRInt32 L;
  if (!s) return NS_ERROR_OUT_OF_MEMORY;
  L = nsCRT::strlen(s);
  if (PRInt32(state->file->write(s, L)) < L) {
	  return MK_MIME_ERROR_WRITING_FILE;
  }
  PR_Free(s);
  s = 0;

  /* Now initialize the crypto library, so that we can filter the object
	 to be encrypted through it.
   */

  if (! state->isDraft) {
    PRUint32 numCerts;
    state->certs->Count(&numCerts);
    PR_ASSERT(numCerts > 0);
	  if (numCerts == 0) return -1;
  }

  /* Initialize the base64 encoder. */
  PR_ASSERT(!state->crypto_encoder_data);
  state->crypto_encoder_data = MIME_B64EncoderInit(mime_encoder_output_fn,
												  state);
  if (!state->crypto_encoder_data)
	return NS_ERROR_OUT_OF_MEMORY;

  /* Initialize the encrypter (and add the sender's cert.) */
  PR_ASSERT(state->self_encryption_cert);
  PR_SetError(0,0);
  state->encryption_cinfo = do_CreateInstance(NS_CMSMESSAGE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return 0;
  rv = state->encryption_cinfo->CreateEncrypted(); // XXX Fix this later XXX //
  if (NS_FAILED(rv)) {
	  status = PR_GetError();
	  PR_ASSERT(status < 0);
	  if (status >= 0) status = -1;
	  goto FAIL;
	}

  PR_SetError(0,0);
  state->encryption_context = do_CreateInstance(NS_CMSENCODER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return 0;

  rv = state->encryption_context->Start(); // XXX Fix this later XXX //
  if (NS_FAILED(rv)) {
	  status = PR_GetError();
	  PR_ASSERT(status < 0);
	  if (status >= 0) status = -1;
	  goto FAIL;
	}

  /* If we're signing, tack a multipart/signed header onto the front of
	 the data to be encrypted, and initialize the sign-hashing code too.
   */
  if (sign_p) {
	  status = mime_init_multipart_signed(state, PR_FALSE);
	  if (status < 0) goto FAIL;
	}

 FAIL:
  msg_remember_security_error(state, status);
  return status;
}

static int mime_finish_multipart_signed (mime_crypto_closure *state,
										 PRBool outer_p);
static int mime_finish_encryption (mime_crypto_closure *state, PRBool sign_p);


/* Called by compose.c / msgsend.cpp, when the whole body has been processed.
 */
nsresult
mime_finish_crypto_encapsulation (void *closure, PRBool abort_p)
{
  mime_crypto_closure *state = (mime_crypto_closure *) closure;
  int status = 0;

  if (!abort_p)
	{
	  switch (state->crypto_state)
		{
		case mime_crypto_clear_signed:
		  status = mime_finish_multipart_signed (state, PR_TRUE);
		  break;
		case mime_crypto_opaque_signed:
		  PR_ASSERT(0);    /* #### no api for this yet */
		  status = -1;
		  break;
		case mime_crypto_signed_encrypted:
		  status = mime_finish_encryption (state, PR_TRUE);
		  break;
		case mime_crypto_encrypted:
		  status = mime_finish_encryption (state, PR_FALSE);
		  break;
		default:
		  PR_ASSERT(0);
		  status = -1;
		  break;
		}
	}

  msg_remember_security_error(state, status);
  free_crypto_closure(state);
  return status;
}


/* Helper function for mime_finish_crypto_encapsulation() to close off
   a multipart/signed object (for signed or signed-and-encrypted messages.)
 */
static int
mime_finish_multipart_signed (mime_crypto_closure *state, PRBool outer_p)
{
  int status;
  nsresult rv;
  PRUint32 sec_item_len;
  unsigned char * sec_item_data;
  nsCOMPtr<nsICMSMessage> cinfo = do_CreateInstance(NS_CMSMESSAGE_CONTRACTID, &rv);
  nsCOMPtr<nsICMSEncoder> encoder = do_CreateInstance(NS_CMSENCODER_CONTRACTID, &rv);
  PRStatus ds_status = PR_FAILURE;

  /* Compute the hash...
   */
  state->data_hash_context->ResultLen(state->hash_type, &sec_item_len);
  sec_item_data = (unsigned char *) PR_MALLOC(sec_item_len);
  if (!sec_item_data)	{
	  status = NS_ERROR_OUT_OF_MEMORY;
	  goto FAIL;
	}

  PR_SetError(0,0);
  state->data_hash_context->End(sec_item_data, &sec_item_len, sec_item_len);
  status = PR_GetError();
  if (status < 0) {
    goto FAIL;
  }

  PR_SetError(0,0);
  state->data_hash_context = 0;

  status = PR_GetError();
  if (status < 0) goto FAIL;

  /* Write out the headers for the signature.
   */
  {
	PRInt32 L;
	char *header =
	  PR_smprintf(CRLF
				  "--%s" CRLF
				  "Content-Type: " APPLICATION_XPKCS7_SIGNATURE
				    "; name=\"smime.p7s\"" CRLF
				  "Content-Transfer-Encoding: " ENCODING_BASE64 CRLF
				  "Content-Disposition: attachment; "
				    "filename=\"smime.p7s\"" CRLF
				  "Content-Description: %s" CRLF
				  CRLF,
				  state->multipart_signed_boundary,
				  MIME_SMIME_SIGNATURE_CONTENT_DESCRIPTION);
	if (!header) {
		status = NS_ERROR_OUT_OF_MEMORY;
		goto FAIL;
	}

	L = nsCRT::strlen(header);
	if (outer_p) {
		/* If this is the outer block, write it to the file. */
    if (PRInt32(state->file->write(header, L)) < L) {
		  status = MK_MIME_ERROR_WRITING_FILE;
    } else {
		  /* If this is an inner block, feed it through the crypto stream. */
		  status = mime_crypto_write_block (state, header, L);
	  }
  }

	PR_Free(header);
	if (status < 0) goto FAIL;
  }

  /* Create the signature...
   */

  PR_ASSERT(state->hash_type == nsIHash::HASH_AlgSHA1);

  PR_ASSERT (state->self_signing_cert);
  PR_SetError(0,0);
  rv = cinfo->CreateSigned();
  if (NS_FAILED(rv))	{
	  status = PR_GetError();
	  PR_ASSERT(status < 0);
	  if (status >= 0) status = -1;
	  goto FAIL;
	}

  /* Initialize the base64 encoder for the signature data.
   */
  PR_ASSERT(!state->sig_encoder_data);
  state->sig_encoder_data =
	MIME_B64EncoderInit((outer_p
						? mime_encoder_output_fn
						: mime_nested_encoder_output_fn),
					   state);
  if (!state->sig_encoder_data) {
	  status = NS_ERROR_OUT_OF_MEMORY;
	  goto FAIL;
	}

  /* Write out the signature.
   */
  PR_SetError(0,0);
  rv = encoder->Encode (cinfo);
  if (NS_FAILED(rv)) {
	  status = PR_GetError();
	  PR_ASSERT(status < 0);
	  if (status >= 0) status = -1;
	  goto FAIL;
	}

  /* Shut down the sig's base64 encoder.
   */
  status = MIME_EncoderDestroy(state->sig_encoder_data, PR_FALSE);
  state->sig_encoder_data = 0;
  if (status < 0) goto FAIL;

  /* Now write out the terminating boundary.
   */
  {
	PRInt32 L;
	char *header = PR_smprintf(CRLF "--%s--" CRLF,
							   state->multipart_signed_boundary);
	PR_Free(state->multipart_signed_boundary);
	state->multipart_signed_boundary = 0;

	if (!header) {
		status = NS_ERROR_OUT_OF_MEMORY;
		goto FAIL;
	}
	L = nsCRT::strlen(header);
	if (outer_p) {
		/* If this is the outer block, write it to the file. */
		if (PRInt32(state->file->write(header, L)) < L)
		  status = MK_MIME_ERROR_WRITING_FILE;
  } else {
		/* If this is an inner block, feed it through the crypto stream. */
		status = mime_crypto_write_block (state, header, L);
	}
  }

FAIL:
  PR_FREEIF(sec_item_data);

  msg_remember_security_error(state, status);
  return status;
}


/* Helper function for mime_finish_crypto_encapsulation() to close off
   an opaque crypto object (for encrypted or signed-and-encrypted messages.)
 */
static int
mime_finish_encryption (mime_crypto_closure *state, PRBool sign_p)
{
  int status = 0;
  nsresult rv;

  /* If this object is both encrypted and signed, close off the
	 signature first (since it's inside.) */
  if (sign_p)
	{
	  status = mime_finish_multipart_signed (state, PR_FALSE);
	  if (status < 0) goto FAIL;
	}

  /* Close off the opaque encrypted blob.
   */
  PR_ASSERT(state->encryption_context);
  if (!state->encryption_context) status = -1;
  PR_SetError(0,0);
  rv = state->encryption_context->Finish();
  if (NS_FAILED(rv)) {
	  status = PR_GetError();
	  PR_ASSERT(status < 0);
	  if (status >= 0) status = -1;
	}

  state->encryption_context = 0;
  if (status < 0) goto FAIL;

  PR_ASSERT(state->encryption_cinfo);
  if (!state->encryption_cinfo) {
    status = -1;
  }
  if (state->encryption_cinfo) {
    state->encryption_cinfo = 0;
  }
  if (status < 0) goto FAIL;

  /* Shut down the base64 encoder. */
  status = MIME_EncoderDestroy(state->crypto_encoder_data, PR_FALSE);
  state->crypto_encoder_data = 0;

  if (status < 0) goto FAIL;

 FAIL:
  msg_remember_security_error(state, status);
  return status;
}

extern char* ExtractRFC822AddressMailboxes (const char *line);
extern nsresult *RemoveDuplicateAddresses (const char *addrs,
										   const char *other_addrs,
										   PRBool removeAliasesToMe, char **newaddresses);
extern int ParseRFC822Addresses (const char *line,
									 char **names, char **addresses);


/* Used to figure out what certs should be used when encrypting this message.
 */
static int
mime_crypto_hack_certs(mime_crypto_closure *state, const char *recipients,
					   PRBool encrypt_p, PRBool sign_p)
{
  int status = 0;
  char *all_mailboxes = 0, *mailboxes = 0, *mailbox_list = 0;
  const char *mailbox = 0;
  PRUint32 count = 0;
  PRUint32 numCerts;
  nsCOMPtr<nsIX509CertDB> certdb = do_GetService(NS_X509CERTDB_CONTRACTID);
  nsCOMPtr<nsIMsgHeaderParser>  pHeader;
  nsresult res = nsComponentManager::CreateInstance(kMsgHeaderParserCID, 
                                                     NULL, NS_GET_IID(nsIMsgHeaderParser), 
                                                     (void **) getter_AddRefs(pHeader)); 
  res = NS_NewISupportsArray(getter_AddRefs(state->certs));
  if (NS_FAILED(res)) {
    return res;
  }

  PRBool no_clearsigning_p = PR_FALSE;

  PR_ASSERT(encrypt_p || sign_p);
  {
  certdb->GetDefaultEmailEncryptionCert(getter_AddRefs(state->self_encryption_cert));
  certdb->GetDefaultEmailSigningCert(getter_AddRefs(state->self_signing_cert));
	if ((state->self_signing_cert == nsnull) && sign_p)
	  {
		status = SEC_ERROR_NO_EMAIL_CERT;
		goto FAIL;
	  }
	if ((state->self_encryption_cert == nsnull) && encrypt_p) {
		status = SEC_ERROR_NO_EMAIL_CERT;
		goto FAIL;
	  }
  }

  pHeader->ExtractHeaderAddressMailboxes(nsnull,recipients, &all_mailboxes);
  pHeader->RemoveDuplicateAddresses(nsnull, all_mailboxes, 0, PR_FALSE /*removeAliasesToMe*/, &mailboxes);
  PR_FREEIF(all_mailboxes);

  if (mailboxes)
	  pHeader->ParseHeaderAddresses (nsnull, mailboxes, 0, &mailbox_list, &count);
  PR_FREEIF(mailboxes);
  if (count < 0) return count;

  /* If the message is to be encrypted, then get the recipient certs */
  if (encrypt_p) {
	  mailbox = mailbox_list;
	  for (; count > 0; count--) {
      nsCOMPtr<nsIX509Cert> cert;
		  certdb->GetCertByEmailAddress(nsnull, mailbox, getter_AddRefs(cert));
		  if (!cert) {
			  mailbox += nsCRT::strlen(mailbox) + 1;
			  continue;
		  }

	  /* #### see if recipient requests `signedData'.
		 if (...) no_clearsigning_p = PR_TRUE;
		 (This is the only reason we even bother looking up the certs
		 of the recipients if we're sending a signed-but-not-encrypted
		 message.)
	   */
      state->certs->AppendElement(cert);
		  mailbox += nsCRT::strlen(mailbox) + 1;
	  }
    state->certs->Count(&numCerts);
	  if (numCerts == 0) {
#if 0 // XXX Fix this XXX //
		/* If we wanted to encrypt but couldn't, ask the user whether they'd
		 rather just turn off encryption and send anyway. */
		if (FE_Confirm(state->context,
					 XP_GetString(SEC_ERROR_NO_RECIPIENT_CERTS_QUERY)))
			{
			status = 1;   /* this means "send unencrypted" */

			if (state->crypto_state == mime_crypto_signed_encrypted)
				state->crypto_state = mime_crypto_clear_signed;
			else
				{
				PR_ASSERT(state->crypto_state == mime_crypto_encrypted);
				state->crypto_state = mime_crypto_none;
				}
			}
		else
			{
			status = MK_INTERRUPTED;	/* "fail" without an error dialog */
			}
#endif
    status = 1; // XXX Fix this XXX //
		goto FAIL;
		}
	}
FAIL:
  PR_FREEIF(mailbox_list);

  if (status >= 0 && no_clearsigning_p &&
	  state->crypto_state == mime_crypto_clear_signed)
	state->crypto_state = mime_crypto_opaque_signed;

  if (status < 0)
  msg_remember_security_error(state, status);
  return status;
}



/* Used as the output function of a SEC_PKCS7EncoderContext -- we feed
   plaintext into the crypto engine, and it calls this function with encrypted
   data; then this function writes a base64-encoded representation of that
   data to the file (by filtering it through the given MimeEncoderData object.)

   Also used as the output function of SEC_PKCS7Encode() -- but in that case,
   it's used to write the encoded representation of the signature.  The only
   difference is which MimeEncoderData object is used.
 */
static void
mime_crypto_write_base64 (void *closure, const char *buf,
						  unsigned long size)
{
  MimeEncoderData *data = (MimeEncoderData *) closure;
  int status = MIME_EncoderWrite (data, buf, size);
  PR_SetError(status < 0 ? status : 0, 0);
}


/* Used as the output function of MimeEncoderData -- when we have generated
   the signature for a multipart/signed object, this is used to write the
   base64-encoded representation of the signature to the file.
 */
nsresult
mime_encoder_output_fn(const char *buf, PRInt32 size, void *closure)
{
  mime_crypto_closure *state = (mime_crypto_closure *) closure;
  if (PRInt32(state->file->write((char *) buf, size)) < size)
	return MK_MIME_ERROR_WRITING_FILE;
  else
	return 0;
}

/* Like mime_encoder_output_fn, except this is used for the case where we
   are both signing and encrypting -- the base64-encoded output of the
   signature should be fed into the crypto engine, rather than being written
   directly to the file.
 */
static nsresult
mime_nested_encoder_output_fn (const char *buf, PRInt32 size, void *closure)
{
  mime_crypto_closure *state = (mime_crypto_closure *) closure;
  return mime_crypto_write_block (state, (char *) buf, size);
}

/* Called by compose.c / msgsend.cpp to write the body of the message.
 */
nsresult
mime_crypto_write_block (void *closure, char *buf, PRInt32 size)
{
  mime_crypto_closure *state = (mime_crypto_closure *) closure;
  int status = 0;
  nsresult rv;

  PR_ASSERT(state);
  if (!state) return -1;

  /* If this is a From line, mangle it before signing it.  You just know
	 that something somewhere is going to mangle it later, and that's
	 going to cause the signature check to fail.

	 (This assumes that, in the cases where From-mangling must happen,
	 this function is called a line at a time.  That happens to be the
	 case.)
  */
  if (size >= 5 && buf[0] == 'F' && !nsCRT::strncmp(buf, "From ", 5))
	{
	  char mangle[] = ">";
	  status = mime_crypto_write_block (closure, mangle, 1);
	  if (status < 0)
		return status;
	}

  /* If we're signing, or signing-and-encrypting, feed this data into
	 the computation of the hash. */
  if (state->data_hash_context) {
	  PR_SetError(0,0);
	  state->data_hash_context->Update((unsigned char *) buf, size);
	  status = PR_GetError();
	  if (status < 0) goto FAIL;
	}

  PR_SetError(0,0);
  if (state->encryption_context) {
	  /* If we're encrypting, or signing-and-encrypting, write this data
		 by filtering it through the crypto library. */

	  rv = state->encryption_context->Update(buf, size);
    if (NS_FAILED(rv)) {
		  status = PR_GetError();
		  PR_ASSERT(status < 0);
		  if (status >= 0) status = -1;
		  goto FAIL;
		}
	}
  else
	{
	  /* If we're not encrypting (presumably just signing) then write this
		 data directly to the file. */

    if (PRInt32(state->file->write (buf, size)) < size) {
  		return MK_MIME_ERROR_WRITING_FILE;
    }
	}

 FAIL:
  msg_remember_security_error(state, status);
  return status;
}


#ifdef XP_MAC
#include <ConditionalMacros.h>

#pragma global_optimizer on
#pragma optimization_level 4

#endif


/* Crypto preferences
 */


/* Returns PR_TRUE if the user has selected the preference that says that new
   mail messages should be encrypted by default. 
 */
PRBool
MSG_GetMailEncryptionPreference(void)
{
  return PR_TRUE;
}

/* Returns PR_TRUE if the user has selected the preference that says that new
   mail messages should be cryptographically signed by default. 
 */
PRBool
MSG_GetMailSigningPreference(void)
{
  return PR_TRUE;
}

/* Returns PR_TRUE if the user has selected the preference that says that new
   news messages should be cryptographically signed by default. 
 */
PRBool
MSG_GetNewsSigningPreference(void)
{
  return PR_FALSE;
}


#ifdef XP_MAC
#pragma global_optimizer reset
#endif

