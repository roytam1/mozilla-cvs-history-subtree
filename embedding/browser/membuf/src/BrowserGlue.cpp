// License Block

#include "BrowserGlue.h"

// plugin headers
#include "BrowserContainer.h"
#include "BrowserProgress.h"
#include "BrowserWindow.h"

// platform headers
#include <stdio.h>
#include <stdlib.h>

// mozilla headers
#include "nsAppDirectoryServiceDefs.h"
#include "nsComponentManagerUtils.h"
#include "nsEmbedCID.h"
#include "nsIComponentManager.h"
#include "nsIDirectoryService.h"
#include "nsIDocShell.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsILocalFile.h"
#include "nsIPresShell.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsStringAPI.h"
#include "nsWidgetsCID.h"
#include "nsXULAppAPI.h"
//#include "nsXPCOM.h"

// XXXjgaunt - move to a header in membufWindow and export it.
// matches the data in membufWindow
#define MEMBUF_NATIVE_BITMAP               7700
#define MEMBUF_NATIVE_BITMAP_PARENT        7701
#define MEMBUF_NATIVE_BITMAP_DIRTY_FLAG    7702
#define MEMBUF_NATIVE_BITMAP_REFCOUNT      7703

// set up static stuff
static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);
nsIAppShell* BrowserGlue::sAppShell = nsnull;

BrowserGlue::BrowserGlue() : mOwner(nsnull),
                             mWebBrowser(nsnull),
                             mBrowserWindow(nsnull),
                             mBrowserProgress(nsnull),
                             mDirProvider(nsnull),
                             mProfDirProvider(nsnull),
                             mWebNav(nsnull),
                             mBitmapIsSet(PR_FALSE),
                             mWidth(0),
                             mHeight(0)
{
  printf("BrowserGlue::BrowserGlue()\n");
}

BrowserGlue::~BrowserGlue()
{
  printf("BrowserGlue::~BrowserGlue()\n");
/*
  nsWeakPtr weakRef(do_GetWeakReference(NS_STATIC_CAST( nsIWebProgressListener*,
                                                        mBrowserProgress )));
  mWebBrowser->RemoveWebBrowserListener( weakRef,
                                         NS_GET_IID(nsIWebProgressListener) );

  if (mProfDirProvider) {
    mProfDirProvider->Shutdown();
    NS_RELEASE(mProfDirProvider);
    mProfDirProvider = nsnull;
  }
*/ 

  // null out the XPCOM pointers
  mWebBrowser = nsnull;
  mWebNav = nsnull;
  mBitmapIsSet = PR_FALSE;

  // delete the c-style pointers
  if (mBrowserWindow)
    delete mBrowserWindow; 
  if (mBrowserProgress)
    delete mBrowserProgress;

  // remove the reference to our owner, do not delete
  mOwner = nsnull;

  // shut down the appshell
/*
  if (sAppShell) {
    sAppShell->Exit();
    sAppShell->Spindown();
    NS_RELEASE(sAppShell);
    sAppShell = 0;
  }
*/
  // Shut ourself down
  Shutdown();

  // shutdown the XRE 
  XRE_TermEmbedding();
}

nsresult
BrowserGlue::Shutdown()
{
  printf("BrowserGlue::Shutdown()\n");
  nsWeakPtr weakRef(do_GetWeakReference(NS_STATIC_CAST(nsIWebProgressListener*,
                                                       mBrowserProgress)));
  mWebBrowser->RemoveWebBrowserListener(weakRef,
                                        NS_GET_IID(nsIWebProgressListener));
  if (mProfDirProvider) {
    mProfDirProvider->Shutdown();
    NS_RELEASE(mProfDirProvider);
    mProfDirProvider = nsnull;
  }

  if (sAppShell) {
    sAppShell->Exit();
    sAppShell->Spindown();
    NS_RELEASE(sAppShell);
    sAppShell = 0;
  }
  return NS_OK;
}

// XXXjgaunt add parameters to this call to set the
//           different directories by the caller.
NS_METHOD
BrowserGlue::Init( BrowserContainer *aBrowserOwner,
                   PRInt32 aWidth,
                   PRInt32 aHeight )
{
  printf("BrowserGlue::Init()\n");
  NS_ENSURE_ARG_POINTER (aBrowserOwner);
  nsresult rv = NS_ERROR_FAILURE;

  mWidth = aWidth;
  mHeight = aHeight;

  // Set the owning plugin member
  mOwner = aBrowserOwner;

  // Get the Application directory
  //const char *appPath = getenv("OPENPFC_BIN_HOME");
  const char *appPath = getenv("XRE_HOME");
  if (!appPath) {
    printf("BrowserGlue::Init() - failed to get app path\n");
    return rv;
  }
  else
    printf("BrowserGlue::Init() - app path:%s\n", appPath);

  nsCOMPtr<nsILocalFile> appDir;
  rv = NS_NewNativeLocalFile( nsDependentCString(appPath),
                              PR_TRUE,
                              getter_AddRefs(appDir) );
  if (NS_FAILED(rv)) {
    printf("BrowserGlue::Init() - failed to get app directory\n");
    return rv;
  }
  //appDir->Append(NS_LITERAL_STRING("XRE"));

  // Get the XRE directory
  const char *libXULPath = getenv("XRE_HOME");
  //const char *libXULPath = getenv("OPENPFC_BIN_HOME");
  if (!libXULPath) {
    printf("BrowserGlue::Init() - failed to get XUL path\n");
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsILocalFile> libXULDir;
  rv = NS_NewNativeLocalFile( nsDependentCString(libXULPath),
                              PR_TRUE,
                              getter_AddRefs(libXULDir) );

  if (NS_FAILED(rv)) {
    printf("BrowserGlue::Init() - failed to get XUL directory\n");
    return rv;
  }
  //libXULDir->Append(NS_LITERAL_STRING("XRE"));

  nsCAutoString xul_path;
  libXULDir->GetNativePath(xul_path);
  printf("BrowserGlue::Init() - libXULDir:%s\n", xul_path.get() );

  nsCAutoString app_path;
  appDir->GetNativePath(app_path);
  printf("BrowserGlue::Init() - appDir:%s\n", app_path.get() );

  //SetupProfile();
/*
  // Start the XRE - XXX do we need a nsIDirectoryServiceProvider
  rv = XRE_InitEmbedding( libXULDir,    // directory to find libxul
                          appDir,       // directory to find...
                          nsnull,       // directory provider for GRE dirs
                          nsnull,       // static components provided by app
                          0 );          // count of static components
  if (NS_FAILED(rv)) {
    printf("BrowserGlue::Init() - failed to initialize XPCOM\n");
    return rv;
  }
*/
  // not available on the branch!!!
  //XRE_NotifyProfile();

  // setup the profile directory - XXXjgaunt clone the xrepath, add profile
  nsCOMPtr<nsILocalFile> appProfDir;
  rv = NS_NewNativeLocalFile( nsDependentCString(appPath),
                              PR_TRUE,
                              getter_AddRefs(appProfDir) );
  if (NS_FAILED(rv)) {
    printf("BrowserGlue::Init() - failed to get appProf directory\n");
   // XRE_TermEmbedding();
    return rv;
  }
  //appProfDir->Append(NS_LITERAL_STRING("XRE"));
  appProfDir->Append(NS_LITERAL_STRING("membufprofile"));

  nsCAutoString path;
  appProfDir->GetNativePath(path);
  printf("BrowserGlue::Init() - appProfDir:%s\n", path.get() );

  // setup the directory service provider
  nsCOMPtr<nsProfileDirServiceProvider> dirServiceProvider;
  rv = NS_NewProfileDirServiceProvider( PR_TRUE,
                                        getter_AddRefs(dirServiceProvider));
  mProfDirProvider = dirServiceProvider;
  NS_IF_ADDREF(mProfDirProvider);
  if (NS_FAILED(rv)) {
    printf("BrowserGlue::Init() - failed to create dirServiceProvider\n");
    return rv;
  }

  // Start the XRE
  rv = XRE_InitEmbedding( libXULDir,        // dir to find libxul
                          appDir,           // dir to find components/prefs
                          mProfDirProvider, // dir provider for GRE dirs
                          nsnull,       // static components provided by app
                          0 );          // count of static components
  if (NS_FAILED(rv)) {
    printf("BrowserGlue::Init() - failed to initialize XPCOM\n");
    return rv;
  }

  // register the dir provider with the component manager
  rv = mProfDirProvider->Register();
  if (NS_FAILED(rv)) {
    printf("BrowserGlue::Init() - failed to regsiter dirServiceProvider\n");
    XRE_TermEmbedding();
    return rv;
  }

  // set the profile dir with the dir provider
  rv = mProfDirProvider->SetProfileDir(appProfDir);
  if (NS_FAILED(rv)) {
    printf("BrowserGlue::Init() - failed to set profile directory\n");
    XRE_TermEmbedding();
    return rv;
  }
  
  // create the AppShell. 
  nsCOMPtr<nsIAppShell> appShell;
  appShell = do_CreateInstance(kAppShellCID);
  if (!appShell) {
    NS_WARNING("BrowserGlue::Init() - Failed to create appshell\n");
    XRE_TermEmbedding();
    return NS_ERROR_FAILURE;
  }
  sAppShell = appShell.get();
  NS_ADDREF(sAppShell);
  sAppShell->Create(0, nsnull);
  sAppShell->Spinup();

  rv = InitStage2();
  if(NS_FAILED(rv)) {
    NS_WARNING("BrowserGlue::Init() - Failed to InitStage2\n");
    XRE_TermEmbedding();
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

// not needed as of yet, not clear if this will work on the branch
//   without the XRE_NotifyProfile() call.
nsresult
BrowserGlue::SetupProfile()
{
  printf("BrowserGlue::SetupProfile()\n");

  mDirProvider = new BrowserDirProvider();
  NS_IF_ADDREF(mDirProvider);
  mDirProvider->Init(this);
  return NS_OK;
}

NS_METHOD
BrowserGlue::InitStage2()
{
  printf("BrowserGlue::InitStage2()\n");

  // create the nsIWebBrowserChrome object
  mBrowserWindow = new BrowserWindow();
  if (!mBrowserWindow) {
    printf("BrowserGlue::InitStage2() - ERR failed to create browserwindow\n");
    return NS_ERROR_FAILURE;
  }
  NS_IF_ADDREF(mBrowserWindow);
  mBrowserWindow->Init(this, mWidth, mHeight);

  // setup the browser progress listener
  mBrowserProgress = new BrowserProgress();
  if (!mBrowserProgress) {
    printf("BrowserGlue::InitStage2() - ERR failed to create BrowserProgress\n");
    return NS_ERROR_FAILURE;
  }
  NS_IF_ADDREF(mBrowserProgress);
  mBrowserProgress->Init(this);

  return NS_OK;
}

nsresult
BrowserGlue::SetViewportSize(PRUint32 aWidth, PRUint32 aHeight)
{
  printf("BrowserGlue::SetViewportSize(%d x %d)\n",aWidth, aHeight);
  mWidth = aWidth;
  mHeight = aHeight;
  /*
  mBrowserWindow->SetDimensions(nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER,
                                0, 0, aWidth, aHeight);
 */
  return NS_OK;
}

NS_METHOD
BrowserGlue::CreateBrowser()
{
  printf("BrowserGlue::CreateBrowser()\n");

  nsresult rv = NS_ERROR_FAILURE;

  // this call creates the nsIWidget and the BITMAP
  mBrowserWindow->CreateWindow();
  mBrowserWindow->GetWebBrowser(getter_AddRefs(mWebBrowser));

  // Get a handle to the nsIWebNavigation interface of the browser
  mWebNav = do_QueryInterface( mWebBrowser, &rv );
  if ( NS_FAILED(rv) || !mWebNav ) {
    printf("BrowserGlue::CreateBrowser - failed to QI webnav\n");
    return NS_ERROR_FAILURE;
  }

  // Listen to progress notifications
  nsWeakPtr weakRef(do_GetWeakReference(NS_STATIC_CAST( nsIWebProgressListener*,
                                                        mBrowserProgress )));
  mWebBrowser->AddWebBrowserListener( weakRef,
                                      NS_GET_IID(nsIWebProgressListener) );

  return NS_OK;
}

nsresult
BrowserGlue::Run()
{
  if (sAppShell) {
    // this call enters a loop and only exits when the app exits
    sAppShell->Run();
    return NS_OK;
  }
  printf("XXXjgaunt - Whoa, where'd that appshell go?\n");
  return NS_ERROR_FAILURE;
}

nsresult
BrowserGlue::UpdateBitmap()
{
  printf("BrowserGlue::UpdateBitmap()\n");
  
  allegro_errno = &errno;
  char* filename_before = "browserglue_image.tga";
  char* filename_after = "openpfc_browser_image_after.tga";
  BITMAP *bitmap = nsnull;

  // force the widget to paint before we capture the image and hand it off
  //   this fixes issues where the paints aren't finished before we give the
  //   bitmap to the plugin. e.g. www.google.com
  Invalidate();

  // Get the BITMAP from the BrowserWindow object
  bitmap = mBrowserWindow->GetBitmap();

/*
  // XXXjgaunt - we'll be making this call a lot, so I think we can hold
  //             off with the dumpfiles
  printf("   saving bitmap to '%s'\n", filename_before);
  //save_tga(ToNewCString(mFilename), bitmap, 0);
  save_tga(filename_before, bitmap, 0);
*/ 

  // Tell the embeddor the bitmap has changed, if we've been here already.
  if (mBitmapIsSet) {
    printf("   bitmap already set, calling BitmapUpdated()\n");
    mOwner->BitmapUpdated();
  }
  else {
    // We reach this point before the bitmap gets created somehow.
    if (bitmap) {
      // Give the embeddor the data in the form of an Allegro Bitmap
      printf("   bitmap not set.\n");
      // call both, give the embeddor the option of what to save and deal with
      mOwner->SetBuffer((unsigned char*)bitmap->dat);
      mOwner->SetBitmap(bitmap);
      mBitmapIsSet = PR_TRUE;
    }
  }
  return NS_OK;
}

nsresult
BrowserGlue::Invalidate()
{
  printf("BrowserGlue[%x]::Invalidate()\n",this);

  // get the view for the browser
  nsIView *view = nsnull;

  // Get the docShell for the currently loaded document
  nsCOMPtr<nsIDocShell> docShell = do_GetInterface(mWebBrowser);
  NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);

  // Get the presShell from the docShell
  nsCOMPtr<nsIPresShell> presShell;
  docShell->GetPresShell(getter_AddRefs(presShell));
  NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

  // Get the root view, or the "viewport view"
  nsIViewManager *viewManager = presShell->GetViewManager();
  NS_ENSURE_TRUE(viewManager, NS_ERROR_FAILURE);

  viewManager->GetRootView(view);
  NS_ENSURE_TRUE(view, NS_ERROR_FAILURE);

  // get the widget for the view
  nsIWidget* widget = view->GetWidget();
  NS_ENSURE_TRUE(widget, NS_ERROR_FAILURE);

  widget->Invalidate(PR_TRUE);

  return NS_OK;
}

nsresult
BrowserGlue::LoadURI(const char *aURI)
{
  printf("BrowserGlue::LoadURI(%s)\n",aURI);
  CopyUTF8toUTF16(aURI, mURI);
  mWebNav->LoadURI( mURI.get(),                        // URI string
                    nsIWebNavigation::LOAD_FLAGS_NONE, // Load flags
                    nsnull,                            // Referring URI
                    nsnull,                            // Post data
                    nsnull);                           // extra headers
  return NS_OK;
}

nsresult
BrowserGlue::SetOutputFile(const char *aFilename)
{
  NS_ENSURE_ARG(aFilename);
  // figure out how to get a char* into a ns[C]String here
  CopyUTF8toUTF16(aFilename, mFilename);
  printf("BrowserGlue[%x]::SetOutputFile(%s)\n",this,ToNewCString(mFilename));
  return NS_OK;
}





