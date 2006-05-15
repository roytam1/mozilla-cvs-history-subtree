#include "nsIProxyObjectManager.h"
#include "nsIServiceManager.h"
#include "nsSocketTransport2.h"
#include "nsUDPSocket.h"
#include "nsAutoLock.h"
#include "nsAutoPtr.h"
#include "nsNetError.h"
#include "nsNetCID.h"
#include "nsProxyRelease.h"
#include "prnetdb.h"
#include "prio.h"
#include "prerror.h"


static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);

//-----------------------------------------------------------------------------
// nsDatagram
//-----------------------------------------------------------------------------

class nsDatagram : public nsIDatagram {
public:
  nsDatagram();
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDATAGRAM
  
  nsCString mData;
  PRNetAddr mAddr;
};

nsDatagram::nsDatagram()
{}

// nsISupports implementation:

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDatagram, nsIDatagram)

// nsIDatagram methods:

/* readonly attribute ACString data; */
NS_IMETHODIMP nsDatagram::GetData(nsACString & aData)
{
  aData = mData;
  return NS_OK;
}

/* readonly attribute ACString address; */
NS_IMETHODIMP nsDatagram::GetAddress(nsACString & aAddress)
{
  char buf[254];
  if (PR_NetAddrToString(&mAddr, buf, sizeof(buf)) != PR_SUCCESS)
    return NS_ERROR_FAILURE;
  aAddress = buf;
  return NS_OK;
}

/* readonly attribute long port; */
NS_IMETHODIMP nsDatagram::GetPort(PRInt32 *aPort)
{
  PRUint16 port;
  if (mAddr.raw.family == PR_AF_INET)
    port = mAddr.inet.port;
  else
    port = mAddr.ipv6.port;
  *aPort = (PRInt32) PR_ntohs(port);
  return NS_OK;
}

//-----------------------------------------------------------------------------

typedef void (nsUDPSocket:: *nsUDPSocketFunc)(void);

static nsresult
PostEvent(nsUDPSocket *s, nsUDPSocketFunc func)
{
  nsCOMPtr<nsIRunnable> ev = new nsRunnableMethod<nsUDPSocket>(s, func);
  if (!ev)
    return NS_ERROR_OUT_OF_MEMORY;

  return gSocketTransportService->Dispatch(ev, NS_DISPATCH_NORMAL);
}

//----------------------------------------------------------------------------
// A 'send' event for proxying nsIUDPSocket::send onto the socket thread.
// XXX Not sure its neccessary actually.

class nsUDPSendEvent : public nsRunnable
{
public:
  nsUDPSendEvent(nsUDPSocket *s, nsDatagram *dg)
      : mSocket(s), mDatagram(dg) {
  }

  NS_IMETHOD Run() {
    mSocket->OnMsgSend(mDatagram);
    return NS_OK;
  }
  
private:
  nsRefPtr<nsUDPSocket> mSocket;
  nsRefPtr<nsDatagram> mDatagram;
};

static nsresult
PostSendEvent(nsUDPSocket *s, nsDatagram *datagram)
{
  nsCOMPtr<nsIRunnable> ev = new nsUDPSendEvent(s, datagram);
  if (!ev)
    return NS_ERROR_OUT_OF_MEMORY;

  return gSocketTransportService->Dispatch(ev, NS_DISPATCH_NORMAL);
}

//-----------------------------------------------------------------------------
// nsUDPSocket
//-----------------------------------------------------------------------------

nsUDPSocket::nsUDPSocket()
  : mLock(nsnull)
  , mFD(nsnull)
  , mAttached(PR_FALSE)
{
  // we want to be able to access the STS directly, and it may not have been
  // constructed yet.  the STS constructor sets gSocketTransportService.
  if (!gSocketTransportService)
  {
    nsCOMPtr<nsISocketTransportService> sts =
        do_GetService(kSocketTransportServiceCID);
    NS_ASSERTION(sts, "no socket transport service");
  }
  // make sure the STS sticks around as long as we do
  NS_ADDREF(gSocketTransportService);
}

nsUDPSocket::~nsUDPSocket()
{
  Close(); // just in case :)

  if (mLock)
    PR_DestroyLock(mLock);

  // release our reference to the STS
  nsSocketTransportService *serv = gSocketTransportService;
  NS_RELEASE(serv);
}

void
nsUDPSocket::OnMsgClose()
{
  if (NS_FAILED(mCondition))
    return;

  // tear down socket.  this signals the STS to detach our socket handler.
  mCondition = NS_BINDING_ABORTED;

  // if we are attached, then we'll close the socket in our OnSocketDetached.
  // otherwise, call OnSocketDetached from here.
  if (!mAttached)
    OnSocketDetached(mFD);
}

void
nsUDPSocket::OnMsgAttach()
{
  if (NS_FAILED(mCondition))
    return;

  mCondition = TryAttach();
  
  // if we hit an error while trying to attach then bail...
  if (NS_FAILED(mCondition))
  {
    NS_ASSERTION(!mAttached, "should not be attached already");
    OnSocketDetached(mFD);
  }
}

void
nsUDPSocket::OnMsgSend(nsDatagram*dg)
{
  if (!dg) {
    NS_WARNING("null datagram");
    return;
  }

  nsCString host;
  dg->GetAddress(host);
  PRInt32 port;
  dg->GetPort(&port);
#ifdef DEBUG_afri
//  printf("-> sending %d bytes to %s, port %d ... ", dg->mData.Length(), host.get(), port);  
#endif
  nsACString::const_iterator iter;
  dg->mData.BeginReading(iter);
  const char *dataBuf = iter.get();
  PRInt32 sent = PR_SendTo(mFD, dataBuf, dg->mData.Length(), 0,
                           &dg->mAddr, PR_INTERVAL_NO_TIMEOUT);
  if (sent == -1) {
    NS_WARNING("Send failure. Maybe we need some buffering?");
    return;
  }
  NS_ASSERTION(sent==dg->mData.Length(), "Sent fewer bytes than in buffer");
#ifdef DEBUG_afri
//  printf(" done <-\n");
#endif
}

nsresult
nsUDPSocket::TryAttach()
{
  nsresult rv;

  //
  // find out if it is going to be ok to attach another socket to the STS.
  // if not then we have to wait for the STS to tell us that it is ok.
  // the notification is asynchronous, which means that when we could be
  // in a race to call AttachSocket once notified.  for this reason, when
  // we get notified, we just re-enter this function.  as a result, we are
  // sure to ask again before calling AttachSocket.  in this way we deal
  // with the race condition.  though it isn't the most elegant solution,
  // it is far simpler than trying to build a system that would guarantee
  // FIFO ordering (which wouldn't even be that valuable IMO).  see bug
  // 194402 for more info.
  //
  if (!gSocketTransportService->CanAttachSocket())
  {
    nsCOMPtr<nsIRunnable> event =
      NS_NEW_RUNNABLE_METHOD(nsUDPSocket, this, OnMsgAttach);
    if (!event)
      return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = gSocketTransportService->NotifyWhenCanAttachSocket(event);
    if (NS_FAILED(rv))
      return rv;
  }

  //
  // ok, we can now attach our socket to the STS for polling
  //
  rv = gSocketTransportService->AttachSocket(mFD, this);
  if (NS_FAILED(rv))
    return rv;

  mAttached = PR_TRUE;

  //
  // now, configure our poll flags for listening...
  //
  mPollFlags = (PR_POLL_READ | PR_POLL_EXCEPT);
  return NS_OK;
}

//-----------------------------------------------------------------------------
// nsUDPSocket::nsASocketHandler
//-----------------------------------------------------------------------------

void
nsUDPSocket::OnSocketReady(PRFileDesc *fd, PRInt16 outFlags)
{
#ifdef DEBUG_afri
//  printf("Socket %p ready: %d\n", this, outFlags);
#endif
  
  NS_ASSERTION(NS_SUCCEEDED(mCondition), "oops");
  NS_ASSERTION(mFD == fd, "wrong file descriptor");
  NS_ASSERTION(outFlags != -1, "unexpected timeout condition reached");

  if (outFlags & (PR_POLL_ERR | PR_POLL_HUP | PR_POLL_NVAL))
  {
    NS_WARNING("error polling on listening socket");
    mCondition = NS_ERROR_UNEXPECTED;
    return;
  }

  if (outFlags & (PR_POLL_READ)) {
    nsRefPtr<nsDatagram> datagram = new nsDatagram();
    NS_ASSERTION(datagram, "could not create datagram");
    
    PRInt32 avail = PR_Available(mFD);
    if (avail == -1 || avail == 0) {
      NS_WARNING("no data available");
#ifdef DEBUG_afri
      printf("%s\n", PR_ErrorToString(PR_GetError(),PR_LANGUAGE_I_DEFAULT));
#endif
      mCondition = NS_ERROR_UNEXPECTED;
    }

    datagram->mData.SetLength(avail);
#ifdef DEBUG_afri
//    printf("udp socket: data waiting for us: %d\n", avail);
#endif
    char *buf = datagram->mData.BeginWriting();
    // XXX we should have some out-of-memory check here

    PRInt32 rv = PR_RecvFrom(mFD, buf, avail, 0, &(datagram->mAddr), PR_INTERVAL_NO_WAIT);
    if (rv < 0) {
      NS_WARNING("receive error");
      
      // XXX we occassionaly hit this when writing to invalid
      // ports. PR_Available returns '1', but we get no data...
      
      //mCondition = NS_ERROR_UNEXPECTED;
      return;
    }

    if (rv != avail) {
      datagram->mData.SetLength(rv);
#ifdef DEBUG_afri
      printf("nsUDPSocket: read bytes %d != avail bytes %d\n",
             rv, avail);
#endif
    }
    
    mReceiver->HandleDatagram(this, datagram);
  }
}

void
nsUDPSocket::OnSocketDetached(PRFileDesc *fd)
{
  // force a failure condition if none set; maybe the STS is shutting down :-/
  if (NS_SUCCEEDED(mCondition))
    mCondition = NS_ERROR_ABORT;

  if (mFD)
  {
    NS_ASSERTION(mFD == fd, "wrong file descriptor");
    PR_Close(mFD);
    mFD = nsnull;
  }

  if (mReceiver)
  {
    // need to atomically clear mReceiver.  see our Close() method.
    nsAutoLock lock(mLock);
    mReceiver = nsnull;
  }
}


//-----------------------------------------------------------------------------
// nsUDPSocket::nsISupports
//-----------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(nsUDPSocket, nsIUDPSocket)


//-----------------------------------------------------------------------------
// nsUDPSocket::nsIUDPSocket
//-----------------------------------------------------------------------------

NS_IMETHODIMP
nsUDPSocket::Init(PRInt32 aPort)
{
  NS_ENSURE_TRUE(mFD == nsnull, NS_ERROR_ALREADY_INITIALIZED);
  
  PRNetAddr addr;

  if (aPort < 0)
    aPort = 0;
  PR_SetNetAddr(PR_IpAddrAny, PR_AF_INET, aPort, &addr);

  if (!mLock)
  {
    mLock = PR_NewLock();
    if (!mLock)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  mFD = PR_OpenUDPSocket(addr.raw.family);
  if (!mFD)
  {
    NS_WARNING("unable to create udp socket");
    return NS_ERROR_FAILURE;
  }

   PRSocketOptionData opt;

   opt.option = PR_SockOpt_Nonblocking;
   opt.value.non_blocking = PR_TRUE;
   PR_SetSocketOption(mFD, &opt);

  if (PR_Bind(mFD, &addr) != PR_SUCCESS)
  {
    NS_WARNING("failed to bind socket");
    goto fail;
  }

  //XXX should we set recv/send buffer sizes?
  
  // get the resulting socket address, which may be different than what
  // we passed to bind.
  if (PR_GetSockName(mFD, &mAddr) != PR_SUCCESS)
  {
    NS_WARNING("cannot get socket name");
    goto fail;
  }

  // wait until setReceiver is called before polling the socket for
  // client connections.
  return NS_OK;

fail:
  Close();
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsUDPSocket::Close()
{
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  {
    nsAutoLock lock(mLock);
    // we want to proxy the close operation to the socket thread if a receiver
    // has been set.  otherwise, we should just close the socket here...
    if (!mReceiver)
    {
      if (mFD)
      {
        PR_Close(mFD);
        mFD = nsnull;
      }
      return NS_OK;
    }
  }
  return PostEvent(this, &nsUDPSocket::OnMsgClose);
}

/* void send (in ACString data, in ACString address, in long port); */
NS_IMETHODIMP
nsUDPSocket::Send(const nsACString & data, const nsACString & address, PRInt32 port)
{
  NS_ENSURE_TRUE(mFD, NS_ERROR_NOT_INITIALIZED);
  
  // prepare datagram:
  nsRefPtr<nsDatagram> datagram = new nsDatagram();
  NS_ASSERTION(datagram, "could not create datagram");

  // apparently PR_StringToNetAddr does not properly initialize
  // the output buffer in the case of IPv6 input.  see bug 223145.
  memset(&datagram->mAddr, 0, sizeof(PRNetAddr));

  nsACString::const_iterator iter;
  address.BeginReading(iter);
  const char *addressBuf = iter.get();
  if (PR_StringToNetAddr(addressBuf, &datagram->mAddr) != PR_SUCCESS) {
    NS_WARNING("could not convert address to PRNetAddr");
    return NS_ERROR_FAILURE;
  }
  if (datagram->mAddr.raw.family == PR_AF_INET)
    datagram->mAddr.inet.port = PR_htons(port);
  else
    datagram->mAddr.ipv6.port = PR_htons(port);

  datagram->mData = data;

  // proxy onto sts thread:
  PostSendEvent(this, datagram);
  
  return NS_OK;
}

/* void setReceiver (in nsIUDPReceiver receiver); */
NS_IMETHODIMP
nsUDPSocket::SetReceiver(nsIUDPReceiver *aReceiver)
{
  // ensuring mFD implies ensuring mLock
  NS_ENSURE_TRUE(mFD, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mReceiver == nsnull, NS_ERROR_IN_PROGRESS);
  {
    nsAutoLock lock(mLock);
    nsresult rv = NS_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                                       NS_GET_IID(nsIUDPReceiver),
                                       aReceiver,
                                       NS_PROXY_ASYNC | NS_PROXY_ALWAYS,
                                       getter_AddRefs(mReceiver));
    if (NS_FAILED(rv))
      return rv;
  }
  return PostEvent(this, &nsUDPSocket::OnMsgAttach);
}

NS_IMETHODIMP
nsUDPSocket::GetPort(PRInt32 *aResult)
{
  // no need to enter the lock here
  PRUint16 port;
  if (mAddr.raw.family == PR_AF_INET)
    port = mAddr.inet.port;
  else
    port = mAddr.ipv6.port;
  *aResult = (PRInt32) PR_ntohs(port);
  return NS_OK;
}

/* readonly attribute ACString address; */
NS_IMETHODIMP
nsUDPSocket::GetAddress(nsACString & aAddress)
{
  char buf[254];
  if (PR_NetAddrToString(&mAddr, buf, sizeof(buf)) != PR_SUCCESS)
    return NS_ERROR_FAILURE;
  aAddress = buf;
  return NS_OK;
}


