#ifndef nsHttpRequestHead_h__
#define nsHttpRequestHead_h__

#include "nsHttpHeaderArray.h"
#include "nsHttp.h"
#include "nsXPIDLString.h"

//-----------------------------------------------------------------------------
// nsHttpRequestHead represents the request line and headers from an HTTP
// request.
//-----------------------------------------------------------------------------

class nsHttpRequestHead
{
public:
    nsHttpRequestHead() {}
   ~nsHttpRequestHead() {}

    void SetMethod(nsHttpAtom method) { mMethod = method; }
    void SetVersion(nsHttpVersion version) { mVersion = version; }
    void SetRequestURI(const char *s) { mRequestURI = s; }

    nsHttpHeaderArray &Headers()    { return mHeaders; }
    nsHttpAtom         Method()     { return mMethod; }
    nsHttpVersion      Version()    { return mVersion; }
    const char        *RequestURI() { return mRequestURI; }

    const char *PeekHeader(nsHttpAtom h)            { return mHeaders.PeekHeader(h); }
    nsresult SetHeader(nsHttpAtom h, const char *v) { return mHeaders.SetHeader(h, v); }
    nsresult GetHeader(nsHttpAtom h, char **v)      { return mHeaders.GetHeader(h, v); }

    nsresult Flatten(nsACString &);

private:
    nsHttpHeaderArray mHeaders;
    nsHttpAtom        mMethod;
    nsHttpVersion     mVersion;
    nsXPIDLCString    mRequestURI;
};

#endif // nsHttpRequestHead_h__
