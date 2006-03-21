// License Block

#include "BrowserDirProvider.h"

#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceDefs.h"
#include "nsILocalFile.h"
#include "prenv.h"
#include "nsStringAPI.h"
#include "nsString.h"
#include "nsXPCOM.h"


// pulled from nsXULAppAPI.h
#define NS_APP_PROFILE_DIR_STARTUP "ProfDS"
#define NS_APP_PROFILE_LOCAL_DIR_STARTUP "ProfLDS"

BrowserDirProvider::BrowserDirProvider() : mOwner(nsnull)
{
  printf("BrowserDirProvider::BrowserDirProvider()\n");
  NS_INIT_ISUPPORTS();
}

BrowserDirProvider::~BrowserDirProvider()
{
  printf("BrowserDirProvider::~BrowserDirProvider()\n");
  mOwner = nsnull;
}

NS_IMPL_ISUPPORTS1( BrowserDirProvider,
                    nsIDirectoryServiceProvider )

nsresult
BrowserDirProvider::Init( BrowserGlue *aOwner )
{
  printf("BrowserDirProvider::Init()\n");
  NS_ENSURE_ARG_POINTER(aOwner);

  mOwner = aOwner;

  return NS_OK;
}

NS_IMETHODIMP
BrowserDirProvider::GetFile( const char *aProperty,
                             PRBool *aPersistent,
                             nsIFile **aFile )
{
  printf("BrowserDirProvider::GetFile(%s) \n", aProperty);
  nsresult rv;
  nsCOMPtr<nsILocalFile> localFile;

  *aPersistent = PR_TRUE;

  if (!strcmp(aProperty, NS_APP_USER_PROFILE_50_DIR)) {
    printf("BrowserDirProvider::GetFile() - App User Prof 50 dir \n");
    rv = GetProductDirectory(getter_AddRefs(localFile));
  }
  else if (!strcmp(aProperty, NS_APP_USER_PROFILE_LOCAL_50_DIR)) {
    printf("BrowserDirProvider::GetFile() - App User Prof Local 50 dir \n");
    rv = GetProductDirectory(getter_AddRefs(localFile));
  }
  else if (!strcmp(aProperty, NS_APP_PROFILE_DIR_STARTUP)) {
    printf("BrowserDirProvider::GetFile() - App Prof Dir Startup \n");
    rv = GetProductDirectory(getter_AddRefs(localFile));
  }
  else if (!strcmp(aProperty, NS_APP_PROFILE_LOCAL_DIR_STARTUP)) {
    printf("BrowserDirProvider::GetFile() - App Prof Local Dir Startup \n");
    rv = GetProductDirectory(getter_AddRefs(localFile));
  }

  if (localFile) {
    printf("BrowserDirProvider::GetFile() - Have localFile \n");
    return CallQueryInterface(localFile, aFile);
  }

  printf("BrowserDirProvider::GetFile() - don't have localFile \n");
  return NS_ERROR_FAILURE;
}

// hand back the directory where OpenPFC is located
nsresult
BrowserDirProvider::GetProductDirectory( nsILocalFile **aFile )
{
  printf("BrowserDirProvider::GetProductDirectory() \n");
  nsresult rv;
  nsCOMPtr<nsILocalFile> localDir;

  // create a new file handle (don't have to clone)
  rv = NS_NewNativeLocalFile(nsDependentCString(PR_GetEnv("OPENPFC_BIN_HOME")),
                             PR_TRUE,
                             getter_AddRefs(localDir));
  NS_ENSURE_SUCCESS(rv, rv);

  // location within the OpenPFC directory where the profile, components
  //    and other mozilla related information will reside
  rv = localDir->AppendRelativeNativePath(NS_LITERAL_CSTRING("XRE"));
  NS_ENSURE_SUCCESS(rv, rv);

  *aFile = localDir;
  NS_IF_ADDREF(*aFile);
  return NS_OK;
}


