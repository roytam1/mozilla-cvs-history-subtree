/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM nsIAccessibleDocumentInternal.idl
 */

#ifndef __gen_nsIAccessibleDocumentInternal_h__
#define __gen_nsIAccessibleDocumentInternal_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

#ifndef __gen_nsIAccessible_h__
#include "nsIAccessible.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
class nsIDocument; /* forward declaration */


/* starting interface:    nsIAccessibleDocumentInternal */
#define NS_IACCESSIBLEDOCUMENTINTERNAL_IID_STR "79d31a0f-a798-4e2f-bc8d-bdafc57071b8"

#define NS_IACCESSIBLEDOCUMENTINTERNAL_IID \
  {0x79d31a0f, 0xa798, 0x4e2f, \
    { 0xbc, 0x8d, 0xbd, 0xaf, 0xc5, 0x70, 0x71, 0xb8 }}

class NS_NO_VTABLE nsIAccessibleDocumentInternal : public nsISupports {
 public: 

  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IACCESSIBLEDOCUMENTINTERNAL_IID)

  /* nsIDocument getDocument (); */
  NS_IMETHOD GetDocument(nsIDocument **_retval) = 0;

};

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIACCESSIBLEDOCUMENTINTERNAL \
  NS_IMETHOD GetDocument(nsIDocument **_retval); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIACCESSIBLEDOCUMENTINTERNAL(_to) \
  NS_IMETHOD GetDocument(nsIDocument **_retval) { return _to ## GetDocument(_retval); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIACCESSIBLEDOCUMENTINTERNAL(_to) \
  NS_IMETHOD GetDocument(nsIDocument **_retval) { return !_to ## ? NS_ERROR_NULL_POINTER : _to ##-> GetDocument(_retval); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsAccessibleDocumentInternal : public nsIAccessibleDocumentInternal
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIACCESSIBLEDOCUMENTINTERNAL

  nsAccessibleDocumentInternal();
  virtual ~nsAccessibleDocumentInternal();
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsAccessibleDocumentInternal, nsIAccessibleDocumentInternal)

nsAccessibleDocumentInternal::nsAccessibleDocumentInternal()
{
  NS_INIT_ISUPPORTS();
  /* member initializers and constructor code */
}

nsAccessibleDocumentInternal::~nsAccessibleDocumentInternal()
{
  /* destructor code */
}

/* nsIDocument getDocument (); */
NS_IMETHODIMP nsAccessibleDocumentInternal::GetDocument(nsIDocument **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIAccessibleDocumentInternal_h__ */
