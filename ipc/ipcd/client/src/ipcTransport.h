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

#ifndef ipcTransport_h__
#define ipcTransport_h__

#include "nsIObserver.h"
#include "nsITimer.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "prmon.h"

#include "ipcMessage.h"
#include "ipcMessageQ.h"

#ifdef XP_UNIX
#include "ipcTransportUnix.h"
typedef nsISocketEventHandler ipcTransportSuper;
#else
typedef nsISupports ipcTransportSuper;
#endif

//----------------------------------------------------------------------------
// ipcTransportObserver interface
//----------------------------------------------------------------------------

class ipcTransportObserver
{
public:
    virtual void OnConnectionEstablished(PRUint32 clientID) = 0;
    virtual void OnConnectionLost() = 0;
    virtual void OnMessageAvailable(const ipcMessage *) = 0;
};

//-----------------------------------------------------------------------------
// ipcTransport
//-----------------------------------------------------------------------------

class ipcTransport : public ipcTransportSuper
{
public:
    NS_DECL_ISUPPORTS

    ipcTransport()
        : mMonitor(PR_NewMonitor())
        , mObserver(nsnull)
        , mIncomingMsgQ(nsnull)
        , mSyncReplyMsg(nsnull)
        , mSyncWaiting(nsnull)
        , mSentHello(PR_FALSE)
        , mHaveConnection(PR_FALSE)
        , mSpawnedDaemon(PR_FALSE)
        , mConnectionAttemptCount(0)
        , mClientID(0)
        {}

    virtual ~ipcTransport()
    {
        PR_DestroyMonitor(mMonitor);
#ifdef XP_UNIX
        if (mReceiver)
            ((ipcReceiver *) mReceiver.get())->ClearTransport();
#endif
    }

    nsresult Init(ipcTransportObserver *observer);
    nsresult Shutdown();

    // takes ownership of |msg|
    nsresult SendMsg(ipcMessage *msg, PRBool sync = PR_FALSE);

    PRBool   HaveConnection() const { return mHaveConnection; }

public:
    //
    // internal to implementation
    //
    void OnMessageAvailable(ipcMessage *); // takes ownership

private:
    //
    // helpers
    //
    nsresult PlatformInit();
    nsresult Connect();
    nsresult Disconnect();
    nsresult OnConnectFailure();
    nsresult SendMsg_Internal(ipcMessage *msg);
    nsresult SpawnDaemon();
    void     ProxyToMainThread(PLHandleEventProc);
    void     ProcessIncomingMsgQ();

    PR_STATIC_CALLBACK(void *) ProcessIncomingMsgQ_EventHandler(PLEvent *);
    PR_STATIC_CALLBACK(void *) ConnectionEstablished_EventHandler(PLEvent *);
    PR_STATIC_CALLBACK(void *) ConnectionLost_EventHandler(PLEvent *);
    PR_STATIC_CALLBACK(void)   Generic_EventCleanup(PLEvent *);

    //
    // data
    //
    PRMonitor             *mMonitor;
    ipcTransportObserver  *mObserver; // weak reference
    ipcMessageQ            mDelayedQ;
    ipcMessageQ           *mIncomingMsgQ;
    ipcMessage            *mSyncReplyMsg;
    PRPackedBool           mSyncWaiting;
    PRPackedBool           mSentHello;
    PRPackedBool           mHaveConnection;
    PRPackedBool           mSpawnedDaemon;
    PRUint32               mConnectionAttemptCount;
    PRUint32               mClientID;

#ifdef XP_UNIX
    nsCOMPtr<nsIInputStreamNotify> mReceiver;
    nsCOMPtr<nsISocketTransport>   mTransport;
    nsCOMPtr<nsIInputStream>       mInputStream;
    nsCOMPtr<nsIOutputStream>      mOutputStream;

    //
    // unix specific helpers
    //
    nsresult CreateTransport();
    nsresult GetSocketPath(nsACString &);

public:
    NS_DECL_NSISOCKETEVENTHANDLER

    //
    // internal helper methods
    //
    void OnConnectionLost(nsresult reason);
#endif
};

#endif // !ipcTransport_h__
