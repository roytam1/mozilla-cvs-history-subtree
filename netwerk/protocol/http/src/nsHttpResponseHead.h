#ifndef nsHttpResponseHead_h__
#define nsHttpResponseHead_h__

#include "nsHttpHeaderArray.h"
#include "nsHttp.h"
#include "nsXPIDLString.h"

//-----------------------------------------------------------------------------
// nsHttpResponseHead represents the status line and headers from an HTTP
// response.
//-----------------------------------------------------------------------------

class nsHttpResponseHead
{
public:
    nsHttpResponseHead() : mStatus(200), mContentLength(-1) {}
   ~nsHttpResponseHead() {}
    
    nsHttpHeaderArray &Headers()        { return mHeaders; }
    nsHttpVersion      Version()        { return mVersion; }
    PRUint32           Status()         { return mStatus; }
    const char        *StatusText()     { return mStatusText; }
    PRInt32            ContentLength()  { return mContentLength; }
    const char        *ContentType()    { return mContentType; }
    const char        *ContentCharset() { return mContentCharset; }

    const char *PeekHeader(nsHttpAtom h)            { return mHeaders.PeekHeader(h); }
    nsresult SetHeader(nsHttpAtom h, const char *v) { return mHeaders.SetHeader(h, v); }
    nsresult GetHeader(nsHttpAtom h, char **v)      { return mHeaders.GetHeader(h, v); }

    nsresult Flatten(nsACString &);

    // parse flattened response head. block must be null terminated. parsing is
    // destructive.
    nsresult Parse(char *block);

    // parse the status line. line must be null terminated.
    nsresult ParseStatusLine(char *line);

    // parse a header line. line must be null terminated. parsing is destructive.
    nsresult ParseHeaderLine(char *line);

private:
    nsresult ParseVersion(const char *);
    nsresult ParseContentType(char *);

private:
    nsHttpHeaderArray mHeaders;
    nsHttpVersion     mVersion;
    PRUint32          mStatus;
    nsXPIDLCString    mStatusText;
    PRInt32           mContentLength;
    nsXPIDLCString    mContentType;
    nsXPIDLCString    mContentCharset;
};

#endif // nsHttpResponseHead_h__
