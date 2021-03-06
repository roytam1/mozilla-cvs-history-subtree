/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
/*******************************************************************************
 * Source date: 9 Apr 1997 21:45:12 GMT
 * netscape/fonts/nff public interface
 * Generated by jmc version 1.8 -- DO NOT EDIT
 ******************************************************************************/

#ifndef _Mnff_H_
#define _Mnff_H_

#include "jmc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*******************************************************************************
 * nff
 ******************************************************************************/

/* The type of the nff interface. */
struct nffInterface;

/* The public type of a nff instance. */
typedef struct nff {
	const struct nffInterface*	vtable;
} nff;

/* The inteface ID of the nff interface. */
#ifndef JMC_INIT_nff_ID
extern EXTERN_C_WITHOUT_EXTERN const JMCInterfaceID nff_ID;
#else
EXTERN_C const JMCInterfaceID nff_ID = { 0x4f477462, 0x12081671, 0x50135b6a, 0x37743b64 };
#endif /* JMC_INIT_nff_ID */
/*******************************************************************************
 * nff Operations
 ******************************************************************************/

#define nff_getInterface(self, a, exception)	\
	(((self)->vtable->getInterface)(self, nff_getInterface_op, a, exception))

#define nff_addRef(self, exception)	\
	(((self)->vtable->addRef)(self, nff_addRef_op, exception))

#define nff_release(self, exception)	\
	(((self)->vtable->release)(self, nff_release_op, exception))

#define nff_hashCode(self, exception)	\
	(((self)->vtable->hashCode)(self, nff_hashCode_op, exception))

#define nff_equals(self, a, exception)	\
	(((self)->vtable->equals)(self, nff_equals_op, a, exception))

#define nff_clone(self, exception)	\
	(((self)->vtable->clone)(self, nff_clone_op, exception))

#define nff_toString(self, exception)	\
	(((self)->vtable->toString)(self, nff_toString_op, exception))

#define nff_finalize(self, exception)	\
	(((self)->vtable->finalize)(self, nff_finalize_op, exception))

#define nff_GetState(self, exception)	\
	(((self)->vtable->GetState)(self, nff_GetState_op, exception))

#define nff_EnumerateSizes(self, a, exception)	\
	(((self)->vtable->EnumerateSizes)(self, nff_EnumerateSizes_op, a, exception))

#define nff_GetRenderableFont(self, a, b, exception)	\
	(((self)->vtable->GetRenderableFont)(self, nff_GetRenderableFont_op, a, b, exception))

#define nff_GetMatchInfo(self, a, b, exception)	\
	(((self)->vtable->GetMatchInfo)(self, nff_GetMatchInfo_op, a, b, exception))

#define nff_GetRcMajorType(self, exception)	\
	(((self)->vtable->GetRcMajorType)(self, nff_GetRcMajorType_op, exception))

#define nff_GetRcMinorType(self, exception)	\
	(((self)->vtable->GetRcMinorType)(self, nff_GetRcMinorType_op, exception))

/*******************************************************************************
 * nff Interface
 ******************************************************************************/

struct netscape_jmc_JMCInterfaceID;
struct java_lang_Object;
struct java_lang_String;
struct netscape_fonts_nfrc;
struct netscape_fonts_nfrf;
struct netscape_fonts_nffmi;

struct nffInterface {
	void*	(*getInterface)(struct nff* self, jint op, const JMCInterfaceID* a, JMCException* *exception);
	void	(*addRef)(struct nff* self, jint op, JMCException* *exception);
	void	(*release)(struct nff* self, jint op, JMCException* *exception);
	jint	(*hashCode)(struct nff* self, jint op, JMCException* *exception);
	jbool	(*equals)(struct nff* self, jint op, void* a, JMCException* *exception);
	void*	(*clone)(struct nff* self, jint op, JMCException* *exception);
	const char*	(*toString)(struct nff* self, jint op, JMCException* *exception);
	void	(*finalize)(struct nff* self, jint op, JMCException* *exception);
	jint	(*GetState)(struct nff* self, jint op, JMCException* *exception);
	void*	(*EnumerateSizes)(struct nff* self, jint op, struct nfrc* a, JMCException* *exception);
	struct nfrf*	(*GetRenderableFont)(struct nff* self, jint op, struct nfrc* a, jdouble b, JMCException* *exception);
	struct nffmi*	(*GetMatchInfo)(struct nff* self, jint op, struct nfrc* a, jint b, JMCException* *exception);
	jint	(*GetRcMajorType)(struct nff* self, jint op, JMCException* *exception);
	jint	(*GetRcMinorType)(struct nff* self, jint op, JMCException* *exception);
};

/*******************************************************************************
 * nff Operation IDs
 ******************************************************************************/

typedef enum nffOperations {
	nff_getInterface_op,
	nff_addRef_op,
	nff_release_op,
	nff_hashCode_op,
	nff_equals_op,
	nff_clone_op,
	nff_toString_op,
	nff_finalize_op,
	nff_GetState_op,
	nff_EnumerateSizes_op,
	nff_GetRenderableFont_op,
	nff_GetMatchInfo_op,
	nff_GetRcMajorType_op,
	nff_GetRcMinorType_op
} nffOperations;

/******************************************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* _Mnff_H_ */
