/* vim:set ts=2 sw=2 sts=2 et cin: */
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
 * The Original Code is Mozilla.
 *
 * The Initial Developer of the Original Code is IBM Corporation.
 * Portions created by IBM Corporation are Copyright (C) 2004
 * IBM Corporation. All Rights Reserved.
 *
 * Contributor(s):
 *   Darin Fisher <darin@meer.net>
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

#include <windows.h>
#include <shlobj.h>
#include <string.h>
#include <wchar.h>

//-----------------------------------------------------------------------------

#ifdef DEBUG

#include <stdio.h>
#include <stdarg.h>

static void LogPrintf(const char *fmt, ... )
{
  va_list ap;
  FILE *fp = fopen("C:\\mozicoh.log", "a");

  va_start(ap, fmt);

  fprintf(fp, "%u:", GetCurrentProcessId());
  vfprintf(fp, fmt, ap);

  va_end(ap);

  fclose(fp);
}

#define LOG(args) LogPrintf args
#else
#define LOG(args)
#endif

//-----------------------------------------------------------------------------

// {c33a45e5-54ef-4873-a8b1-4543009393ca}
static const GUID CLSID_nsIconHandler =
  { 0xc33a45e5, 0x54ef, 0x4873, { 0xa8, 0xb1, 0x45, 0x43, 0x00, 0x93, 0x93, 0xca } };

//-----------------------------------------------------------------------------

class nsIconHandler : public IPersistFile, public IExtractIconA, public IExtractIconW
{
public:
  nsIconHandler()
    : mRefCnt(0)
    , mFileName(NULL)
  {}

  ~nsIconHandler()
  {
    free(mFileName);
  }
  
  // IUnknown
  STDMETHODIMP QueryInterface(REFIID iid, void **ppv);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  // IPersist
  STDMETHODIMP GetClassID(CLSID *pClassID);

  // IPersistFile
  STDMETHODIMP IsDirty();
  STDMETHODIMP Load(LPCOLESTR pszFileName, DWORD dwMode);
  STDMETHODIMP Save(LPCOLESTR pszFileName, BOOL fRemember);
  STDMETHODIMP SaveCompleted(LPCOLESTR pszFileName);
  STDMETHODIMP GetCurFile(LPOLESTR *pszFileName);

  // IExtractIconA
  STDMETHODIMP GetIconLocation(UINT uFlags, LPSTR szIconFile, UINT cchMax, int *piIndex, UINT *pwFlags);
  STDMETHODIMP Extract(LPCSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);

  // IExtractIconW
  STDMETHODIMP GetIconLocation(UINT uFlags, LPWSTR szIconFile, UINT cchMax, int *piIndex, UINT *pwFlags);
  STDMETHODIMP Extract(LPCWSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);

private:
  ULONG  mRefCnt;
  WCHAR *mFileName;
};

HRESULT
nsIconHandler::QueryInterface(REFIID iid, void **ppv)
{
  if (iid == IID_IPersist)
    *ppv = (IPersist *) this;    
  else if (iid == IID_IPersistFile)
    *ppv = (IPersistFile *) this;
  else if (iid == IID_IExtractIconA)
    *ppv = (IExtractIconA *) this;
  else if (iid == IID_IExtractIconW)
    *ppv = (IExtractIconW *) this;
  else if (iid == IID_IUnknown)
    *ppv = (IUnknown *) (IPersistFile *) this;
  else
  {
    *ppv = NULL;
    return E_NOINTERFACE;
  }

  AddRef();
  return S_OK;
}

ULONG
nsIconHandler::AddRef()
{
  return ++mRefCnt;
}

ULONG
nsIconHandler::Release()
{
  ULONG count = --mRefCnt;
  if (count == 0)
    delete this;
  return count;
}

// From MSDN:
//
// The Shell calls GetClassID first, and the function returns the class identifier
// (CLSID) of the extension handler object. The Shell then calls Load and passes
// in two values. The first, pszFileName, is a Unicode string with the name of the
// file or folder that Shell is about to operate on. The second is dwMode, which
// indicates the file access mode. Because there is normally no need to access
// files, dwMode is typically zero. The method stores these values as needed for
// later reference.

HRESULT
nsIconHandler::GetClassID(CLSID *pClassID)
{
  LOG(("GetClassID\n"));

  *pClassID = CLSID_nsIconHandler;
  return S_OK;
}

HRESULT
nsIconHandler::IsDirty()
{
  LOG(("IsDirty\n"));
  return E_NOTIMPL;
}

HRESULT
nsIconHandler::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
  LOG(("Load\n"));

  WCHAR *name = _wcsdup(pszFileName);
  if (!name)
    return E_OUTOFMEMORY;

  if (mFileName)
    free(mFileName);
  mFileName = name;

  return S_OK;
}

HRESULT
nsIconHandler::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
  LOG(("Save\n"));
  return E_NOTIMPL;
}

HRESULT
nsIconHandler::SaveCompleted(LPCOLESTR pszFileName)
{
  LOG(("SaveCompleted\n"));
  return E_NOTIMPL;
}

HRESULT
nsIconHandler::GetCurFile(LPOLESTR *pszFileName)
{
  LOG(("GetCurFile\n"));
  return E_NOTIMPL;
}

// IExtractIconA : used by Win9x derivatives

HRESULT
nsIconHandler::GetIconLocation(UINT uFlags, LPSTR szIconFile, UINT cchMax, int *piIndex, UINT *pwFlags)
{
  LOG(("IExtractIconA::GetIconLocation\n"));

  *piIndex = 0;
  *pwFlags = 0;

  // convert mFileName to ANSI codepage
  char fname[MAX_PATH];
  DWORD fnameLen = WideCharToMultiByte(CP_ACP, 0, mFileName, -1,
                                       fname, sizeof(fname), NULL, NULL);
  if (fnameLen == 0)
    return E_FAIL;
  fname[fnameLen] = '\0';

  char *fnameTail = strrchr(fname, '\\');
  if (!fnameTail)
    return E_FAIL;
  fnameTail++;
  DWORD baseLen = fnameTail - fname;

  char suffix[MAX_PATH];
  DWORD suffixLen = GetPrivateProfileStringA("Shell", "Icon", "",
                                             suffix, sizeof(suffix), fname);

  // make sure we have enough room in the destination buffer
  if (cchMax < baseLen + suffixLen + 1)
    return E_FAIL;

  memcpy(szIconFile, fname, baseLen);
  memcpy(szIconFile + baseLen, suffix, suffixLen + 1);
  return S_OK;
}

HRESULT
nsIconHandler::Extract(LPCSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
  LOG(("IExtractIconA::Extract\n"));
  // let the shell extract the icon from pszFile itself
  *phiconLarge = NULL;
  *phiconSmall = NULL;
  return S_FALSE;
}

// IExtractIconW : used by WinNT derivatives

HRESULT
nsIconHandler::GetIconLocation(UINT uFlags, LPWSTR szIconFile, UINT cchMax, int *piIndex, UINT *pwFlags)
{
  LOG(("IExtractIconW::GetIconLocation\n"));

  *piIndex = 0;
  *pwFlags = 0;

  const WCHAR *fnameTail = wcsrchr(mFileName, '\\');
  if (!fnameTail)
    return E_FAIL;
  fnameTail++;
  DWORD baseLen = fnameTail - mFileName;

  WCHAR suffix[MAX_PATH];
  DWORD suffixLen = GetPrivateProfileStringW(L"Shell", L"Icon", L"",
                                             suffix, sizeof(suffix), mFileName);

  // make sure we have enough room in the destination buffer
  if (cchMax < baseLen + suffixLen + 1)
    return E_FAIL;

  memcpy(szIconFile, mFileName, baseLen * sizeof(WCHAR));
  memcpy(szIconFile + baseLen, suffix, (suffixLen + 1) * sizeof(WCHAR));
  return S_OK;
}

HRESULT
nsIconHandler::Extract(LPCWSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
  LOG(("IExtractIconW::Extract\n"));
  // let the shell extract the icon from pszFile itself
  *phiconLarge = NULL;
  *phiconSmall = NULL;
  return S_FALSE;
}

//-----------------------------------------------------------------------------

class nsIconHandlerFactory : public IClassFactory
{
public:
  nsIconHandlerFactory() : mRefCnt(0) {}

  // IUnknown
  STDMETHODIMP QueryInterface(REFIID iid, void **ppv);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  // IClassFactory
  STDMETHODIMP CreateInstance(IUnknown *outer, REFIID riid, void **ppv);
  STDMETHODIMP LockServer(BOOL fLock);

private:
  ULONG mRefCnt; 
};

HRESULT
nsIconHandlerFactory::QueryInterface(REFIID iid, void **ppv)
{
  if (iid != IID_IClassFactory && iid != IID_IUnknown)
  {
    *ppv = NULL;
    return E_NOINTERFACE;
  }

  *ppv = this;
  AddRef();
  return S_OK;
}

ULONG
nsIconHandlerFactory::AddRef()
{
  return ++mRefCnt;
}

ULONG
nsIconHandlerFactory::Release()
{
  ULONG count = --mRefCnt;
  if (count == 0)
    delete this;
  return count;
}

HRESULT
nsIconHandlerFactory::CreateInstance(IUnknown *outer, REFIID riid, void **ppv)
{
  LOG(("CreateInstance\n"));

  *ppv = NULL;

  if (outer)
    return CLASS_E_NOAGGREGATION;

  nsIconHandler *handler = new nsIconHandler();
  if (!handler)
    return E_OUTOFMEMORY;

  handler->AddRef();
  HRESULT hr = handler->QueryInterface(riid, ppv);
  handler->Release();

  return hr;
}

HRESULT
nsIconHandlerFactory::LockServer(BOOL fLock)
{
  LOG(("LockServer [fLock=%d]\n", fLock));

  // XXX do something here?
  return S_OK;
}

//-----------------------------------------------------------------------------

static char gModuleFilePath[MAX_PATH];

BOOL WINAPI DllMain(
  HINSTANCE hinstDLL,
  DWORD     fdwReason,
  LPVOID    lpvReserved)
{
  LOG(("DllMain: fdwReason=%u\n", fdwReason));

  if (fdwReason == DLL_PROCESS_ATTACH)
  {
    // get our module path
    GetModuleFileName(hinstDLL, gModuleFilePath, sizeof(gModuleFilePath));
  }

  return TRUE;
}

STDMETHODIMP DllGetClassObject(
  REFCLSID rclsid,
  REFIID riid,
  LPVOID *ppv)
{
  LOG(("DllGetClassObject\n"));

  *ppv = NULL;

  if (rclsid != CLSID_nsIconHandler)
    return CLASS_E_CLASSNOTAVAILABLE;

  nsIconHandlerFactory *factory = new nsIconHandlerFactory(); 
  if (!factory)
  {
    *ppv = NULL;
    return E_OUTOFMEMORY;
  }
    
  factory->AddRef();
  HRESULT hr = factory->QueryInterface(riid, ppv); 
  factory->Release(); 
  return hr;
}

STDMETHODIMP DllCanUnloadNow()
{
  return S_FALSE;
}

// XXX this is better done using an installer JST file
#if 0
STDMETHODIMP DllRegisterServer()
{
  LOG(("DllRegisterServer\n"));

  //
  // HKEY_CLASSES_ROOT
  //   .moz
  //     (Default)              MOZ.XULApp.1
  //   MOZ.XULApp.1
  //     (Default)              Mozilla XUL Application (XXX Localize)
  //     DefaultIcon
  //       (Default)            %1
  //     ShellEx
  //       IconHandler
  //         (Default)          {clsid}
  //   CLSID
  //     {clsid}
  //       (Default)            MOZ.IconHandler.1
  //       InprocServer32
  //         (Default)          C:\path\to\component.dll
  //         ThreadingModel     Apartment

  LONG rv;
  HKEY hKey;
  
  rv = RegCreateKeyEx(HKEY_CLASSES_ROOT,
                      "CLSID\\{c33a45e5-54ef-4873-a8b1-4543009393ca}",
                      0,
                      NULL,
                      REG_OPTION_NON_VOLATILE,
                      KEY_ALL_ACCESS,
                      NULL,
                      &hKey,
                      NULL);
  if (rv != ERROR_SUCCESS)
    return E_FAIL;

  const unsigned char kProgID[] = "MOZ.IconHandler.1";

  RegSetValueEx(hKey, NULL, 0, REG_SZ, kProgID, sizeof(kProgID));

  HKEY hProcKey;
  rv = RegCreateKeyEx(hKey,
                      "InprocServer32",
                      0,
                      NULL,
                      REG_OPTION_NON_VOLATILE,
                      KEY_ALL_ACCESS,
                      NULL,
                      &hProcKey,
                      NULL);
  RegCloseKey(hKey);
  if (rv != ERROR_SUCCESS)
    return E_FAIL;

  RegSetValueEx(hProcKey, NULL, 0, REG_SZ, (const unsigned char *) gModuleFilePath, strlen(gModuleFilePath));

  const unsigned char kApartment[] = "Apartment";
  RegSetValueEx(hProcKey, "ThreadingModel", 0, REG_SZ, kApartment, sizeof(kApartment));

  RegCloseKey(hProcKey);
  return S_OK;
}

STDMETHODIMP DllUnregisterServer()
{
  return S_OK;
}
#endif
