/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 tw=80 et cindent: */
/* ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Christopher Blizzard. Portions created by Christopher Blizzard are Copyright (C) Christopher Blizzard.  All Rights Reserved.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Christopher Blizzard <blizzard@mozilla.org>
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
 *
 * ***** END LICENSE BLOCK ***** */
/**
 * Derived from GContentHandler http://landfill.mozilla.org/mxr-test/gnome/source/galeon/mozilla/ContentHandler.cpp
 */
#include "EmbedDownloadMgr.h"
#include "EmbedGtkTools.h"
#ifdef MOZILLA_INTERNAL_API
#include "nsXPIDLString.h"
#else
#include "nsComponentManagerUtils.h"
#endif
#include "nsIChannel.h"
#include "nsIWebProgress.h"
#include "nsIDOMWindow.h"
#include "nsIURI.h"
#include "nsCRT.h"
#include "nsIPromptService.h"
#include "nsIWebProgressListener2.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIURI.h"
#include "nsIURL.h"
#include "nsIFile.h"
#include "nsIDOMWindow.h"
#include "nsIExternalHelperAppService.h"
#include "nsCExternalHandlerService.h"
#include "nsMemory.h"
#include "nsNetError.h"
#include "nsIStreamListener.h"
#include "nsIFile.h"
#include "nsILocalFile.h"
#include "nsNetCID.h"
#include <unistd.h>
#include <gtkmozembed_download.h>

#define UNKNOWN_FILE_SIZE -1

class EmbedDownloadMgr;
class ProgressListener : public nsIWebProgressListener2
{
public:
    ProgressListener (EmbedDownload *aDownload)
    {
        mDownload = aDownload;
    };

    ~ProgressListener (void)
    {
    };

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBPROGRESSLISTENER
    NS_DECL_NSIWEBPROGRESSLISTENER2

    EmbedDownload *mDownload;
};

NS_IMPL_ISUPPORTS2 (ProgressListener, nsIWebProgressListener2, nsIWebProgressListener)
NS_IMPL_ISUPPORTS1 (EmbedDownloadMgr, nsIHelperAppLauncherDialog)

EmbedDownloadMgr::EmbedDownloadMgr(void)
{
}

EmbedDownloadMgr::~EmbedDownloadMgr(void)
{
}

NS_IMETHODIMP
EmbedDownloadMgr::RemoveSchemeFromFilePath (gchar **path)
{
  char** temp_splitv = NULL;
  char* temp_path = NULL;

  if (g_str_has_prefix (*path, "file://")) {
    temp_path = *path;

    temp_splitv = g_strsplit (*path, "file://", 2);
    *path = g_strdup(temp_splitv[1]);

    g_strfreev(temp_splitv);
    g_free(temp_path);
  }

  return NS_OK;
}

NS_IMETHODIMP
EmbedDownloadMgr::Show (nsIHelperAppLauncher *aLauncher,
                        nsISupports *aContext,
                        PRUint32 aForced)
{
  nsresult rv;

  /* create a Download object */
  GtkObject* instance = gtk_moz_embed_download_new ();
  mDownload = (EmbedDownload *) GTK_MOZ_EMBED_DOWNLOAD(instance)->data;
  mDownload->parent = instance;

  rv = GetDownloadInfo(aLauncher, aContext);

  /* Retrieve GtkMozEmbed object from DOM Window */
  nsCOMPtr<nsIDOMWindow> parentDOMWindow = do_GetInterface (aContext);
  mDownload->gtkMozEmbedParentWidget = GetGtkWidgetForDOMWindow(parentDOMWindow);

  gtk_signal_emit (GTK_OBJECT (mDownload->gtkMozEmbedParentWidget),
                   moz_embed_signals[DOWNLOAD_REQUEST],
                   mDownload->server,
                   mDownload->file_name,
                   mDownload->file_type,
                   (gulong) mDownload->file_size,
                   1);

  gtk_signal_emit (GTK_OBJECT (mDownload->parent),
                   moz_embed_download_signals[DOWNLOAD_STARTED_SIGNAL],
                   &mDownload->file_name_with_path);

  if (!mDownload->file_name_with_path) {
    gtk_moz_embed_download_do_command (GTK_MOZ_EMBED_DOWNLOAD (mDownload->parent),
                                       GTK_MOZ_EMBED_DOWNLOAD_CANCEL);
    return NS_OK;
  }

  rv = RemoveSchemeFromFilePath (&mDownload->file_name_with_path);

  aLauncher->SaveToDisk (nsnull, PR_FALSE);

  return NS_OK;
}

NS_METHOD
EmbedDownloadMgr::GetDownloadInfo (nsIHelperAppLauncher *aLauncher,
                                   nsISupports *aContext)
{
  nsresult rv = NS_OK;

  /* File type */
  nsCOMPtr<nsIMIMEInfo> mMIMEInfo;
  rv = aLauncher->GetMIMEInfo (getter_AddRefs(mMIMEInfo));
  if (NS_FAILED (rv))
    return NS_ERROR_FAILURE;

  nsCAutoString aMimeType;
  rv = mMIMEInfo->GetMIMEType (aMimeType);
  if (NS_FAILED (rv))
    return NS_ERROR_FAILURE;

  /* File name */
  nsCAutoString aTempFileName;
  nsAutoString aSuggestedFileName;
  rv = aLauncher->GetSuggestedFileName (aSuggestedFileName);

  if (NS_FAILED (rv))
    return NS_ERROR_FAILURE;

  aTempFileName = NS_ConvertUTF16toUTF8 (aSuggestedFileName);

  /* Complete source URL */
  nsCOMPtr<nsIURI> mUri;
  rv = aLauncher->GetSource (getter_AddRefs (mUri));
  if (NS_FAILED (rv))
    return NS_ERROR_FAILURE;

  nsCAutoString mSpec;
  rv = mUri->Resolve (NS_LITERAL_CSTRING ("."), mSpec);
  if (NS_FAILED (rv))
    return NS_ERROR_FAILURE;

  /* Sets download object to keep control of each download. */
  mDownload->launcher = aLauncher;
  mDownload->downloaded_size = -1;
  mDownload->file_name = g_strdup ((gchar *) aTempFileName.get ());
  mDownload->server = g_strconcat(mSpec.get(), (gchar *) mDownload->file_name, NULL);
  mDownload->file_type = g_strdup (aMimeType.get());
  mDownload->file_size = UNKNOWN_FILE_SIZE;

  return rv;
}

NS_IMETHODIMP EmbedDownloadMgr::PromptForSaveToFile (nsIHelperAppLauncher *aLauncher,
                                                     nsISupports *aWindowContext,
                                                     const PRUnichar *aDefaultFile,
                                                     const PRUnichar *aSuggestedFileExtension,
                                                     nsILocalFile **_retval)
{
  nsresult rv;

  nsCAutoString file_path;
  file_path.Assign (mDownload->file_name_with_path);

  nsCOMPtr <nsILocalFile> destFile;
  NS_NewNativeLocalFile (file_path,
                         PR_TRUE,
                         getter_AddRefs (destFile));

  NS_ADDREF (*_retval = destFile);

  /* Progress listener to follow the download and connecting it to
     the launcher which controls the download. */
  nsCOMPtr<nsIWebProgressListener2> listener = new ProgressListener(mDownload);
  rv = aLauncher->SetWebProgressListener (listener);
  if (NS_FAILED (rv))
    return NS_ERROR_FAILURE;

  return NS_OK;
}

/* nsIWebProgressListener Functions
   all these methods must be here due to nsIWebProgressListenr/2 inheritance */
NS_IMETHODIMP ProgressListener::OnStatusChange (nsIWebProgress *aWebProgress,
                                                nsIRequest *aRequest,
                                                nsresult aStatus,
                                                const PRUnichar *aMessage)
{
  if (NS_SUCCEEDED (aStatus))
    return NS_OK;

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP ProgressListener::OnStateChange (nsIWebProgress *aWebProgress,
                                               nsIRequest *aRequest, PRUint32 aStateFlags,
                                               nsresult aStatus)
{
  if (NS_FAILED (aStatus))
    return NS_ERROR_FAILURE;

  if (aStateFlags & STATE_STOP)
    gtk_signal_emit(GTK_OBJECT(mDownload->parent),
                    moz_embed_download_signals[DOWNLOAD_COMPLETED_SIGNAL]);

  return NS_OK;
}

NS_IMETHODIMP ProgressListener::OnProgressChange (nsIWebProgress *aWebProgress,
                                                  nsIRequest *aRequest, PRInt32 aCurSelfProgress,
                                                  PRInt32 aMaxSelfProgress, PRInt32 aCurTotalProgress,
                                                  PRInt32 aMaxTotalProgress)
{
  return OnProgressChange64 (aWebProgress,
                             aRequest,
                             aCurSelfProgress,
                             aMaxSelfProgress,
                             aCurTotalProgress,
                             aMaxTotalProgress);
}

NS_IMETHODIMP ProgressListener::OnLocationChange (nsIWebProgress *aWebProgress,
                                                  nsIRequest *aRequest, nsIURI *location)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP ProgressListener::OnSecurityChange (nsIWebProgress *aWebProgress,
                                                  nsIRequest *aRequest, PRUint32 state)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIWebProgressListener2 method */
NS_IMETHODIMP ProgressListener::OnProgressChange64 (nsIWebProgress *aWebProgress,
                                                    nsIRequest *aRequest, PRInt64 aCurSelfProgress,
                                                    PRInt64 aMaxSelfProgress, PRInt64 aCurTotalProgress,
                                                    PRInt64 aMaxTotalProgress)
{
  mDownload->request = aRequest;

  if (aMaxSelfProgress != UNKNOWN_FILE_SIZE) {
    gtk_signal_emit(GTK_OBJECT(mDownload->parent),
                    moz_embed_download_signals[DOWNLOAD_PROGRESS_SIGNAL],
                    (gulong) aCurSelfProgress, (gulong) aMaxSelfProgress, 1);
  }
  else {
    gtk_signal_emit(GTK_OBJECT(mDownload->parent),
                    moz_embed_download_signals[DOWNLOAD_PROGRESS_SIGNAL],
                    (gulong) aCurSelfProgress, 0, 1);
  }

  /* storing current downloaded size. */
  mDownload->downloaded_size = (gulong) aCurSelfProgress;

  return NS_OK;
}

NS_IMETHODIMP ProgressListener::OnRefreshAttempted(nsIWebProgress *aWebProgress,
                                                   nsIURI *aUri, PRInt32 aDelay,
                                                   PRBool aSameUri,
                                                   PRBool *allowRefresh)
{
  *allowRefresh = PR_TRUE;
  return NS_OK;
}

