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
 * The Original Code is Mozilla IPC.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Darin Fisher <darin@netscape.com>
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

#include "nsIServiceManager.h"
#include "nsIObserverService.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIFile.h"
#include "nsIProcess.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsEventQueueUtils.h"
#include "nsAutoLock.h"
#include "nsNetCID.h"
#include "netCore.h"
#include "prerror.h"
#include "plstr.h"

#include "ipcConfig.h"
#include "ipcLog.h"
#include "ipcMessageUtils.h"
#include "ipcTransport.h"
#include "ipcm.h"

//-----------------------------------------------------------------------------
// ipcTransport
//-----------------------------------------------------------------------------

nsresult
ipcTransport::Init(ipcTransportObserver *obs)
{
    LOG(("ipcTransport::Init\n"));

    mObserver = obs;

    nsresult rv = PlatformInit();
    if (NS_FAILED(rv)) return rv;

    return Connect();
}

nsresult
ipcTransport::Shutdown()
{
    LOG(("ipcTransport::Shutdown\n"));

    mObserver = 0;
    return Disconnect();
}

nsresult
ipcTransport::SendMsg(ipcMessage *msg, PRBool sync)
{
    NS_ENSURE_ARG_POINTER(msg);
    NS_ENSURE_TRUE(mObserver, NS_ERROR_NOT_INITIALIZED);

    LOG(("ipcTransport::SendMsg [msg=%p dataLen=%u]\n", msg, msg->DataLen()));

    ipcMessage *syncReply = nsnull;
    {
        nsAutoMonitor mon(mMonitor);
        nsresult rv;

        if (sync) {
            msg->SetFlag(IPC_MSG_FLAG_SYNC_QUERY);
            // flag before sending to avoid race with background thread.
            mSyncWaiting = PR_TRUE;
        }

        if (mHaveConnection) {
            rv = SendMsg_Internal(msg);
            if (NS_FAILED(rv)) return rv;
        }
        else {
            LOG(("  delaying message until connected\n"));
            mDelayedQ.Append(msg);
        }

        if (sync) {
            if (!mSyncReplyMsg) {
                LOG(("  waiting for response...\n"));
                mon.Wait();
            }

            if (!mSyncReplyMsg) {
                LOG(("  sync request timed out or was canceled\n"));
                return NS_ERROR_FAILURE;
            }

            syncReply = mSyncReplyMsg;
            mSyncReplyMsg = nsnull;
        }
    }
    if (syncReply) {
        // NOTE: may re-enter SendMsg
        mObserver->OnMessageAvailable(syncReply);
        delete syncReply;
    }
    return NS_OK;
}

void
ipcTransport::ProcessIncomingMsgQ()
{
    LOG(("ipcTransport::ProcessIncomingMsgQ\n"));

    // we can't hold mMonitor while calling into the observer, so we grab
    // mIncomingMsgQ and NULL it out inside the monitor to prevent others
    // from modifying it while we iterate over it.
    ipcMessageQ *inQ;
    {
        nsAutoMonitor mon(mMonitor);
        inQ = mIncomingMsgQ;
        mIncomingMsgQ = nsnull;
    }
    if (inQ) {
        while (!inQ->IsEmpty()) {
            ipcMessage *msg = inQ->First();
            if (mObserver)
                mObserver->OnMessageAvailable(msg);
            inQ->DeleteFirst();
        }
        delete inQ;
    }
}

void *
ipcTransport::ProcessIncomingMsgQ_EventHandler(PLEvent *ev)
{
    ipcTransport *self = (ipcTransport *) PL_GetEventOwner(ev);
    self->ProcessIncomingMsgQ();
    return nsnull;
}

void *
ipcTransport::ConnectionEstablished_EventHandler(PLEvent *ev)
{
    ipcTransport *self = (ipcTransport *) PL_GetEventOwner(ev);
    if (self->mObserver)
        self->mObserver->OnConnectionEstablished(self->mClientID);
    return nsnull;
}

void *
ipcTransport::ConnectionLost_EventHandler(PLEvent *ev)
{
    ipcTransport *self = (ipcTransport *) PL_GetEventOwner(ev);
    if (self->mObserver)
        self->mObserver->OnConnectionLost();
    return nsnull;
}

void
ipcTransport::Generic_EventCleanup(PLEvent *ev)
{
    ipcTransport *self = (ipcTransport *) PL_GetEventOwner(ev);
    NS_RELEASE(self);
    delete ev;
}

// called on a background thread
void
ipcTransport::OnMessageAvailable(ipcMessage *rawMsg)
{
    LOG(("ipcTransport::OnMessageAvailable [msg=%p dataLen=%u]\n",
        rawMsg, rawMsg->DataLen()));

    //
    // XXX FIX COMMENTS XXX
    //
    // 1- append to incoming message queue
    //
    // 2- post event to main thread to handle incoming message queue
    //    or if sync waiting, unblock waiter so it can scan incoming
    //    message queue.
    //

    PRBool dispatchEvent = PR_FALSE;
    PRBool connectEvent = PR_FALSE;
    {
        nsAutoMonitor mon(mMonitor);

        if (!mHaveConnection) {
            if (rawMsg->Target().Equals(IPCM_TARGET)) {
                if (IPCM_GetMsgType(rawMsg) == IPCM_MSG_TYPE_CLIENT_ID) {
                    LOG(("  connection established!\n"));
                    mHaveConnection = PR_TRUE;

                    // remember our client ID
                    ipcMessageCast<ipcmMessageClientID> msg(rawMsg);
                    mClientID = msg->ClientID();
                    connectEvent = PR_TRUE;

                    // move messages off the delayed message queue
                    while (!mDelayedQ.IsEmpty()) {
                        ipcMessage *msg = mDelayedQ.First();
                        mDelayedQ.RemoveFirst();
                        SendMsg_Internal(msg);
                    }
                    return;
                }
            }
            LOG(("  received unexpected first message!\n"));
            return;
        }

        LOG(("  mSyncWaiting=%u MSG_FLAG_SYNC_REPLY=%u\n",
             mSyncWaiting, rawMsg->TestFlag(IPC_MSG_FLAG_SYNC_REPLY) != 0));

        if (mSyncWaiting && rawMsg->TestFlag(IPC_MSG_FLAG_SYNC_REPLY)) {
            mSyncReplyMsg = rawMsg;
            mSyncWaiting = PR_FALSE;
            mon.Notify();
        }
        else {
            if (!mIncomingMsgQ) {
                mIncomingMsgQ = new ipcMessageQ();
                if (!mIncomingMsgQ)
                    return;
                dispatchEvent = PR_TRUE;
            }
            mIncomingMsgQ->Append(rawMsg);
        }

        LOG(("  connectEvent=%u dispatchEvent=%u mSyncReplyMsg=%p mIncomingMsgQ=%p\n",
            connectEvent, dispatchEvent, mSyncReplyMsg, mIncomingMsgQ));
    }

    if (connectEvent)
        ProxyToMainThread(ConnectionEstablished_EventHandler);
    if (dispatchEvent)
        ProxyToMainThread(ProcessIncomingMsgQ_EventHandler);
}

void
ipcTransport::ProxyToMainThread(PLHandleEventProc proc)
{
    LOG(("ipcTransport::ProxyToMainThread\n"));

    nsCOMPtr<nsIEventQueue> eq;
    NS_GetMainEventQ(getter_AddRefs(eq));
    if (eq) {
        PLEvent *ev = new PLEvent();
        PL_InitEvent(ev, this, proc, Generic_EventCleanup);
        NS_ADDREF_THIS();
        if (eq->PostEvent(ev) == PR_FAILURE) {
            LOG(("  PostEvent failed"));
            NS_RELEASE_THIS();
            delete ev;
        }
    }
}

nsresult
ipcTransport::SpawnDaemon()
{
    LOG(("ipcTransport::SpawnDaemon\n"));

    nsresult rv;
    nsCOMPtr<nsIFile> file;

    rv = NS_GetSpecialDirectory(NS_XPCOM_CURRENT_PROCESS_DIR, getter_AddRefs(file));
    if (NS_FAILED(rv)) return rv;

    rv = file->AppendNative(NS_LITERAL_CSTRING(IPC_DAEMON_APP_NAME));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIProcess> proc(do_CreateInstance(NS_PROCESS_CONTRACTID,&rv));
    if (NS_FAILED(rv)) return rv;

    rv = proc->Init(file);
    if (NS_FAILED(rv)) return rv;

    PRUint32 pid;
    return proc->Run(PR_FALSE, nsnull, 0, &pid);
}

// called on a background thread
nsresult
ipcTransport::OnConnectFailure()
{
    LOG(("ipcTransport::OnConnectFailure\n"));

    nsresult rv;
    if (!mSpawnedDaemon) {
        //
        // spawn daemon on connection failure
        //
        rv = SpawnDaemon();
        if (NS_FAILED(rv)) {
            LOG(("  failed to spawn daemon [rv=%x]\n", rv));
            return rv;
        }
        mSpawnedDaemon = PR_TRUE;
    }

    Disconnect();

    PRUint32 ms = 50 * mConnectionAttemptCount;
    LOG(("  sleeping for %u ms...\n", ms));
    PR_Sleep(PR_MillisecondsToInterval(ms));

    Connect();
    return NS_OK;
}

#ifdef XP_UNIX
NS_IMPL_THREADSAFE_ISUPPORTS1(ipcTransport, nsISocketEventHandler)
#else
NS_IMPL_THREADSAFE_ISUPPORTS0(ipcTransport)
#endif
