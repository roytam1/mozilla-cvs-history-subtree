#include <stdlib.h>
#include "nsIServiceManager.h"
#include "ipcLockService.h"
#include "ipcLockProtocol.h"
#include "ipcCID.h"
#include "ipcLog.h"

static NS_DEFINE_IID(kIPCServiceCID, IPC_SERVICE_CID);
static const nsID kLockTargetID = IPC_LOCK_TARGETID;

ipcLockService::ipcLockService()
{
}

ipcLockService::~ipcLockService()
{
}

nsresult
ipcLockService::Init()
{
    nsresult rv;

    mIPCService = do_GetService(kIPCServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    return mIPCService->SetMessageObserver(kLockTargetID, this);
}

NS_IMPL_ISUPPORTS1(ipcLockService, ipcILockService)

NS_IMETHODIMP
ipcLockService::AcquireLock(const char *lockName, ipcILockNotify *notify, PRBool waitIfBusy)
{
    LOG(("ipcLockService::AcquireLock [lock=%s sync=%u wait=%u]\n",
        lockName, notify == nsnull, waitIfBusy));

    ipcLockMsg msg;
    msg.opcode = IPC_LOCK_OP_ACQUIRE;
    msg.flags = (waitIfBusy ? 0 : IPC_LOCK_FL_NONBLOCKING);
    msg.key = lockName;

    PRUint32 bufLen;
    PRUint8 *buf = IPC_FlattenLockMsg(&msg, &bufLen);
    if (!buf)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = mIPCService->SendMessage(0, kLockTargetID, buf, bufLen, (notify == nsnull));
    free(buf);
    if (NS_FAILED(rv)) {
        LOG(("  SendMessage failed [rv=%x]\n", rv));
        return rv;
    }

    if (notify) {
        nsCStringKey hashKey(lockName);
        mPendingTable.Put(&hashKey, notify);
    }

    return NS_OK;
}

NS_IMETHODIMP
ipcLockService::ReleaseLock(const char *lockName)
{
    LOG(("ipcLockService::ReleaseLock [lock=%s]\n", lockName));

    ipcLockMsg msg;
    msg.opcode = IPC_LOCK_OP_RELEASE;
    msg.flags = 0;
    msg.key = lockName;

    PRUint32 bufLen;
    PRUint8 *buf = IPC_FlattenLockMsg(&msg, &bufLen);
    if (!buf)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = mIPCService->SendMessage(0, kLockTargetID, buf, bufLen, PR_FALSE);
    free(buf);

    if (NS_FAILED(rv)) return rv;
    
    nsCStringKey hashKey(lockName);
    mPendingTable.Remove(&hashKey);
    return NS_OK;
}

NS_IMETHODIMP
ipcLockService::OnMessageAvailable(const nsID &target, const PRUint8 *data, PRUint32 dataLen)
{
    ipcLockMsg msg;
    IPC_UnflattenLockMsg(data, dataLen, &msg);

    LOG(("ipcLockService::OnMessageAvailable [lock=%s opcode=%u]\n", msg.key, msg.opcode)); 

    nsresult status;
    if (msg.opcode == IPC_LOCK_OP_STATUS_ACQUIRED)
        status = NS_OK;
    else
        status = NS_ERROR_FAILURE;

    NotifyComplete(msg.key, status);
    return NS_OK;
}

void
ipcLockService::NotifyComplete(const char *lockName, nsresult status)
{
    nsCStringKey hashKey(lockName);
    nsISupports *obj = mPendingTable.Get(&hashKey); // ADDREFS
    if (obj) {
        nsCOMPtr<ipcILockNotify> notify = do_QueryInterface(obj);
        NS_RELEASE(obj);
        if (notify)
            notify->OnAcquireLockComplete(lockName, status);
    }
}
