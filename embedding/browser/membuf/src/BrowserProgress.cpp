// License Block

#include "BrowserProgress.h"
#include "BrowserGlue.h"

#include <stdio.h>

BrowserProgress::BrowserProgress() : mOwner(nsnull),
                                     mLoadCount(0)
{
  printf("BrowserProgress[%x]::BrowserProgress()\n",this);
  NS_INIT_ISUPPORTS();
}

BrowserProgress::~BrowserProgress()
{
  printf("BrowserProgress[%x]::~BrowserProgress()\n",this);
  mOwner = nsnull;
  mLoadCount = 0;
}

NS_IMPL_ISUPPORTS2( BrowserProgress,
                    nsIWebProgressListener,
                    nsISupportsWeakReference )

nsresult
BrowserProgress::Init( BrowserGlue *aOwner )
{
  printf("BrowserProgress[%x]::Init()\n",this);
  NS_ENSURE_ARG_POINTER(aOwner);

  mOwner = aOwner;
  return NS_OK;
}

NS_IMETHODIMP
BrowserProgress::OnProgressChange( nsIWebProgress *aWebProgress,
                                   nsIRequest *aRequest,
                                   PRInt32 aCurSelfProgress,
                                   PRInt32 aMaxSelfProgress,
                                   PRInt32 aCurTotalProgress,
                                   PRInt32 aMaxTotalProgress )
{
  printf("BrowserProgress[%x]::OnProgressChange()\n",this);
  mOwner->UpdateBitmap();
  return NS_OK;
}

NS_IMETHODIMP
BrowserProgress::OnStateChange( nsIWebProgress *aWebProgress,
                                nsIRequest *aRequest,
                                PRUint32 aProgressStateFlags,
                                nsresult status )
{
  printf("BrowserProgress[%x]::OnStateChange()\n",this);
  if ((aProgressStateFlags & STATE_IS_WINDOW) &&
      (aProgressStateFlags & STATE_STOP)) {
    printf("  - window finished loading\n");
    mOwner->UpdateBitmap();
  }

  // A document or sub-document is starting to load
  if ((aProgressStateFlags & STATE_IS_DOCUMENT) &&
      (aProgressStateFlags & STATE_START)) {
    ++mLoadCount;
  }

  // A document or sub-document has finished loading
  if ((aProgressStateFlags & STATE_IS_DOCUMENT) &&
      (aProgressStateFlags & STATE_STOP)) {
    --mLoadCount;

    // The document and all sub-documents are loaded, so trigger painting
    if(mLoadCount == 0) {
      printf("  - mLoadCount reached zero.\n");
      //ResizeViewportToFitCanvas();
      //SetViewportSize(1024, 1024);
      //mOwner->UpdateBitmap();

      // Only to stop execution
      //mOwner->ExitAppShell();
      //mAppShell->Exit();
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
BrowserProgress::OnLocationChange( nsIWebProgress *aWebProgress,
                                   nsIRequest *aRequest,
                                   nsIURI *location )
{
  printf("BrowserProgress[%x]::OnLocationChange()\n",this);
  return NS_OK;
}

NS_IMETHODIMP
BrowserProgress::OnStatusChange( nsIWebProgress *aWebProgress,
                                 nsIRequest *aRequest,
                                 nsresult aStatus,
                                 const PRUnichar* aMessage )
{
  printf("BrowserProgress[%x]::OnStatusChange()\n",this);
  return NS_OK;
}

NS_IMETHODIMP
BrowserProgress::OnSecurityChange( nsIWebProgress *aWebProgress,
                                   nsIRequest *aRequest,
                                   PRUint32 state )
{
  printf("BrowserProgress[%x]::OnSecurityChange()\n",this);
  return NS_OK;
}

// EOF

