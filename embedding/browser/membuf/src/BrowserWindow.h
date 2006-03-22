// License Block

#ifndef BROWSER_WINDOW_H
#define BROWSER_WINDOW_H

#include <nsIBaseWindow.h>
#include <nsIEmbeddingSiteWindow2.h>
#include <nsIInterfaceRequestor.h>
#include <nsITooltipListener.h>
#include <nsIView.h>
#include <nsIWebBrowserChrome.h>
#include <nsIWebBrowserChromeFocus.h>
#include <nsStringAPI.h>
#include <nsString.h>
#include <nsWeakReference.h>

#include <membufAllegroDefines.h>
#include "allegro.h"

// forward declarations
class BrowserGlue;

class BrowserWindow : public nsIWebBrowserChrome,
                      public nsIWebBrowserChromeFocus,
                      public nsIEmbeddingSiteWindow,
                      public nsITooltipListener,
                      public nsIInterfaceRequestor
{

public:
  BrowserWindow();
  virtual ~BrowserWindow();

  nsresult Init( BrowserGlue *aOwner, PRInt32 aWidth, PRInt32 aHeight );
  nsresult CreateWindow();
  BITMAP* GetBitmap();

  unsigned char* GetBuffer();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIWEBBROWSERCHROME
  NS_DECL_NSIWEBBROWSERCHROMEFOCUS
  NS_DECL_NSIEMBEDDINGSITEWINDOW
  NS_DECL_NSITOOLTIPLISTENER
  NS_DECL_NSIINTERFACEREQUESTOR

  nsresult GetWidget(nsIWidget **aWidget);

protected:

  nsresult GetView(nsIView **aView);

  // handle back to our owner
  BrowserGlue *mOwner;

  // XPCOM handles
  nsCOMPtr<nsIWebBrowser>  mWebBrowser; // a proxy to the actual object
  nsCOMPtr<nsIBaseWindow>  mBaseWindow; // QI of mWebBrowser
  nsCOMPtr<nsIWidget>      mWidget;
  
  // Allegro BITMAP struct
  BITMAP*                  mBitmap;

  // State
  PRBool mVisibility;
  PRBool mIsModal;
  nsString mTitle;
  nsString mJSStatus;
  nsString mLinkMessage;
  PRUint32 mChromeFlags;
  PRInt32 mWidth;
  PRInt32 mHeight;

};

#endif /* BROWSER_WINDOW_H */

// EOF

