// License Block

#ifndef BROWSER_DIR_PROVIDER_H
#define BROWSER_DIR_PROVIDER_H

#include "nsCOMPtr.h"
#include <nsIDirectoryService.h>
#include <nsILocalFile.h>
#include <nsIFile.h>

// forward declarations
class BrowserGlue;

class BrowserDirProvider : public nsIDirectoryServiceProvider
{

public:
  BrowserDirProvider();
  virtual ~BrowserDirProvider();

  nsresult Init( BrowserGlue *aOwner );

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDIRECTORYSERVICEPROVIDER

protected:

  nsresult GetProductDirectory( nsILocalFile **aFile );
  // handle back to our owner
  BrowserGlue *mOwner;

  // XPCOM handles
  nsCOMPtr<nsIFile>  mProfileDir;

};

#endif /* BROWSER_DIR_PROVIDER_H */

// EOF

