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
    nsHttpHeaderArray();
   ~nsHttpHeaderArray();

    const char *PeekHeader(nsHttpAtom header);

    nsresult SetHeader(nsHttpAtom header, const char *value);
    nsresult GetHeader(nsHttpAtom header, char **value);

    nsresult VisitHeaders(nsIHttpHeaderVisitor *visitor);

private:
    struct nsEntry
    {
        nsEntry(nsHttpAtom h, const char *v)
            : header(h) { value = v; }

        nsHttpAtom header;
        nsCString  value;
    };

    PRInt32 LookupEntry(nsHttpAtom header, nsEntry **);
    PRBool  CanAppendToHeader(nsHttpAtom header);

private:
    nsVoidArray mHeaders;
};

#endif
