/* ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is the Mozilla capabilities security API.
 *
 * The Initial Developer of the Original Code is
 * Benjamin Smedberg <benjamin@smedbergs.us>.
 *
 * Portions created by the Initial Developer are Copyright (C) 2005
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

#ifndef capsAPI_h__
#define capsAPI_h__

#include "nscore.h"
#include "nsError.h"
#include "nsISupports.h"

class capsIEvidence;
class capsIPermission;
class nsIPrompt;

#ifdef IMPL_CAPS_API
#define CAPS_API NS_EXPORT
#else
#define CAPS_API NS_IMPORT
#endif

/**
 * CAPS Error codes (nsresult)
 */
#define NS_ERROR_CAPS_ACCESS_DENIED \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_CAPS, 0)

/**
 * To avoid making frequent interface method calls, the CAPS symbols
 * do not take the capsI* interfaces pointers directly, but instead make use of
 * cacheing wrappers.
 */
class CAPSWrapper
{
public:
  virtual nsrefcnt AddRef() = 0;
  virtual nsrefcnt Release() = 0;

protected:
  CAPSWrapper() { }
  ~CAPSWrapper() { }
};

typedef CAPSWrapper CAPSEvidence;
typedef CAPSWrapper CAPSPermission;
typedef CAPSWrapper CAPSSavedStack;

/**
 * Obtain a CAPSEvidence wrapper from a capsIEvidence interface.
 *
 * @param aResult Out-param will return an already-addrefed CAPSEvidence.
 * @throws NS_ERROR_OUT_OF_MEMORY in OOM conditions.
 */
extern "C" CAPS_API nsresult
CAPS_GetEvidence(capsIEvidence* aEvidence, CAPSEvidence* *aResult);


/**
 * Obtain the CAPSEvidence wrapper for the "system" evidence (omnipotent).
 */
extern "C" CAPS_API nsresult
CAPS_GetSystemEvidence(CAPSEvidence* *aResult);


/**
 * Obtain the CAPSEvidence wrapper for the "null" evidence (impotent).
 */
extern "C" CAPS_API nsresult
CAPS_GetNullEvidence(CAPSEvidence* *aResult);


/**
 * Obtain a CAPSPermission wrapper from a capsIPermission.
 *
 * @param aResult Out-param will return an already-addrefed CAPSEvidence.
 * @throws NS_ERROR_OUT_OF_MEMORY in OOM conditions.
 */
extern "C" CAPS_API nsresult
CAPS_GetPermission(capsIPermission* aPermission, CAPSPermission* *aResult);


/**
 * An opaque structure identifying a CAPS stack frame.
 */
struct CAPSStackFrame;


/**
 * Begin a CAPS stack by transitioning from unsecured to secured execution.
 *
 * @param aEvidence The initial evidence to apply to the thread.
 * @return Opaque stack frame pointer, or null in OOM conditions.
 */
extern "C" CAPS_API CAPSStackFrame*
CAPS_Begin(CAPSEvidence* evidence);


/**
 * Begin a CAPS stack using a saved stack frame (created with CAPS_SaveStack).
 * @note The CAPS system holds a reference to the CAPSSavedStack while it is
 *       in use: client code can release it after this call if it is no longer
 *       needed.
 */
extern "C" CAPS_API CAPSStackFrame*
CAPS_Load(CAPSSavedStack* aStack);


/**
 * Save the CAPS stack for later use with CAPS_Load.
 */
extern "C" CAPS_API nsresult
CAPS_Save(CAPSSavedStack* *aResult);


/**
 * Enter a CAPS stack frame.
 *
 * @param aEvidence the new evidence to apply.
 * @param aAssert True if the frame should automatically obtain elevated
 *                privileges; false if an explicit call to CAPS_Assert is
 *                required to obtain elevated privileges.
 * @return Opaque stack frame pointer, or null in extreme OOM conditions.
 */
extern "C" CAPS_API CAPSStackFrame*
CAPS_Enter(CAPSEvidence* aEvidence, PRBool aAssert);


/**
 * Suspend the CAPS stack (to enter unsecured code). CAPS_Resume or
 * CAPS_Begin must be called before any more CAPS stack calls can be made.
 */
extern "C" CAPS_API void
CAPS_Suspend(CAPSStackFrame* aFrame);


/**
 * Resume the CAPS stack after CAPS_Suspend.
 */
extern "C" CAPS_API void
CAPS_Resume(CAPSStackFrame* aFrame);


/**
 * Exit a CAPS stack frame from CAPS_Begin or CAPS_Enter.
 */
extern "C" CAPS_API void
CAPS_Exit(CAPSStackFrame* aFrame);


/**
 * Assert that running code should have the specified privilege.
 */
extern "C" CAPS_API nsresult
CAPS_Assert(CAPSStackFrame* aFrame, CAPSPermission* aPermission,
            PRBool aShowUI, nsIPrompt* aPrompt);


/**
 * Revert an permission asserted with CAPS_Assert.
 */
extern "C" CAPS_API void
CAPS_RevertAssert(CAPSStackFrame* aFrame, CAPSPermission* aPermission);


/**
 * Deny the specified privilege to code in the current frame.
 */
extern "C" CAPS_API void
CAPS_Deny(CAPSStackFrame* aFrame, CAPSPermission* aPermission);


/**
 * Revert an permission denied with CAPS_Deny.
 */
extern "C" CAPS_API void
CAPS_RevertDeny(CAPSStackFrame* aFrame, CAPSPermission* aPermission);


/**
 * Check whether a privilege is enabled.
 *
 * @param aShowUI If true, and the user must be asked whether to enable
 *                the privilege, a synchronous user prompt will be shown.
 * @param aPrompt The specified prompt interface will be used to show the
 *                security dialog. If null, a default prompt service will
 *                be used.
 */
extern "C" CAPS_API nsresult
CAPS_Demand(CAPSStackFrame* aFrame, CAPSPermission* aPermission,
	    PRBool aShowUI, nsIPrompt* aPrompt);

#endif // capsAPI_h__
