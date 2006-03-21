// License Block

#ifndef BROWSER_PROGRESS_H
#define BROWSER_PROGRESS_H

// mozilla headers
#include "nsIWebProgressListener.h"
#include "nsWeakReference.h"

// forward decl
class BrowserGlue;

// class to handle the progress notifications
class BrowserProgress : public nsIWebProgressListener,
                        public nsSupportsWeakReference
{
  public:

    BrowserProgress();
    virtual ~BrowserProgress();

    nsresult Init( BrowserGlue *aOwner );

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBPROGRESSLISTENER

  private:

    BrowserGlue *mOwner;
    PRUint32 mLoadCount;

};

#endif /* BROWSER_PROGRESS_H */

