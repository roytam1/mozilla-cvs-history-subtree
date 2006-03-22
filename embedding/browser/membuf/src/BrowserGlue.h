// License Block

#ifndef MOZ_EMBED_IMPL_H
#define MOZ_EMBED_IMPL_H

#include <nsStringAPI.h>

// plugin headers
#include "BrowserContainer.h"
#include "BrowserDirProvider.h"
#include "BrowserProgress.h"
#include "BrowserWindow.h"

// mozilla headers
#include <nsIAppShell.h>
#include <nsIBaseWindow.h>
#include <nsProfileDirServiceProvider.h>
#include <nsIWebBrowser.h>
#include <nsIWebNavigation.h>
#include <nsWeakReference.h>

#include <membufAllegroDefines.h>
#include "allegro.h"

// forward declare the embedding container class
class BrowserContainer;

/**
 * This class represents the API exposed by the membuf embedding
 *  side. Embeddors should instantiate this class and use it to
 *  interact with mozilla.
 */
class BrowserGlue 
{
  public:

    BrowserGlue();
    virtual ~BrowserGlue();

    // These should be called in this order

    /**
     *
     */
    nsresult Init( BrowserContainer *aBrowserOwner,
                   PRInt32 aWidth,
                   PRInt32 aHeight );

    /**
     *
     */
    nsresult CreateBrowser();

    /**
     *
     */
    nsresult SetViewportSize(PRUint32 aWidth, PRUint32 aHeight);

    /**
     * Tells the embeddor that a change has happened to the bitmap
     *   by calling the BitampUpdated method of nsIBrowserContainer
     */
    nsresult UpdateBitmap();

    /**
     *
     */
    nsresult Run();

    /**
     * Tells the browser to load this page.
     */
    nsresult LoadURI(const char *aURI);

    /**
     *
     */
    nsresult SendKeyEvent(nsKeyEvent aKeyEvent);

    /**
     *
     */
    nsresult Shutdown();

    /**
     * For diagnostic output of the bitmap. Sets the name of the file
     *   that will be dumped.
     */
    nsresult SetOutputFile(const char *aFilename);

    static nsIAppShell *sAppShell;

  private:

    // called by Init to create and init the aggregate classes
    nsresult InitStage2();
    // Gets the widget for the current document and tells it to invalidate 
    nsresult Invalidate();

    // Sets the, unused, mDirProvider
    nsresult SetupProfile();


    // handle to the Embedding nsIBrowserContainer object
    BrowserContainer *mOwner;
    // handle to the mozilla browser (retrieved from BrowserWindow)
    nsCOMPtr<nsIWebBrowser> mWebBrowser;
    // nsIWebBrowserChrome object
    BrowserWindow *mBrowserWindow;
    // nsIWebProgress object
    BrowserProgress *mBrowserProgress;
    // not used yet, may only be useful on Trunk with XRE_NotifyProfile()
    BrowserDirProvider *mDirProvider;
    // Set to the default Directory Provider
    nsProfileDirServiceProvider *mProfDirProvider;
    // nsIWebNavigation object
    nsCOMPtr<nsIWebNavigation> mWebNav;
    // Have we handed the embeddor the bitmap
    PRBool mBitmapIsSet; 
    // the URL to load
    nsString mURI;
    // width of the browser window (& bitmap)
    PRInt32 mWidth;
    // height of the browser window (& bitmap)
    PRInt32 mHeight;

    // XXXjgaunt - ifdef debug me!!!
    // test file to dump to
    nsString mFilename;

};

#endif /* MOZ_EMBED_IMPL_H */

// EOF

