/* Insert copyright and license here 1996 */

#include "mimempkc.h"
#include "nspr.h"

#define MIME_SUPERCLASS mimeMultipartSignedClass
MimeDefClass(MimeMultipartSignedPKCS7, MimeMultipartSignedPKCS7Class,
			 mimeMultipartSignedPKCS7Class, &MIME_SUPERCLASS);

static int MimeMultipartSignedPKCS7_initialize (MimeObject *);

static void *MimeMultPKCS7_init (MimeObject *);
static int MimeMultPKCS7_data_hash (char *, PRInt32, void *);
static int MimeMultPKCS7_sig_hash  (char *, PRInt32, void *);
static int MimeMultPKCS7_data_eof (void *, PRBool);
static int MimeMultPKCS7_sig_eof  (void *, PRBool);
static int MimeMultPKCS7_sig_init (void *, MimeObject *, MimeHeaders *);
static char * MimeMultPKCS7_generate (void *);
static void MimeMultPKCS7_free (void *);
static void MimeMultPKCS7_get_content_info (MimeObject *,
											SEC_PKCS7ContentInfo **,
											char **, PRInt32 *, PRInt32 *, PRBool *);

extern int SEC_ERROR_CERT_ADDR_MISMATCH;

static int
MimeMultipartSignedPKCS7ClassInitialize(MimeMultipartSignedPKCS7Class *clazz)
{
  MimeObjectClass          *oclass = (MimeObjectClass *)    clazz;
  MimeMultipartSignedClass *sclass = (MimeMultipartSignedClass *) clazz;

  oclass->initialize  = MimeMultipartSignedPKCS7_initialize;

  sclass->crypto_init           = MimeMultPKCS7_init;
  sclass->crypto_data_hash      = MimeMultPKCS7_data_hash;
  sclass->crypto_data_eof       = MimeMultPKCS7_data_eof;
  sclass->crypto_signature_init = MimeMultPKCS7_sig_init;
  sclass->crypto_signature_hash = MimeMultPKCS7_sig_hash;
  sclass->crypto_signature_eof  = MimeMultPKCS7_sig_eof;
  sclass->crypto_generate_html  = MimeMultPKCS7_generate;
  sclass->crypto_free           = MimeMultPKCS7_free;

  clazz->get_content_info	    = MimeMultPKCS7_get_content_info;

  PR_ASSERT(!oclass->class_initialized);
  return 0;
}

static int
MimeMultipartSignedPKCS7_initialize (MimeObject *object)
{
  return ((MimeObjectClass*)&MIME_SUPERCLASS)->initialize(object);
}


typedef struct MimeMultPKCS7data {
  HASH_HashType hash_type;
  HASHContext *data_hash_context;
  SEC_PKCS7DecoderContext *sig_decoder_context;
  SEC_PKCS7ContentInfo *content_info;
  char *sender_addr;
  PRInt32 decode_error;
  PRInt32 verify_error;
  SECItem item;
  MimeObject *self;
  PRBool parent_is_encrypted_p;
  PRBool parent_holds_stamp_p;
} MimeMultPKCS7data;


static void
MimeMultPKCS7_get_content_info(MimeObject *obj,
							   SEC_PKCS7ContentInfo **content_info_ret,
							   char **sender_email_addr_return,
							   PRInt32 *decode_error_ret,
							   PRInt32 *verify_error_ret,
                               PRBool *ci_is_encrypted)
{
  MimeMultipartSigned *msig = (MimeMultipartSigned *) obj;
  if (msig && msig->crypto_closure)
	{
	  MimeMultPKCS7data *data = (MimeMultPKCS7data *) msig->crypto_closure;

	  *decode_error_ret = data->decode_error;
	  *verify_error_ret = data->verify_error;
	  *content_info_ret = data->content_info;
      *ci_is_encrypted     = PR_FALSE;

	  if (sender_email_addr_return)
		*sender_email_addr_return = (data->sender_addr
                   ? nsCRT::strdup(data->sender_addr)
									 : 0);
	}
}

/* #### MimeEncryptedPKCS7 and MimeMultipartSignedPKCS7 have a sleazy,
        incestuous, dysfunctional relationship. */
extern PRBool MimeEncryptedPKCS7_encrypted_p (MimeObject *obj);
extern PRBool MimePKCS7HeadersAndCertsMatch(MimeObject *obj,
											 SEC_PKCS7ContentInfo *,
											 char **);
extern char *MimePKCS7_MakeSAURL(MimeObject *obj);
extern char *IMAP_CreateReloadAllPartsUrl(const char *url);

static void *
MimeMultPKCS7_init (MimeObject *obj)
{
  MimeHeaders *hdrs = obj->headers;
  MimeMultPKCS7data *data = 0;
  char *ct, *micalg;
  HASH_HashType hash_type;

  ct = MimeHeaders_get (hdrs, HEADER_CONTENT_TYPE, PR_FALSE, PR_FALSE);
  if (!ct) return 0; /* #### bogus message?  out of memory? */
  micalg = MimeHeaders_get_parameter (ct, PARAM_MICALG, NULL, NULL);
  PR_FREE(ct);
  ct = 0;
  if (!micalg) return 0; /* #### bogus message?  out of memory? */

  if (!strcasecomp(micalg, PARAM_MICALG_MD5))
	hash_type = HASH_AlgMD5;
  else if (!strcasecomp(micalg, PARAM_MICALG_SHA1) ||
		   !strcasecomp(micalg, PARAM_MICALG_SHA1_2) ||
		   !strcasecomp(micalg, PARAM_MICALG_SHA1_3) ||
		   !strcasecomp(micalg, PARAM_MICALG_SHA1_4) ||
		   !strcasecomp(micalg, PARAM_MICALG_SHA1_5))
	hash_type = HASH_AlgSHA1;
  else if (!strcasecomp(micalg, PARAM_MICALG_MD2))
	hash_type = HASH_AlgMD2;
  else
	hash_type = HASH_AlgNULL;

  PR_FREE(micalg);
  micalg = 0;

  if (hash_type == HASH_AlgNULL) return 0; /* #### bogus message? */

  data = (MimeMultPKCS7data *) PR_MALLOC(sizeof(*data));
  if (!data) return 0;

  nsCRT::memset(data, 0, sizeof(*data));

  data->self = obj;
  data->hash_type = hash_type;

  PR_ASSERT(!data->data_hash_context);
  PR_ASSERT(!data->sig_decoder_context);

  data->data_hash_context = HASH_Create(data->hash_type);

  PR_ASSERT(data->data_hash_context);
  if (!data->data_hash_context)
	{
	  PR_FREE(data);
	  return 0;
	}

  PR_SetError(0,0);
  HASH_Begin(data->data_hash_context);
  if (!data->decode_error)
	{
	  data->decode_error = PR_GetError();
	  if (data->decode_error)
		{
		  PR_FREE(data);
		  return 0;
		}
	}

  data->parent_holds_stamp_p =
	(obj->parent && mime_crypto_stamped_p(obj->parent));

  data->parent_is_encrypted_p =
	(obj->parent && MimeEncryptedPKCS7_encrypted_p (obj->parent));

  /* If the parent of this object is a crypto-blob, then it's the grandparent
	 who would have written out the headers and prepared for a stamp...
	 (This s##t s$%#s.)
   */
  if (data->parent_is_encrypted_p &&
	  !data->parent_holds_stamp_p &&
	  obj->parent && obj->parent->parent)
	data->parent_holds_stamp_p =
	  mime_crypto_stamped_p (obj->parent->parent);

  return data;
}

static int
MimeMultPKCS7_data_hash (char *buf, PRInt32 size, void *crypto_closure)
{
  MimeMultPKCS7data *data = (MimeMultPKCS7data *) crypto_closure;
  PR_ASSERT(data && data->data_hash_context);
  if (!data || !data->data_hash_context) return -1;

  PR_ASSERT(!data->sig_decoder_context);

  PR_SetError(0, 0);
  HASH_Update(data->data_hash_context, (unsigned char *) buf, size);
  if (!data->verify_error)
	data->verify_error = PR_GetError();

  return 0;
}

static int
MimeMultPKCS7_data_eof (void *crypto_closure, PRBool abort_p)
{
  MimeMultPKCS7data *data = (MimeMultPKCS7data *) crypto_closure;
  PR_ASSERT(data && data->data_hash_context);
  if (!data || !data->data_hash_context) return -1;

  PR_ASSERT(!data->sig_decoder_context);

  data->item.len = HASH_ResultLen(data->hash_type);
  data->item.data = (unsigned char *) PR_MALLOC(data->item.len);
  if (!data->item.data) return MK_OUT_OF_MEMORY;

  PR_SetError(0, 0);
  HASH_End(data->data_hash_context, data->item.data, &data->item.len,
		   data->item.len);
  if (!data->verify_error)
	data->verify_error = PR_GetError();

  HASH_Destroy(data->data_hash_context);
  data->data_hash_context = 0;

  /* At this point, data->item.data contains a digest for the first part.
	 When we process the signature, the security library will compare this
	 digest to what's in the signature object. */

  return 0;
}


static int
MimeMultPKCS7_sig_init (void *crypto_closure,
						MimeObject *multipart_object,
						MimeHeaders *signature_hdrs)
{
  MimeMultPKCS7data *data = (MimeMultPKCS7data *) crypto_closure;
  MimeDisplayOptions *opts = multipart_object->options;
  char *ct;
  int status = 0;

  PR_ASSERT(!data->data_hash_context);
  PR_ASSERT(!data->sig_decoder_context);

  PR_ASSERT(signature_hdrs);
  if (!signature_hdrs) return -1;

  ct = MimeHeaders_get (signature_hdrs, HEADER_CONTENT_TYPE, PR_TRUE, PR_FALSE);

  /* Verify that the signature object is of the right type. */
  if (!ct || (strcasecomp(ct, APPLICATION_XPKCS7_SIGNATURE) &&
              strcasecomp(ct, APPLICATION_PKCS7_SIGNATURE)))
	status = -1; /* #### error msg about bogus message */
  PR_FREEIF(ct);
  if (status < 0) return status;

  data->sig_decoder_context =
	SEC_PKCS7DecoderStart(0, 0, /* no content cb */
						  ((SECKEYGetPasswordKey) opts->passwd_prompt_fn),
						  opts->passwd_prompt_fn_arg,
						  NULL, NULL, 
						  SECMIME_DecryptionAllowed);
  if (!data->sig_decoder_context)
	{
	  status = PR_GetError();
	  PR_ASSERT(status < 0);
	  if (status >= 0) status = -1;
	}
  return status;
}


static int
MimeMultPKCS7_sig_hash (char *buf, PRInt32 size, void *crypto_closure)
{
  MimeMultPKCS7data *data = (MimeMultPKCS7data *) crypto_closure;

  PR_ASSERT(data && data->sig_decoder_context);
  if (!data || !data->sig_decoder_context) return -1;

  PR_ASSERT(!data->data_hash_context);

  if (SECSuccess != SEC_PKCS7DecoderUpdate(data->sig_decoder_context, 
  											buf, size))
	{
	  if (!data->verify_error)
		data->verify_error = PR_GetError();
	  PR_ASSERT(data->verify_error < 0);
	  if (data->verify_error >= 0)
		data->verify_error = -1;
	}

  return 0;
}

static int
MimeMultPKCS7_sig_eof (void *crypto_closure, PRBool abort_p)
{
  MimeMultPKCS7data *data = (MimeMultPKCS7data *) crypto_closure;

  if (!data) return -1;

  PR_ASSERT(!data->data_hash_context);

  /* Hand an EOF to the crypto library.

	 We save away the value returned and will use it later to emit a
	 blurb about whether the signature validation was cool.
   */

  PR_ASSERT(!data->content_info);

  if (data->sig_decoder_context)
	{
	  data->content_info = SEC_PKCS7DecoderFinish (data->sig_decoder_context);
	  data->sig_decoder_context = 0;
	  if (!data->content_info && !data->verify_error)
		data->verify_error = PR_GetError();

	  PR_ASSERT(data->content_info ||
				data->verify_error || data->decode_error);
	}

  return 0;
}



static void
MimeMultPKCS7_free (void *crypto_closure)
{
  MimeMultPKCS7data *data = (MimeMultPKCS7data *) crypto_closure;
  PR_ASSERT(data);
  if (!data) return;

  PR_FREEIF(data->sender_addr);

  if (data->data_hash_context)
	{
	  HASH_Destroy(data->data_hash_context);
	  data->data_hash_context = 0;
	}

  if (data->sig_decoder_context)
	{
	  SEC_PKCS7ContentInfo *cinfo =
		SEC_PKCS7DecoderFinish (data->sig_decoder_context);
	  if (cinfo)
		SEC_PKCS7DestroyContentInfo(cinfo);
	}

  if (data->content_info)
	{
	  SEC_PKCS7DestroyContentInfo(data->content_info);
	  data->content_info = 0;
	}

  PR_FREEIF(data->item.data);

  PR_FREE(data);
}


static char *
MimeMultPKCS7_generate (void *crypto_closure)
{
  MimeMultPKCS7data *data = (MimeMultPKCS7data *) crypto_closure;
  PRBool signed_p = PR_TRUE;
  PRBool good_p = PR_TRUE;
  PRBool encrypted_p;
  PRBool unverified_p = PR_FALSE;

  PR_ASSERT(data);
  if (!data) return 0;
  encrypted_p = data->parent_is_encrypted_p;

  if (data->content_info)
	{
	  good_p =
		SEC_PKCS7VerifyDetachedSignature(data->content_info,
										 certUsageEmailSigner,
										 &data->item,
										 data->hash_type,
										 PR_TRUE);  /* #### keepcerts */
	  if (!good_p)
		{
		  if (!data->verify_error)
			data->verify_error = PR_GetError();
		  PR_ASSERT(data->verify_error < 0);
		  if (data->verify_error >= 0)
			data->verify_error = -1;
		}
	  else
		{
		  good_p = MimePKCS7HeadersAndCertsMatch(data->self,
												 data->content_info,
												 &data->sender_addr);
		  if (!good_p && !data->verify_error)
			data->verify_error = SEC_ERROR_CERT_ADDR_MISMATCH;
		}

	  if (SEC_PKCS7ContainsCertsOrCrls(data->content_info))
		{
		  /* #### call libsec telling it to import the certs */
		}

	  /* Don't free these yet -- keep them around for the lifetime of the
		 MIME object, so that we can get at the security info of sub-parts
		 of the currently-displayed message. */
#if 0
	  SEC_PKCS7DestroyContentInfo(data->content_info);
	  data->content_info = 0;
#endif /* 0 */
	}
  else
	{
	  /* No content_info at all -- since we're inside a multipart/signed,
		 that means that we've either gotten a message that was truncated
		 before the signature part, or we ran out of memory, or something
		 awful has happened.  Anyway, it sure ain't good_p.
	   */
	  good_p = PR_FALSE;
	}

  unverified_p = data->self->options->missing_parts; 

  PR_ASSERT(data->self);
  if (data->self && data->self->parent)
	mime_set_crypto_stamp(data->self->parent, signed_p, encrypted_p);


  {
	char *stamp_url = 0, *result;
	if (data->self)
	{
		if (unverified_p && data->self->options)
			stamp_url = IMAP_CreateReloadAllPartsUrl(data->self->options->url);
		else
			stamp_url = MimePKCS7_MakeSAURL(data->self);
	}

	result =
	  MimeHeaders_make_crypto_stamp (encrypted_p, signed_p, good_p,
									 unverified_p,
									 data->parent_holds_stamp_p,
									 stamp_url);
	PR_FREEIF(stamp_url);
	return result;
  }
}
