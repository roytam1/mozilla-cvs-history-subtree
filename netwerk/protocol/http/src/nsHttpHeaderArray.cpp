#include "nsHttpHeaderArray.h"
#include "nsHttp.h"

//-----------------------------------------------------------------------------
// nsHttpHeaderArray
//-----------------------------------------------------------------------------

nsresult
nsHttpHeaderArray::SetHeader(nsHttpAtom header, const char *value)
{
    nsEntry *entry = nsnull;
    PRInt32 index;

    // If a NULL value is passed in, then delete the header entry...
    index = LookupEntry(header, &entry);
    if (!value) {
        if (entry) {
            mHeaders.RemoveElementAt(index);
            delete entry;
        }
        return NS_OK;
    }

    // Create a new entry or...
    if (!entry) {
        entry = new nsEntry(header, value);
        if (!entry)
            return NS_ERROR_OUT_OF_MEMORY;
        if (!mHeaders.AppendElement(entry)) {
            NS_WARNING("AppendElement failed");
            delete entry;
        }
    }
    // Append the new value to the existing value iff...
    else if (CanAppendToHeader(header)) {
        if (header == nsHttp::Set_Cookie ||
            header == nsHttp::WWW_Authenticate ||
            header == nsHttp::Proxy_Authenticate)
            // Special case these headers and use a newline delimiter to
            // delimit the values from one another b/c commas may appear
            // in the values of these headers contrary to what the spec says.
            entry->value.Append('\n');
        else
            // Delimit each value from the others using a comma (per HTTP spec)
            entry->value.Append(", ");
        entry->value.Append(value);
    }
    // Replace the existing string with the new value
    else
        entry->value = value;
    return NS_OK;
}

const char *
nsHttpHeaderArray::PeekHeader(nsHttpAtom header)
{
    nsEntry *entry = nsnull;
    LookupEntry(header, &entry);
    return entry ? entry->value.get() : nsnull;
}

nsresult
nsHttpHeaderArray::GetHeader(nsHttpAtom header, char **result)
{
    nsEntry *entry = nsnull;

    LookupEntry(header, &entry);
    if (!entry) return NS_ERROR_NOT_AVAILABLE;

    return DupString(entry->value.get(), result);
}

nsresult
nsHttpHeaderArray::VisitHeaders(nsIHttpHeaderVisitor *visitor)
{
    NS_ENSURE_ARG_POINTER(visitor);
    PRInt32 i, count = mHeaders.Count();
    for (i=0; i<count; ++i) {
        nsEntry *entry = (nsEntry *) mHeaders[i];
        if (NS_FAILED(visitor->VisitHeader(entry->header, entry->value.get())))
            break;
    }
    return NS_OK;
}

nsresult
nsHttpHeaderArray::Flatten(nsACString &buf)
{
    PRInt32 i, count = mHeaders.Count();
    for (i=0; i<count; ++i) {
        nsEntry *entry = (nsEntry *) mHeaders[i];
        buf.Append(entry->header);
        buf.Append(": ");
        buf.Append(entry->value);
        buf.Append("\r\n");
    }
    return NS_OK;
}

PRInt32
nsHttpHeaderArray::LookupEntry(nsHttpAtom header, nsEntry **entry)
{
    PRInt32 i, count = mHeaders.Count();
    for (i=0; i<count; ++i) {
        *entry = (nsEntry *) mHeaders[i];
        if ((*entry)->header == header)
            return i;
    }
    *entry = nsnull;
    return -1;
}

PRBool
nsHttpHeaderArray::CanAppendToHeader(nsHttpAtom header)
{
    return header == nsHttp::Accept_Charset ||
           header == nsHttp::User_Agent ||
           header == nsHttp::Referer ||
           header == nsHttp::Host ||
           header == nsHttp::Authorization ||
           header == nsHttp::If_Modified_Since ||
           header == nsHttp::If_Unmodified_Since ||
           header == nsHttp::From ||
           header == nsHttp::Location ||
           header == nsHttp::Max_Forwards
           ?
           PR_FALSE : PR_TRUE;
}
