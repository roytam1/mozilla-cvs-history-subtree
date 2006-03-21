/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Membuf server code
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by the Initial Developer are
 * Copyright (C) 2003 the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Joe Hewitt <hewitt@netscape.com>
 *    Stuart Parmenter <pavlov@netscape.com>
 *    Ben Goodger <ben@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 */

#include "membufBrowser.h"
#include "membufDirProvider.h"

#include "nsIGenericFactory.h"
#include "nsIComponentManager.h"
#include "nsILocalFile.h"

#include "nsCWebBrowser.h"

#include "nsIWebBrowserSetup.h"

#include "nsEmbedAPI.h"

#include "nsIWebNavigation.h"
#include "nsIDocShell.h"

#include "nsIProfile.h"

#include "nsString.h"
#include "nsReadableUtils.h"

#include "nsIWidget.h"
#include "nsWidgetsCID.h"

#include "nsIPresShell.h"
#include "nsPresContext.h"

#include "nsIViewManager.h"
#include "nsIView.h"

#include "nsIContent.h"
#include "nsILink.h"
#include "nsIDocument.h"
//#include "nsIHTMLContent.h"
#include "nsIForm.h"
#include "nsIFormControl.h"
//#include "nsHTMLValue.h"
#include "nsImageMapUtils.h"
#include "nsIDocumentViewer.h"

#include "nsIFrame.h"
#include "nsIFrameDebug.h"
#include "nsIImageFrame.h"
#include "nsIDocShellTreeNode.h"

#include "imgIRequest.h"
#include "imgIContainer.h"

#include "nsIDOMDocument.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMElement.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMHTMLMapElement.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLSelectElement.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsIDOMHTMLOptionElement.h"

#include "nsIScrollableView.h"

#include "gfxIImageFrame.h"
#include "nsIImage.h"

#include "nsIMIMEInputStream.h"
#include "nsIURI.h"
#include "nsNetUtil.h"

#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceDefs.h"
#include "nsProfileDirServiceProvider.h"

#include <sys/io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <allegro.h>
#include <signal.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define MEMBUF_NATIVE_BITMAP               7700
#define MEMBUF_NATIVE_BITMAP_PARENT        7701
#define MEMBUF_NATIVE_BITMAP_DIRTY_FLAG    7702
#define MEMBUF_NATIVE_BITMAP_REFCOUNT      7703

static NS_DEFINE_CID(kWindowCID, NS_WINDOW_CID);
static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

//////////////////////////////////////////////

//static membufBrowser *gBrowser;

nsresult StartupProfile();
nsresult SelectDefaultProfile();
nsresult GetCanvasFrame(nsIWebBrowser *aWebBrowser, nsIFrame **aFrame);

//////////////////////////////////////////////

NS_IMPL_ADDREF(membufBrowser)
NS_IMPL_RELEASE(membufBrowser)

NS_INTERFACE_MAP_BEGIN(membufBrowser)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIWebBrowserChrome)
   NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
   NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChrome)
   NS_INTERFACE_MAP_ENTRY(nsIWebProgressListener)
   NS_INTERFACE_MAP_ENTRY(nsIEmbeddingSiteWindow)
   NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END

//////////////////////////////////////////////
//// membufBrowser public

bool membufBrowser::need_embedding_init = true;

membufBrowser::membufBrowser() : mWidget(0),
                                 mBitmap(0),
                                 mLoadCount(0)
{
    printf("membufBrowser::membufBrowser() \n");
    NS_INIT_ISUPPORTS();
}

membufBrowser::~membufBrowser()
{
    printf("membufBrowser::membufBrowser() \n");
    nsWeakPtr weakling(
      dont_AddRef(NS_GetWeakReference((nsIWebProgressListener*)this)));
    mWebBrowser->RemoveWebBrowserListener(weakling, NS_GET_IID(nsIWebProgressListener));
}

nsresult
membufBrowser::Init()
{
    printf("membufBrowser::Init() - 1 \n");
    nsresult rv;

    if(need_embedding_init) {
        // Initialize Gecko embedding
        membufDirProvider *provider = new membufDirProvider();

        printf("membufBrowser::Init() - 2 \n");

        NS_ADDREF(provider);
        rv = NS_InitEmbedding(nsnull, provider);
        //rv = NS_InitEmbedding(nsnull, nsnull);
        NS_ENSURE_SUCCESS(rv, rv);

        printf("membufBrowser::Init() - 3 \n");

        rv = StartupProfile();
        NS_ENSURE_SUCCESS(rv, rv);

        printf("membufBrowser::Init() - 4 \n");

        // Use the default profile under the "Membuf" directory
        //rv = SelectDefaultProfile();
        //NS_ENSURE_SUCCESS(rv, rv);

        printf("membufBrowser::Init() - 5 \n");

        need_embedding_init = false;
    }

    mWebBrowser = do_CreateInstance(NS_WEBBROWSER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    printf("membufBrowser::Init() - 6 \n");

    mWebBrowser->SetContainerWindow
                        (NS_STATIC_CAST(nsIWebBrowserChrome*, this));

    nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(mWebBrowser);

    if (!baseWindow)
      printf("crap!!! no baseWindow\n");

    printf("membufBrowser::Init() - 7 \n");

    // need to do this to start the allegro environment.
    //mWidget = do_CreateInstance(kWindowCID, &rv);

    // make a bitmap that we can hang on to.
    //mBitmap = create_bitmap_ex(32, 800, 600);

    //printf("membufBrowser::Init() - created TOP BITMAP: [%x]\n", mBitmap);
/*
    if (NS_FAILED(rv)) 
      printf("failed to create a new widget\n");
    else
      printf("created mWidget: [%x]\n", &mWidget);
    baseWindow->InitWindow(nsnull, mWidget, 0, 0, 800, 600);
*/

    // initialize and create the browser window
    //baseWindow->InitWindow(mBitmap, nsnull, 0, 0, 800, 600);

    // pass in -1 to have the membufWindow create a BITMAP locally
    baseWindow->InitWindow((void*)-1, nsnull, 0, 0, 800, 600);
    rv = baseWindow->Create();
    NS_ENSURE_SUCCESS(rv, rv);

    // get the main widget from the baseWindow
    baseWindow->GetMainWidget(getter_AddRefs(mWidget));

    // get the bitmap from the main widget
    mBitmap = (BITMAP*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP);

    printf("membufBrowser::Init() - 8 \n");

    // Listen to page load progress
    nsWeakPtr weakling(
        dont_AddRef(NS_GetWeakReference((nsIWebProgressListener*)this)));
    mWebBrowser->AddWebBrowserListener(weakling, NS_GET_IID(nsIWebProgressListener));

    printf("membufBrowser::Init() - 9 \n");

    // Create our application shell
    mAppShell = do_CreateInstance(kAppShellCID);
    NS_ENSURE_SUCCESS(rv, rv);

    printf("membufBrowser::Init() - 10 \n");

    // we have an appshell, get it started
    mAppShell->Spinup();

    /*nsCOMPtr<nsIDocShell> docShell = do_GetInterface(mWebBrowser);
    nsCOMPtr<nsIPresShell> presShell;
    docShell->GetPresShell(getter_AddRefs(presShell));
    NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);
    nsCOMPtr<nsIPresContext> presContext;
    presShell->GetPresContext(getter_AddRefs(presContext));*/

    return rv;
}

nsresult
membufBrowser::SetViewportSize(nscoord aWidth, nscoord aHeight)
{
    printf("membufBrowser::SetViewportSize() \n");
    // Another way of doing it???
    nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(mWebBrowser);
    baseWindow->SetSize(aWidth, aHeight, PR_FALSE);

#if 0
    // Get the presShell for the currently loaded document
    nsCOMPtr<nsIDocShell> docShell = do_GetInterface(mWebBrowser);
    nsCOMPtr<nsIPresShell> presShell;
    docShell->GetPresShell(getter_AddRefs(presShell));
    NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);
    nsCOMPtr<nsIPresContext> presContext;
    presShell->GetPresContext(getter_AddRefs(presContext));

    float p2t = presContext->TwipsToPixels();

    // Reflow the document with the dimensions of the canvas frame
    return presShell->ResizeReflow(aWidth * p2t, aHeight * p2t);
#endif
    return NS_OK;
}

nsresult
membufBrowser::GetCanvasSize(nscoord *aWidth, nscoord *aHeight)
{
    printf("membufBrowser::GetCanvasSize() \n");
    nsIFrame *canvasFrame = nsnull;
    nsresult rv = GetCanvasFrame(&canvasFrame);
    NS_ENSURE_SUCCESS(rv, rv);

    nsRect canvasRect = canvasFrame->GetRect();
    *aWidth = canvasRect.width;
    *aHeight = canvasRect.height;

    nsCOMPtr<nsIDocShell> docShell = do_GetInterface(mWebBrowser);
    nsCOMPtr<nsIPresShell> presShell;
    docShell->GetPresShell(getter_AddRefs(presShell));
    NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);
    nsPresContext *presContext = 
               presShell->GetPresContext();
    float t2p = presContext->TwipsToPixels();

    *aWidth = canvasRect.width * t2p;
    *aHeight = canvasRect.height * t2p;

    return NS_OK;
}

nsresult membufBrowser::GetBitmap(BITMAP** bm, bool** dirty, int** refcount)
{
    printf("membufBrowser::GetBitmap() \n");
    nsIView* view;
    GetViewportView(&view);

    nsIWidget* widget = view->GetWidget();
    *bm = (BITMAP*)widget->GetNativeData(MEMBUF_NATIVE_BITMAP_PARENT);
    *dirty = (bool*)widget->GetNativeData(MEMBUF_NATIVE_BITMAP_DIRTY_FLAG);
    *refcount = (int*)widget->GetNativeData(MEMBUF_NATIVE_BITMAP_REFCOUNT);

/*
    *bm = (BITMAP*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP_PARENT);
    *dirty = (bool*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP_DIRTY_FLAG);
    *refcount = (int*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP_REFCOUNT);
*/

    return NS_OK;
}

nsresult
membufBrowser::ResizeViewportToFitCanvas()
{
    printf("membufBrowser::ResizeViewportToFitCanvas() \n");
    // Get the dimensions of the canvas frame
    nsIFrame *canvasFrame = nsnull;
    nsresult rv = GetCanvasFrame(&canvasFrame);
    NS_ENSURE_SUCCESS(rv, rv);
    nsRect canvasRect = canvasFrame->GetRect();

    // Get the presShell for the currently loaded document
    nsCOMPtr<nsIDocShell> docShell = do_GetInterface(mWebBrowser);
    nsCOMPtr<nsIPresShell> presShell;
    docShell->GetPresShell(getter_AddRefs(presShell));
    NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

    // Reflow the document with the dimensions of the canvas frame
    return presShell->ResizeReflow(canvasRect.width, canvasRect.height);
}

nsresult
membufBrowser::RunAppShell()
{
    printf("membufBrowser::RunAppShell() \n");
    //mAppShell->Spinup();
    mAppShell->Run();
    return NS_OK;
}

nsresult
membufBrowser::LoadURI(const char *aURI, const char* aSavefilename)
{
    printf("membufBrowser::LoadURI() \n");
    mLoadCount = 0;
    nsString uri = NS_ConvertASCIItoUCS2(aURI);

    nsCOMPtr<nsIInputStream> postData;
    nsCString query;

#if 0
    if (method == 1) {
        const char *q = strchr(aURI, '?');

        if (q) {
            query = q + 1;

            uri.SetLength(q - aURI);

            nsresult rv;
            nsCOMPtr<nsIInputStream> dataStream;
            rv = NS_NewCStringInputStream(getter_AddRefs(dataStream), query);
            NS_ENSURE_SUCCESS(rv, rv);

            nsCOMPtr<nsIMIMEInputStream> mimeStream(do_CreateInstance("@mozilla.org/network/mime-input-stream;1", &rv));
            NS_ENSURE_SUCCESS(rv, rv);

            mimeStream->AddHeader("Content-Type", "application/x-www-form-urlencoded");
            mimeStream->SetAddContentLength(PR_TRUE);
            mimeStream->SetData(dataStream);

            postData = mimeStream;
        }
    }
#endif

    // Tell Gecko to load the URI
    nsCOMPtr<nsIWebNavigation> webNav = do_QueryInterface(mWebBrowser);
    nsresult rv = webNav->LoadURI(uri.get(),
                                  nsIWebNavigation::LOAD_FLAGS_NONE,
                                  nsnull, postData, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    RunAppShell();
    ResizeViewportToFitCanvas();
    DumpGraphics();

    nsIView* view;
    GetViewportView(&view);

    nsIWidget* widget = view->GetWidget();
    printf("XXXjgaunt - widget [%x]\n", widget);

/*
    if (&mWidget != 0)
      printf("XXXjgaunt - mWidget [%x]\n", &mWidget);
    else
      printf("XXXjgaunt - NO mWidget!!!\n");
*/
    

    // get the bitmap data from the nsIWidget (membufWindow)
    BITMAP* bmp = (BITMAP*)widget->GetNativeData(MEMBUF_NATIVE_BITMAP_PARENT);
    BITMAP* mbmp = (BITMAP*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP_PARENT);
    printf("XXXjgaunt - bitmap from view [%x]\n", bmp);
    printf("XXXjgaunt - bitmap from mWidget [%x]\n", mbmp);
    printf("XXXjgaunt - mBitmap data member [%x]\n", mBitmap);

    // save the bitmap to a file
    printf("Saving to '%s'\n", aSavefilename);
    allegro_errno = &errno;
    //save_tga(aSavefilename, bmp, 0);
    save_tga(aSavefilename, mBitmap, 0);

    return NS_OK;
}

// Force a global paint message
nsresult
membufBrowser::DumpGraphics()
{
    printf("membufBrowser::DumpGraphics() \n");
    // Get the scrollport widget for the currently loaded document
    nsIView *scrollPortView = nsnull;
    nsresult rv = GetScrollPortView(&scrollPortView);
    NS_ENSURE_SUCCESS(rv, rv);

    if(!scrollPortView) {
        printf("uh oh, GetViewportView no workie\n");
        return NS_OK;
    }
    nsCOMPtr<nsIWidget> widget = scrollPortView->GetWidget();
    //nsCOMPtr<nsIWidget> widget = mWidget;

    if(widget) {
        // Update the widget, which kicks off a full paint
        printf("membufBroswer::DumpGraphics() - calling Invalidate(PR_TRUE)\n");
        widget->Invalidate(PR_TRUE);
    } else {
        printf("XXXjgaunt  - no widget from scrollportView, trying viewportview \n");
        nsIView *viewPortView = nsnull;
        nsresult rv = GetViewportView(&viewPortView);
        NS_ENSURE_SUCCESS(rv, rv);

        if(!viewPortView) {
            printf("uh oh, GetViewportView no workie\n");
            return NS_OK;
        }
        widget = viewPortView->GetWidget();

        if(widget) {
            // Update the widget, which kicks off a full paint
            printf("membufBroswer::DumpGraphics() - calling Update()\n");
            widget->Update();
        }
        else
          printf("XXXjgaunt  - no widget from viewportView,totally screwed  \n");
    }
    return NS_OK;
}

//////////////////////////////////////////////
//// nsIWebBrowserChrome

NS_IMETHODIMP
membufBrowser::SetStatus(PRUint32 aType, const PRUnichar* aStatus)
{
    printf("membufBrowser::SetStatus() \n");
    return NS_OK;
}

NS_IMETHODIMP
membufBrowser::GetWebBrowser(nsIWebBrowser** aWebBrowser)
{
    printf("membufBrowser::GetWebBrowser() \n");
    NS_ENSURE_ARG_POINTER(aWebBrowser);
    *aWebBrowser = mWebBrowser;
    NS_IF_ADDREF(*aWebBrowser);
    return NS_OK;
}

NS_IMETHODIMP
membufBrowser::SetWebBrowser(nsIWebBrowser* aWebBrowser)
{
    printf("membufBrowser::SetWebBrowser() \n");
    mWebBrowser = aWebBrowser;
    return NS_OK;
}

NS_IMETHODIMP
membufBrowser::GetChromeFlags(PRUint32* aChromeMask)
{
    printf("membufBrowser::GetChromeFlags() \n");
    *aChromeMask = mChromeFlags;
    return NS_OK;
}

NS_IMETHODIMP
membufBrowser::SetChromeFlags(PRUint32 aChromeMask)
{
    printf("membufBrowser::SetChromeFlags() \n");
    mChromeFlags = aChromeMask;
    return NS_OK;
}

NS_IMETHODIMP
membufBrowser::DestroyBrowserWindow(void)
{
    printf("membufBrowser::DestroyBrowserWindow() \n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
membufBrowser::SizeBrowserTo(PRInt32 aWidth, PRInt32 aHeight)
{
    printf("membufBrowser::SizeBrowserTo() \n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
membufBrowser::ShowAsModal(void)
{
    printf("membufBrowser::ShowAsModal() \n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
membufBrowser::IsWindowModal(PRBool *_retval)
{
    printf("membufBrowser::IsWindowModal() \n");
    *_retval = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
membufBrowser::ExitModalEventLoop(nsresult aStatus)
{
    printf("membufBrowser::ExitModalEventLoop() \n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

//////////////////////////////////////////////
//// nsIEmbeddingSiteWindow

NS_IMETHODIMP
membufBrowser::SetDimensions(PRUint32 aFlags, PRInt32 x, PRInt32 y, PRInt32 cx, PRInt32 cy)
{
    printf("membufBrowser::SetDimensions() \n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
membufBrowser::GetDimensions(PRUint32 aFlags, PRInt32 *x, PRInt32 *y, PRInt32 *cx, PRInt32 *cy)
{
    printf("membufBrowser::GetDimensions() \n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
membufBrowser::SetFocus()
{
    printf("membufBrowser::SetFocus() \n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
membufBrowser::GetTitle(PRUnichar * *aTitle)
{
    printf("membufBrowser::GetTitle() \n");
   *aTitle = nsnull;
   return NS_OK;
}

NS_IMETHODIMP
membufBrowser::SetTitle(const PRUnichar * aTitle)
{
    printf("membufBrowser::SetTitle() \n");
    return NS_OK;
}

NS_IMETHODIMP
membufBrowser::GetVisibility(PRBool * aVisibility)
{
    printf("membufBrowser::GetVisibility() \n");
    *aVisibility = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
membufBrowser::SetVisibility(PRBool aVisibility)
{
    printf("membufBrowser::SetVisibility() \n");
    return NS_OK;
}

NS_IMETHODIMP
membufBrowser::GetSiteWindow(void **aSiteWindow)
{
    printf("membufBrowser::GetSiteWindow() \n");
   *aSiteWindow = this;
   return NS_OK;
}

//////////////////////////////////////////////
//// nsIWebProgressListener

NS_IMETHODIMP
membufBrowser::OnProgressChange(nsIWebProgress* aProgress,
                              nsIRequest* aRequest,
                              PRInt32 aCurSelfProgress,
                              PRInt32 aMaxSelfProgress,
                              PRInt32 aCurTotalProgress,
                              PRInt32 aMaxTotalProgress)
{
    printf("membufBrowser::OnProgressChange() \n");
    return NS_OK;
}


/// BEGIN DEBUGGING STUFF

already_AddRefed<nsIContentViewer>
doc_viewer(nsIDocShell *aDocShell)
{
    if (!aDocShell)
        return nsnull;
    nsIContentViewer *result;
    aDocShell->GetContentViewer(&result);
    return result;
}

already_AddRefed<nsIPresShell>
pres_shell(nsIDocShell *aDocShell)
{
    nsCOMPtr<nsIDocumentViewer> dv =
        do_QueryInterface(nsCOMPtr<nsIContentViewer>(doc_viewer(aDocShell)));
    if (!dv)
        return nsnull;
    nsIPresShell *result = nsnull;
    dv->GetPresShell(&result);
    return result;
}


void DumpFramesRecur(nsIDocShell* aDocShell, FILE* out)
{
    if (nsnull != aDocShell) {
        fprintf(out, "webshell=%p \n", NS_STATIC_CAST(void*, aDocShell));
        nsCOMPtr<nsIPresShell> shell(pres_shell(aDocShell));
        if (shell) {
            nsIFrame* root = shell->GetRootFrame();
            if (root) {
                nsPresContext *presContext =
                      shell->GetPresContext();

                nsIFrameDebug* fdbg;
                if (NS_SUCCEEDED(CallQueryInterface(root, &fdbg))) {
                    fdbg->List(presContext, out, 0);
                }
            }
        }
        else {
            fputs("null pres shell\n", out);
        }

        // dump the frames of the sub documents
        PRInt32 i, n;
        nsCOMPtr<nsIDocShellTreeNode> docShellAsNode(do_QueryInterface(aDocShell));
        docShellAsNode->GetChildCount(&n);
        for (i = 0; i < n; ++i) {
            nsCOMPtr<nsIDocShellTreeItem> child;
            docShellAsNode->GetChildAt(i, getter_AddRefs(child));
            nsCOMPtr<nsIDocShell> childAsShell(do_QueryInterface(child));
            if (childAsShell) {
                DumpFramesRecur(childAsShell, out);
            }
        }
    }
}

/// END DEBUGGING STUFF


NS_IMETHODIMP
membufBrowser::OnStateChange(nsIWebProgress* aProgress,
                           nsIRequest* aRequest,
                           PRUint32 aProgressStateFlags,
                           nsresult aStatus)
{
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
            //ResizeViewportToFitCanvas();
            //SetViewportSize(1024, 1024);
            //DumpGraphics();

            mAppShell->Exit();
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
membufBrowser::OnLocationChange(nsIWebProgress* aWebProgress,
                              nsIRequest* aRequest,
                              nsIURI* aURI)
{
    printf("membufBrowser::OnLocationChange() \n");
    return NS_OK;
}

NS_IMETHODIMP
membufBrowser::OnStatusChange(nsIWebProgress* aWebProgress,
                            nsIRequest* aRequest,
                            nsresult aStatus,
                            const PRUnichar* aMessage)
{
    return NS_OK;
}

NS_IMETHODIMP
membufBrowser::OnSecurityChange(nsIWebProgress *aWebProgress,
                              nsIRequest *aRequest,
                              PRUint32 state)
{
    printf("membufBrowser::OnSecurityChange() \n");
    return NS_OK;
}

//////////////////////////////////////////////
//// nsIInterfaceRequestor

NS_IMETHODIMP
membufBrowser::GetInterface(const nsIID &aIID, void** aInstancePtr)
{
    printf("membufBrowser::GetInterface() \n");
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

//////////////////////////////////////////////
//// Utilities

nsresult
StartupProfile()
{
  printf("StartupProfile()\n");

  nsCOMPtr<nsIFile> appDataDir;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_APPLICATION_REGISTRY_DIR,
                                       getter_AddRefs(appDataDir));
  if (NS_FAILED(rv))
    return rv;
                                                                                
  appDataDir->Append(NS_LITERAL_STRING("membuf"));
  nsCOMPtr<nsILocalFile> localAppDataDir(do_QueryInterface(appDataDir));
                                                                                
  nsCOMPtr<nsProfileDirServiceProvider> locProvider;
  NS_NewProfileDirServiceProvider(PR_TRUE, getter_AddRefs(locProvider));
  if (!locProvider)
    return NS_ERROR_FAILURE;
                                                                                
  rv = locProvider->Register();
  if (NS_FAILED(rv))
    return rv;
                                                                                
  return locProvider->SetProfileDir(localAppDataDir);
}

nsresult
SelectDefaultProfile()
{
    printf("SelectDefaultProfile()\n");
    nsresult rv;
    nsCOMPtr<nsIProfile> profileService =
             do_GetService(NS_PROFILE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt32 profileCount = 0;
    rv = profileService->GetProfileCount(&profileCount);
    if (profileCount == 0)
    {
        // Make a new default profile
        NS_NAMED_LITERAL_STRING(newProfileName, "default");
        rv = profileService->CreateNewProfile(newProfileName.get(), nsnull, nsnull, PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = profileService->SetCurrentProfile(newProfileName.get());
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else if (profileCount == 1)
    {
        // Use the current profile
        nsXPIDLString   currProfileName;
        rv = profileService->GetCurrentProfile(getter_Copies(currProfileName));
        if (NS_FAILED(rv)) return FALSE;
        rv = profileService->SetCurrentProfile(currProfileName);
        if (NS_FAILED(rv)) return FALSE;
    }

    return NS_OK;
}

nsresult
membufBrowser::GetViewportView(nsIView **aView)
{
    printf("membufBrowser::GetViewportView() \n");
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

nsresult
membufBrowser::GetScrollPortView(nsIView **aView)
{
    printf("membufBrowser::GetScrollPortView() \n");
    nsIWebBrowser *aWebBrowser = mWebBrowser;

    // Get the presShell for the currently loaded document
    nsCOMPtr<nsIDocShell> docShell = do_GetInterface(aWebBrowser);

    nsCOMPtr<nsIPresShell> presShell;
    docShell->GetPresShell(getter_AddRefs(presShell));

    NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

    // Get the root view, or to "viewport view"
    nsIViewManager *viewManager = presShell->GetViewManager();
    NS_ENSURE_TRUE(viewManager, NS_ERROR_FAILURE);

    nsIView *viewportView = nsnull;
    viewManager->GetRootView(viewportView);
    NS_ENSURE_TRUE(viewportView, NS_ERROR_FAILURE);

    // This is kind-of a guessing game.  The scroll port view always
    // seems to be the first visible child of the viewport view
    nsIView *childView = viewportView->GetFirstChild();
    while (childView) {
        nsViewVisibility vis = childView->GetVisibility();
        if (vis == nsViewVisibility_kShow) {
            *aView = childView;
            break;
        }
        childView = childView->GetNextSibling();
    }


    if (!childView) {
        *aView = nsnull;
        return NS_ERROR_FAILURE;
    }


    /*nsCOMPtr<nsIScrollableView> scroll(do_QueryInterface(*aView));
    if(scroll) {
        printf("it is a scrollable view!\n");
        }*/

    return NS_OK;
}

nsresult
membufBrowser::GetCanvasFrame(nsIFrame **aFrame)
{
    printf("membufBrowser::GetCanvasFrame() \n");
//    nsIWebBrowser *aWebBrowser = mWebBrowser;

    // Get the view corresponding to the canvas frame
    nsIView *scrollPortView = nsnull;
    nsresult rv = GetScrollPortView(&scrollPortView);
    NS_ENSURE_SUCCESS(rv, rv);
    nsIView *canvasView = scrollPortView->GetFirstChild();

    // Get the dimensions of the canvas frame
    void *clientData = canvasView->GetClientData();
    if (!clientData) {
      printf("XXXjgaunt - GetCanvasFrame, trying the viewport\n");
      // didn't get what we wanted from the scrollport, try the viewport
      nsIView *viewportView = nsnull;
      rv = GetViewportView(&viewportView);
      NS_ENSURE_SUCCESS(rv, rv);

      canvasView = scrollPortView->GetFirstChild();
      clientData = canvasView->GetClientData();
    }
    *aFrame = (nsIFrame *)clientData;
    NS_ENSURE_TRUE(*aFrame, NS_ERROR_FAILURE);

    return NS_OK;
}

//////////////////////////////////////////////
//// Content model serialization

#if 0
static nsresult
GetFrameSize(nsIFrame *aFrame, nsRect &aRect, nsPresContext *aPresContext)
{
    float t2p = aPresContext->TwipsToPixels();

    aRect = aFrame->GetRect(); /* XXX
                             we need to see if 'flow' is an imageframe and if it is
                             return the size of the image instead so that the
                             anchor covers the entire image */

    nsPoint pt;
    nsIView *view;
    aFrame->GetOffsetFromView(pt, &view);

    aRect.MoveTo(pt);

    while (view) {
        pt = view->GetPosition();
        aRect.MoveBy(pt.x, pt.y);
        view = view->GetParent();
    }

    aRect.x = NSTwipsToIntPixels(aRect.x, t2p);
    aRect.y = NSTwipsToIntPixels(aRect.y, t2p);
    aRect.width = NSTwipsToIntPixels(aRect.width, t2p);
    aRect.height = NSTwipsToIntPixels(aRect.height, t2p);

    return NS_OK;
}


static nsresult
GetNameAndValue(nsIFormControl *aFormControl, uint32 *aName, uint32 *aValue)
{
    nsAutoString uname, uvalue;

    nsCOMPtr<nsIDOMHTMLInputElement> inputElement(do_QueryInterface(aFormControl));
    if (inputElement) {
        inputElement->GetName(uname);
        inputElement->GetValue(uvalue);
    } else {
        nsCOMPtr<nsIDOMHTMLTextAreaElement> textareaElement(do_QueryInterface(aFormControl));
        if (textareaElement) {
            textareaElement->GetName(uname);
            textareaElement->GetValue(uvalue);
        } else {
            nsCOMPtr<nsIDOMHTMLSelectElement> selectElement(do_QueryInterface(aFormControl));
            if (selectElement) {
                selectElement->GetName(uname);
            } else {
                return NS_ERROR_FAILURE;
            }
        }
    }



    /*if (aName) {
        NS_ConvertUCS2toUTF8 name(uname);
        *aName = idmap_AddString(ADTM_IDMAP(), name.get(), name.Length());
    }
    if (aValue) {
        NS_ConvertUCS2toUTF8 value(uvalue);
        *aValue = idmap_AddString(ADTM_IDMAP(), value.get(), value.Length());
    }
    */
    /* XXX we need to be able to get the raw password out of password fields */

    return NS_OK;
}

#endif
