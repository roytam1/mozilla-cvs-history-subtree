#ifndef ipcLockService_h__
#define ipcLockService_h__

#include "ipcILockService.h"
#include "ipcIService.h"
#include "ipcList.h"
#include "nsCOMPtr.h"
#include "nsHashtable.h"

class ipcLockService : public ipcILockService
                     , public ipcIMessageObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_IPCILOCKSERVICE
    NS_DECL_IPCIMESSAGEOBSERVER

    ipcLockService();
    virtual ~ipcLockService();

    nsresult Init();

private:
    void NotifyComplete(const char *lockName, nsresult status);

    nsCOMPtr<ipcIService> mIPCService;

    // map from lockname to locknotify for pending notifications
    nsSupportsHashtable mPendingTable;
};

#endif // !ipcLockService_h__
