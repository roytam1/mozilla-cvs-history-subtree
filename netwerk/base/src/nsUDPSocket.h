#ifndef nsUDPSocket_h__
#define nsUDPSocket_h__

#include "nsIUDPSocket.h"
#include "nsSocketTransportService2.h"

class nsDatagram;

//-----------------------------------------------------------------------------

class nsUDPSocket : public nsASocketHandler
                  , public nsIUDPSocket
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIUDPSOCKET

  // nsASocketHandler methods:
  virtual void OnSocketReady(PRFileDesc *fd, PRInt16 outFlags);
  virtual void OnSocketDetached(PRFileDesc *fd);

  nsUDPSocket();

  virtual ~nsUDPSocket();

  void OnMsgClose();
  void OnMsgAttach();
  void OnMsgSend(nsDatagram*dg);
    
  // try attaching our socket (mFD) to the STS's poll list.
  nsresult TryAttach();

private:
  // lock protects access to mReceiver; so it is not cleared while being used.
  PRLock                           *mLock;
  PRFileDesc                       *mFD;
  PRNetAddr                         mAddr;
  nsCOMPtr<nsIUDPReceiver>          mReceiver;
  PRBool                            mAttached;
};

#endif // nsUDPSocket_h__
