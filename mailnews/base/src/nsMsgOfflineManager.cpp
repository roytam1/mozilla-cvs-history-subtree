/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 */

/*
 * The offline manager service - manages going online and offline, and synchronization
 */
#include "msgCore.h"
#include "nsMsgOfflineManager.h"
#include "nsIServiceManager.h"
#include "nsISupportsArray.h"
#include "nsMsgBaseCID.h"

NS_IMPL_THREADSAFE_ISUPPORTS5(nsMsgOfflineManager,
                              nsIMsgOfflineManager,
                              nsIMsgSendLaterListener,
                              nsIObserver,
                              nsISupportsWeakReference,
                              nsIUrlListener)

nsMsgOfflineManager::nsMsgOfflineManager() :
  m_inProgress (PR_FALSE),
  m_sendUnsentMessage(PR_FALSE),
  m_downloadNews(PR_FALSE),
  m_downloadMail(PR_FALSE),
  m_playbackOfflineImapOps(PR_FALSE),
  m_goOffineWhenDone(PR_FALSE),
  m_curState(eNoState),
  m_curOperation(eNoOp)
{
  NS_INIT_REFCNT();
}

nsMsgOfflineManager::~nsMsgOfflineManager()
{
}

/* attribute nsIMsgWindow window; */
NS_IMETHODIMP nsMsgOfflineManager::GetWindow(nsIMsgWindow * *aWindow)
{
  NS_ENSURE_ARG(aWindow);
  *aWindow = m_window;
  NS_IF_ADDREF(*aWindow);
  return NS_OK;
}
NS_IMETHODIMP nsMsgOfflineManager::SetWindow(nsIMsgWindow * aWindow)
{
  m_window = aWindow;
  if (m_window)
    m_window->GetStatusFeedback(getter_AddRefs(m_statusFeedback));
  else
    m_statusFeedback = nsnull;
  return NS_OK;
}

/* attribute boolean inProgress; */
NS_IMETHODIMP nsMsgOfflineManager::GetInProgress(PRBool *aInProgress)
{
  NS_ENSURE_ARG(aInProgress);
  *aInProgress = m_inProgress;
  return NS_OK;
}

NS_IMETHODIMP nsMsgOfflineManager::SetInProgress(PRBool aInProgress)
{
  m_inProgress = aInProgress;
  return NS_OK;
}


nsresult nsMsgOfflineManager::AdvanceToNextState()
{
  if (m_curOperation == eGoingOnline)
  {
    switch (m_curState)
    {
    case eSendingUnsent:

      m_curState = eSynchronizingOfflineImapChanges;
      if (m_playbackOfflineImapOps)
        return SynchronizeOfflineImapChanges();
      else
        AdvanceToNextState(); // recurse to next state.
      break;
    case eSynchronizingOfflineImapChanges:
      m_curState = eDone;
      break;
    }
  }
  else if (m_curOperation == eDownloadingForOffline)
  {
  }
  return NS_OK;
}

nsresult nsMsgOfflineManager::SynchronizeOfflineImapChanges()
{
  nsresult rv = NS_OK;
  return rv;
}

/* void goOnline (in boolean sendUnsentMessages, in boolean playbackOfflineImapOperations, in nsIMsgWindow aMsgWindow); */
NS_IMETHODIMP nsMsgOfflineManager::GoOnline(PRBool sendUnsentMessages, PRBool playbackOfflineImapOperations, nsIMsgWindow *aMsgWindow)
{
  m_sendUnsentMessage = sendUnsentMessages;
  m_playbackOfflineImapOps = playbackOfflineImapOperations;
  m_curOperation = eGoingOnline;
  SetWindow(aMsgWindow);
  if (!m_sendUnsentMessage && !playbackOfflineImapOperations)
  {
  }
  return NS_OK;
}

/* void synchronizeForOffline (in boolean downloadNews, in boolean downloadMail, in boolean sendUnsentMessages, in boolean goOfflineWhenDone, in nsIMsgWindow aMsgWindow); */
NS_IMETHODIMP nsMsgOfflineManager::SynchronizeForOffline(PRBool downloadNews, PRBool downloadMail, PRBool sendUnsentMessages, PRBool goOfflineWhenDone, nsIMsgWindow *aMsgWindow)
{
  m_curOperation = eDownloadingForOffline;
  return NS_OK;
}

  // nsIUrlListener methods

NS_IMETHODIMP
nsMsgOfflineManager::OnStartRunningUrl(nsIURI * aUrl)
{
    return NS_OK;
}

NS_IMETHODIMP
nsMsgOfflineManager::OnStopRunningUrl(nsIURI * aUrl, nsresult aExitCode)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgOfflineManager::Observe(nsISupports *aSubject, const PRUnichar *aTopic, const PRUnichar *someData)
{
  return NS_OK;
}

// nsIMsgSendLaterListener implementation 
NS_IMETHODIMP nsMsgOfflineManager::OnStartSending(PRUint32 aTotalMessageCount)
{
  return NS_OK;
}

NS_IMETHODIMP nsMsgOfflineManager::OnProgress(PRUint32 aCurrentMessage, PRUint32 aTotalMessage)
{
  if (m_statusFeedback && aTotalMessage)
    return m_statusFeedback->ShowProgress ((100 * aCurrentMessage) / aTotalMessage);
  else
    return NS_OK;
}

NS_IMETHODIMP nsMsgOfflineManager::OnStatus(const PRUnichar *aMsg)
{
  if (m_statusFeedback && aMsg)
    return m_statusFeedback->ShowStatusString (aMsg);
  else
    return NS_OK;
}

NS_IMETHODIMP nsMsgOfflineManager::OnStopSending(nsresult aStatus, const PRUnichar *aMsg, PRUint32 aTotalTried, 
                                 PRUint32 aSuccessful) 
{
#ifdef NS_DEBUG
  if (NS_SUCCEEDED(aStatus))
    printf("SendLaterListener::OnStopSending: Tried to send %d messages. %d successful.\n",
            aTotalTried, aSuccessful);
#endif
  return AdvanceToNextState();
}
