#ifndef nsHttpResponseHead_h__
#define nsHttpResponseHead_h__

#include "nsHttpHeaderArray.h"
#include "nsHttp.h"
#include "nsCommonString.h"

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
    nsCommonCString    StatusText()     { return mStatusText; }
    PRInt32            ContentLength()  { return mContentLength; }
    nsCommonCString    ContentType()    { return mContentType; }
    nsCommonCString    ContentCharset() { return mContentCharset; }

    const char *PeekHeader(nsHttpAtom h)            { return mHeaders.PeekHeader(h); }
    nsresult SetHeader(nsHttpAtom h, const char *v) { return mHeaders.SetHeader(h, v); }
    nsresult GetHeader(nsHttpAtom h, char **v)      { return mHeaders.GetHeader(h, v); }

    nsresult Flatten(nsACString &);

    // called to parse from the result of Flatten
    nsresult Parse(const nsReadingIterator<char> &begin,
                   const nsReadingIterator<char> &end);

    // called to parse the status line
    nsresult ParseStatusLine(const nsReadingIterator<char> &begin,
                             const nsReadingIterator<char> &end);

    // called to parse a header line
    nsresult ParseHeaderLine(const nsReadingIterator<char> &begin,
                             const nsReadingIterator<char> &end);

private:
    nsresult ParseVersion(const char *str);
    nsresult ParseContentType(const nsACString &);

private:
    nsHttpHeaderArray mHeaders;
    nsHttpVersion     mVersion;
    PRUint32          mStatus;
    nsCommonCString   mStatusText;
    PRInt32           mContentLength;
    nsCommonCString   mContentType;
    nsCommonCString   mContentCharset;
};

#endif // nsHttpResponseHead_h__
