#ifndef nsHttpHeaderArray_h__
#define nsHttpHeaderArray_h__

#include "nsVoidArray.h"
#include "nsIHttpChannel.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsHttp.h"

class nsHttpHeaderArray
{
public:
    nsHttpHeaderArray() {}
   ~nsHttpHeaderArray() {}

    const char *PeekHeader(nsHttpAtom header);

    nsresult SetHeader(nsHttpAtom header, const nsACString &value);
    nsresult GetHeader(nsHttpAtom header, nsACString &value);

    nsresult VisitHeaders(nsIHttpHeaderVisitor *visitor);

    nsresult Flatten(nsACString &);

private:
    struct nsEntry
    {
        nsEntry(nsHttpAtom h, const nsACString &v)
            : header(h) { value = v; }

        nsHttpAtom      header;
        nsCommonCString value;
    };

    PRInt32 LookupEntry(nsHttpAtom header, nsEntry **);
    PRBool  CanAppendToHeader(nsHttpAtom header);

private:
    nsVoidArray mHeaders;
};

#endif
