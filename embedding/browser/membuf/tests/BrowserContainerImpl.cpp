// license block

#include "BrowserContainerImpl.h"
#include "nsString.h"
#include "nsIDOMKeyEvent.h"
#include "nsGUIEvent.h"

BrowserContainerImpl::BrowserContainerImpl() : mBrowser(0),
                                               mBitmap(0),
                                               mBuffer(0),
                                               mFilename()
{
  printf("BrowserContainerImpl[%x]::BrowserContainerImpl()\n",this);
}

BrowserContainerImpl::~BrowserContainerImpl()
{
  printf("BrowserContainerImpl[%x]::~BrowserContainerImpl()\n",this);
  if (mBrowser)
    delete mBrowser;
  mBrowser = nsnull;
  mBitmap = nsnull;
  mBuffer = nsnull;
}

// BrowserContainer Interface methods

nsresult
BrowserContainerImpl::SetBuffer(unsigned char *aBuffer)
{
  printf("BrowserContainerImpl[%x]::SetBuffer(%x)\n",this,aBuffer);
  NS_ENSURE_ARG(aBuffer);
  // this is shared memory that we didn't create, don't free
  mBuffer = aBuffer;
  return NS_OK;
}

nsresult
BrowserContainerImpl::SetBitmap(BITMAP *aBitmap)
{
  printf("BrowserContainerImpl[%x]::SetBitmap(%x)\n",this,aBitmap);
  NS_ENSURE_ARG(aBitmap);
  mBitmap = aBitmap;
  BitmapUpdated();
  return NS_OK;
}

nsresult
BrowserContainerImpl::BitmapUpdated()
{
  printf("BrowserContainerImpl[%x]::BitmapUpdated()\n",this);
  save_tga(ToNewCString(mFilename), mBitmap, 0);
  return NS_OK;
}

// BrowserContainerImpl methods

nsresult
BrowserContainerImpl::Init(PRInt32 aWidth, PRInt32 aHeight)
{
  printf("BrowserContainerImpl[%x]::Init(%d,%d)\n",this,aWidth,aHeight);
  nsresult rv = NS_ERROR_FAILURE;

  // create the browser object
  mBrowser = new BrowserGlue();

  // if we have a browser initialize it
  if (mBrowser)
    rv = mBrowser->Init(NS_STATIC_CAST(BrowserContainer*, this),
                        aWidth,
                        aHeight);

  return rv;
}

nsresult
BrowserContainerImpl::CreateBrowser()
{
  printf("BrowserContainerImpl[%x]::CreateBrowser()\n",this);
  // this actually creates the browser window, bitmap etc
  if (mBrowser)
    return mBrowser->CreateBrowser();

  return NS_ERROR_FAILURE;
}

nsresult
BrowserContainerImpl::Run()
{
  printf("BrowserContainerImpl[%x]::Run()\n",this);
  if (mBrowser)
    return mBrowser->Run();

  return NS_ERROR_FAILURE;
}

nsresult
BrowserContainerImpl::SetDimensions( PRUint32 aWidth, PRUint32 aHeight )
{
  printf("BrowserContainerImpl[%x]::setDimensions(%d, %d)\n",
                                         this,aWidth,aHeight);
  if (mBrowser)
    return mBrowser->SetViewportSize(aWidth, aHeight);
  return NS_OK;
}

nsresult
BrowserContainerImpl::LoadURI(char *aURI)
{
  printf("BrowserContainerImpl[%x]::loadURI(%s)\n",this,aURI);
  NS_ENSURE_ARG_POINTER(aURI);
  mURI = aURI;
  mBrowser->LoadURI(aURI);
  return NS_OK;
}

nsresult
BrowserContainerImpl::SetOutputFile(char *aFilename)
{
  printf("BrowserContainerImpl[%x]::setOutputFile(%s)\n",this,aFilename);
  NS_ENSURE_ARG_POINTER(aFilename);
  mFilename = aFilename;
  mBrowser->SetOutputFile(aFilename);
  
  return NS_OK;
}

nsresult
BrowserContainerImpl::SendSpatNavDown()
{
  printf("BrowserContainerImpl[%x]::SendSpatNavDown()\n",this);

  nsKeyEvent keyEvent( PR_TRUE, NS_KEY_PRESS, nsnull );
  keyEvent.keyCode = nsIDOMKeyEvent::DOM_VK_DOWN;
  keyEvent.charCode = 0;
  keyEvent.isChar = PR_TRUE;
  keyEvent.isShift = PR_TRUE;
  keyEvent.isControl = 0;
  keyEvent.isAlt = PR_TRUE;
  keyEvent.isMeta = 0;
  keyEvent.widget = nsnull;
  keyEvent.nativeMsg = nsnull;
  keyEvent.point.x = 0;
  keyEvent.point.y = 0;
  keyEvent.refPoint.x = 0;
  keyEvent.refPoint.y = 0;
  keyEvent.flags = 0;

  mBrowser->SendKeyEvent(keyEvent);

  mBrowser->BitmapUpdated();

  return NS_OK;
}

nsresult
BrowserContainerImpl::SendSpatNavUp()
{
  printf("BrowserContainerImpl[%x]::SendSpatNavUp()\n",this);
  return NS_OK;
}

nsresult
BrowserContainerImpl::SendSpatNavLeft()
{
  printf("BrowserContainerImpl[%x]::SendSpatNavLeft()\n",this);
  return NS_OK;
}

nsresult
BrowserContainerImpl::SendSpatNavRight()
{
  printf("BrowserContainerImpl[%x]::SendSpatNavRight()\n",this);
  return NS_OK;
}

