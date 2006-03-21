// License Block

#include "BrowserWindow.h"

#include "nsComponentManagerUtils.h"
#include "nsCWebBrowser.h" // includes the other neccessary embedding idl files
#include "nsEmbedCID.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDOMWindow.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIPresShell.h"
#include "nsIProxyObjectManager.h"
#include "nsIViewManager.h"
#include "nsIWidget.h"
#include "nsServiceManagerUtils.h"
#include "nsRect.h"
#include "nsWidgetsCID.h"
#include "nsXULAppAPI.h"

// some redefinitions for allegro bad behavior
#include "membufAllegroDefines.h"

// Allegor headers (for BITMAP)
#include <allegro.h>

// defines pulled from membufWindow (should probably go in a seperate header
#define MEMBUF_NATIVE_BITMAP               7700
#define MEMBUF_NATIVE_BITMAP_PARENT        7701
#define MEMBUF_NATIVE_BITMAP_DIRTY_FLAG    7702
#define MEMBUF_NATIVE_BITMAP_REFCOUNT      7703

static NS_DEFINE_CID( kWindowCID, NS_WINDOW_CID );
static NS_DEFINE_CID( kWebBrowserCID, NS_WEBBROWSER_CID );

BrowserWindow::BrowserWindow() : mOwner(nsnull),
                                 mWebBrowser(nsnull),
                                 mBaseWindow(nsnull),
                                 mWidget(nsnull),
                                 mBitmap(nsnull),
                                 mVisibility(PR_FALSE),
                                 mIsModal(PR_FALSE),
                                 mChromeFlags(0),
                                 mWidth(0),
                                 mHeight(0)
{
  printf("BrowserWindow::BrowserWindow()\n");
  NS_INIT_ISUPPORTS();
}

BrowserWindow::~BrowserWindow()
{
  printf("BrowserWindow::~BrowserWindow()\n");
  mOwner = nsnull;

  // XPCOM object gets released
  mWidget = nsnull;
  
  // Make the nsWebBrowser go away
  mBaseWindow->Destroy();
  mBaseWindow = nsnull;
  mWebBrowser = nsnull;
}

NS_IMPL_ISUPPORTS5( BrowserWindow,
                    nsIWebBrowserChrome,
                    nsIWebBrowserChromeFocus,
                    nsIEmbeddingSiteWindow,
                    nsITooltipListener,
                    nsIInterfaceRequestor )

NS_METHOD
BrowserWindow::Init( BrowserGlue *aOwner, PRInt32 aWidth, PRInt32 aHeight )
{
  printf("BrowserWindow::Init(%d,%d)\n",aWidth, aHeight);
  NS_ENSURE_ARG_POINTER(aOwner);
  nsresult rv = NS_ERROR_FAILURE;
  mWidth = aWidth;
  mHeight = aHeight;

  // create a browser object
  nsCOMPtr<nsIWebBrowser> unproxyWebBrowser =
                          do_CreateInstance(NS_WEBBROWSER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    printf("BrowserWindow::Init() - failed to create browser\n");
    return NS_ERROR_FAILURE;
  }

  // get the proxy manager
  nsCOMPtr<nsIProxyObjectManager> proxyObjMgr =
                          do_GetService("@mozilla.org/xpcomproxy;1", &rv);
  if (NS_FAILED(rv)) {
    NS_ERROR("BrowserWindow::Init(): could not create "
             "proxy object manager");
    return NS_ERROR_FAILURE;
  }
                                                                                
  // use it to get a proxy for the web browser for thread safety reasons
  rv = proxyObjMgr->GetProxyForObject(NS_UI_THREAD_EVENTQ,
                                      NS_GET_IID(nsIWebBrowser),
                                      unproxyWebBrowser,
                                      PROXY_SYNC|PROXY_ALWAYS,
                                      getter_AddRefs(mWebBrowser));
  if (NS_FAILED(rv)) {
    NS_ERROR("BrowserWindow::Init(): could not create proxy "
             "browser object");
    return NS_ERROR_FAILURE;
  }
 
  // register as the BrowserChrome 
  mWebBrowser->SetContainerWindow(NS_STATIC_CAST( nsIWebBrowserChrome*, this ));

  // Set the content type
  nsCOMPtr<nsIDocShellTreeItem> item = do_QueryInterface(mWebBrowser);
  item->SetItemType(nsIDocShellTreeItem::typeContentWrapper);

  return rv;
}

NS_METHOD
BrowserWindow::CreateWindow()
{
  printf("BrowserWindow::CreateWindow() - mWebBrowser:[%x]\n",&mWebBrowser);
  nsresult rv = NS_ERROR_FAILURE;

  // get a handle to the nsIBaseWindow interface
  mBaseWindow = do_QueryInterface( mWebBrowser, &rv );
  NS_ENSURE_TRUE(mBaseWindow, NS_ERROR_FAILURE);
  if ( NS_FAILED(rv) || !mBaseWindow ) {
    printf("BrowserWindow::CreateWindow - failed to QI base window\n");
    return NS_ERROR_FAILURE;
  }

  // the width and height need to come from the embeddor so they
  //  match. Otherwise with the linear bitmap it doesn't work and we
  //  wind up several rows off. :-)

  // this creates a membufWindow and causes the window to create a new
  //   BITMAP of the height/width passed in. The BITMAP is the parent 
  //   of all subsequent BITMAPS that get created. The window _should_
  //   be the parent window to all subsequent windows but that isn't
  //   clear from looking the code.
  mBaseWindow->InitWindow( (void*)-1,         // pointer to a native window
                           nsnull,            // nsIWidget 
                           0,                 // X origin
                           0,                 // Y origin
                           mWidth,            // width 
                           mHeight );         // height
  mBaseWindow->Create();

  // get the main widget from the baseWindow (the one we just created above)
  mBaseWindow->GetMainWidget(getter_AddRefs(mWidget));

  // XXXjgaunt
  // get the bitmap from the main widget
  //mBitmap = (BITMAP*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP);
  mBitmap = (BITMAP*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP_PARENT);

  mBaseWindow->SetVisibility( PR_TRUE );
  mBaseWindow->SetPosition( 0, 0 );

  return rv;
}

nsresult
BrowserWindow::GetView(nsIView **aView)
{
    nsIWebBrowser *aWebBrowser = mWebBrowser;

    // Get the presShell for the currently loaded document
    nsCOMPtr<nsIDocShell> docShell = do_GetInterface(aWebBrowser);
    nsCOMPtr<nsIPresShell> presShell;
    docShell->GetPresShell(getter_AddRefs(presShell));
    NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

    // Get the root view, or to "viewport view"
    nsIViewManager  *viewManager = presShell->GetViewManager();
    NS_ENSURE_TRUE(viewManager, NS_ERROR_FAILURE);

    viewManager->GetRootView(*aView);
    return NS_OK;
}

BITMAP*
BrowserWindow::GetBitmap()
{
    return mBitmap;
}

unsigned char*
BrowserWindow::GetBuffer()
{
  printf("BrowserWindow::GetBuffer() b:%x d:%x\n",mBitmap, mBitmap->dat);
  return (unsigned char*)mBitmap->dat;
}

// nsIWebBrowserChrome

NS_IMETHODIMP
BrowserWindow::SetStatus( PRUint32 aStatusType, const PRUnichar *aStatus )
{
  printf("BrowserWindow::SetStatus()\n");
  return NS_OK;
}
             
NS_IMETHODIMP
BrowserWindow::GetWebBrowser( nsIWebBrowser **aWebBrowser )
{
  printf("BrowserWindow::GetWebBrowser()\n");
  NS_ENSURE_ARG_POINTER(aWebBrowser);
  *aWebBrowser = mWebBrowser;
  NS_IF_ADDREF(*aWebBrowser);
  return NS_OK;
}
             
NS_IMETHODIMP
BrowserWindow::SetWebBrowser( nsIWebBrowser *aWebBrowser )
{
  printf("BrowserWindow::SetWebBrowser()\n");
  mWebBrowser = aWebBrowser;
  return NS_OK;
}
             
NS_IMETHODIMP
BrowserWindow::GetChromeFlags( PRUint32 *aChromeFlags )
{
  printf("BrowserWindow::GetChromeFlags()\n");
  *aChromeFlags = mChromeFlags;
  return NS_OK;
}
             
NS_IMETHODIMP
BrowserWindow::SetChromeFlags( PRUint32 aChromeFlags )
{
  printf("BrowserWindow::SetChromeFlags()\n");
  mChromeFlags = aChromeFlags;
  return NS_OK;
}
             
NS_IMETHODIMP
BrowserWindow::DestroyBrowserWindow()
{
  printf("BrowserWindow::DestroyBrowserWindow()\n");
  return NS_OK;
}
             
NS_IMETHODIMP
BrowserWindow::SizeBrowserTo( PRInt32 aCX, PRInt32 aCY )
{
  printf("BrowserWindow::SizeBrowserTo()\n");
  return NS_OK;
}
             
NS_IMETHODIMP
BrowserWindow::ShowAsModal()
{
  printf("BrowserWindow::ShowAsModal()\n");
  return NS_OK;
}
             
NS_IMETHODIMP
BrowserWindow::IsWindowModal( PRBool *_retval )
{
  printf("BrowserWindow::IsWindowModal()\n");
  *_retval = PR_FALSE;
  return NS_OK;
}
             
NS_IMETHODIMP
BrowserWindow::ExitModalEventLoop( nsresult aStatus )
{
  printf("BrowserWindow::ExitModalEventLoop()\n");
  return NS_OK;
}
             
// nsIWebBrowserChromeFocus

NS_IMETHODIMP
BrowserWindow::FocusNextElement()
{
  printf("BrowserWindow::FocusNextElement()\n");
  return NS_OK;
}
             
NS_IMETHODIMP
BrowserWindow::FocusPrevElement()
{
  printf("BrowserWindow::FocusPrevElement()\n");
  return NS_OK;
}
             
// nsIEmbeddingSiteWindow

NS_IMETHODIMP
BrowserWindow::SetDimensions( PRUint32 aFlags,
                              PRInt32 aX,
                              PRInt32 aY,
                              PRInt32 aCX,
                              PRInt32 aCY )
{
  printf("BrowserWindow::SetDimensions()\n");
/*
  // might need to work around this method. In order to have tighter
  //   control over the size & position of the "widow" we might
  //   want to not impl this and have 2 other non-nsI methods
  //   that SetSize and SetPosition.
  if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION &&
      (aFlags & (nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER |
                 nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER))) {
    return mBaseWindow->SetPositionAndSize(aX, aY, aCX, aCY, PR_TRUE);
  }
  else if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION) {
    return mBaseWindow->SetPosition(aX, aY);
  }
  else if (aFlags & (nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER |
                     nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER)) {
    return mBaseWindow->SetSize(aCX, aCY, PR_TRUE);
  }
  return NS_ERROR_INVALID_ARG;
*/
  return NS_ERROR_NOT_IMPLEMENTED;
}
             
NS_IMETHODIMP
BrowserWindow::GetDimensions( PRUint32 aFlags,
                              PRInt32 *aX,
                              PRInt32 *aY,
                              PRInt32 *aCX,
                              PRInt32 *aCY )
{
  printf("BrowserWindow::GetDimensions()\n");
  return NS_OK;
}
             
NS_IMETHODIMP
BrowserWindow::SetFocus()
{
  printf("BrowserWindow::SetFocus()\n");
  return NS_OK;
}
             
NS_IMETHODIMP
BrowserWindow::GetTitle( PRUnichar **aTitle )
{
  printf("BrowserWindow::GetTitle()\n");
  *aTitle = nsnull;
  return NS_OK;
}
             
NS_IMETHODIMP
BrowserWindow::SetTitle( const PRUnichar *aTitle )
{
  printf("BrowserWindow::SetTitle()\n");
  return NS_OK;
}
             
NS_IMETHODIMP
BrowserWindow::GetSiteWindow( void **aSiteWindow )
{
  printf("BrowserWindow::GetSiteWindow()\n");
  *aSiteWindow = this;
  return NS_OK;
}
             
NS_IMETHODIMP
BrowserWindow::GetVisibility( PRBool *aVisibility )
{
  printf("BrowserWindow::GetVisibility()\n");
  *aVisibility = PR_TRUE;
  return NS_OK;
}
             
NS_IMETHODIMP
BrowserWindow::SetVisibility( PRBool aVisibility)
{
  printf("BrowserWindow::SetVisibility()\n");
  return NS_OK;
}
             
// nsITooltipListener

NS_IMETHODIMP
BrowserWindow::OnShowTooltip( PRInt32 aX, 
                              PRInt32 aY, 
                              const PRUnichar *aTipText )
{
  printf("BrowserWindow::OnShowTooltip()\n");
  return NS_OK;
}
             
NS_IMETHODIMP
BrowserWindow::OnHideTooltip()
{
  printf("BrowserWindow::OnHideTooltip()\n");
  return NS_OK;
}
             
// nsIInterfaceRequestor

NS_IMETHODIMP
BrowserWindow::GetInterface( const nsIID &aIID, void **aInstancePtr )
{
 
  printf("BrowserWindow::GetInterface()\n");
  NS_ENSURE_ARG_POINTER(aInstancePtr);

  *aInstancePtr = 0;
  if (aIID.Equals(NS_GET_IID(nsIDOMWindow)))
  {
    if (mWebBrowser)
    {
      return mWebBrowser->GetContentDOMWindow((nsIDOMWindow **) aInstancePtr);
    }
    return NS_ERROR_NOT_INITIALIZED;
  }
  return QueryInterface(aIID, aInstancePtr);
}
             
