#include "nsHttpResponseHead.h"
#include "nsPrintfCString.h"
#include "nsReadableUtils.h"
#include "nsPromiseSubstring.h"

//-----------------------------------------------------------------------------
// nsHttpResponseHead
//-----------------------------------------------------------------------------

nsresult
nsHttpResponseHead::Flatten(nsACString &buf)
{
    if (mVersion == HTTP_VERSION_0_9)
        return NS_OK;

    buf.Append("HTTP/");
    if (mVersion == HTTP_VERSION_1_1)
        buf.Append("1.1 ");
    else
        buf.Append("1.0 ");

    buf.Append(nsPrintfCString("%d", mStatus));
    buf.Append(' ');
    buf.Append(mStatusText);
    buf.Append("\r\n");

    return mHeaders.Flatten(buf);
}

nsresult
nsHttpResponseHead::Parse(const nsReadingIterator<char> &begin,
                          const nsReadingIterator<char> &end)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsHttpResponseHead::ParseStatusLine(const nsReadingIterator<char> &begin,
                                    const nsReadingIterator<char> &end)
{
    //
    // Parse Status-Line:: HTTP-Version SP Status-Code SP Reason-Phrase CRLF
    //
    nsresult rv;
    nsReadingIterator<char> p = begin;
 
    // HTTP-Version
    rv = ParseVersion(p.get());
    if (NS_FAILED(rv)) return rv;
    
    if ((mVersion == HTTP_VERSION_0_9) || !FindCharInReadable(' ', p, end)) {
        mStatus = 200;
        mStatusText = NS_LITERAL_CSTRING("OK");
        goto end;
    }
    
    // Status-Code
    mStatus = atoi((++p).get());
    if (mStatus == 0) {
        LOG(("mal-formed response status; assuming status = 200\n"));
        mStatus = 200;
    }

    // Reason-Phrase is whatever is remaining of the line
    if (!FindCharInReadable(' ', p, end)) {
        LOG(("mal-formed response status line; assuming statusText = 'OK'\n"));
        mStatusText = NS_LITERAL_CSTRING("OK");
    }
    else
        mStatusText = Substring(++p, end);

end:
    LOG(("Have status line [version=%d status=%d statusText=%s]\n",
        mVersion, mStatus, mStatusText.get()));
    return NS_OK;
}

nsresult
nsHttpResponseHead::ParseHeaderLine(const nsReadingIterator<char> &begin,
                                    const nsReadingIterator<char> &end)
{
    nsReadingIterator<char> p = begin;

    if (FindCharInReadable(':', p, end)) {
        nsHttpAtom atom = nsHttp::ResolveAtom(Substring(begin, p));
        if (atom) {
            // skip over whitespace
            do {
                ++p;
            } while (*p == ' ');

            nsPromiseCSubstring val(p, end);

            // assign response header
            mHeaders.SetHeader(atom, val);

            // handle some special case headers...
            if (atom == nsHttp::Content_Length)
                mContentLength = atoi(PromiseFlatCString(val).get());
            else if (atom == nsHttp::Content_Type)
                ParseContentType(val);
        }
        else
            LOG(("unknown header; skipping\n"));
    }
    else
        LOG(("mal-formed header\n"));

    // We ignore mal-formed headers in the hope that we'll still be able
    // to do something useful with the response.
    return NS_OK;
}

nsresult
nsHttpResponseHead::ParseVersion(const char *str)
{
    // Parse HTTP-Version:: "HTTP" "/" 1*DIGIT "." 1*DIGIT

    // make sure we have HTTP at the beginning
    if (PL_strncmp(str, "HTTP", 4) != 0) {
        LOG(("looks like a HTTP/0.9 response\n"));
        mVersion = HTTP_VERSION_0_9;
        return NS_OK;
    }
    str += 4;

    if (*str != '/') {
        LOG(("server did not send a version number; assuming HTTP/1.0\n"));
        // NCSA/1.5.2 has a bug in which it fails to send a version number
        // if the request version is HTTP/1.1, so we fall back on HTTP/1.0
        mVersion = HTTP_VERSION_1_0;
        return NS_OK;
    }

    char *p = PL_strchr(str, '.');
    if (p == nsnull) {
        LOG(("mal-formed server version; assuming HTTP/1.0\n"));
        mVersion = HTTP_VERSION_1_0;
        return NS_OK;
    }

    ++p; // let b point to the minor version

    int major = atoi(str + 1);
    int minor = atoi(p);

    if ((major > 1) || ((major == 1) && (minor >= 1)))
        // at least HTTP/1.1
        mVersion = HTTP_VERSION_1_1;
    else
        // treat anything else as version 1.0
        mVersion = HTTP_VERSION_1_0;

    return NS_OK;
}

nsresult
nsHttpResponseHead::ParseContentType(const nsACString &type)
{
    nsReadingIterator<char> a, b;
    type.BeginReading(a);
    type.EndReading(b);

    if (FindCharInReadable('(', a, b)) {
        // we don't care about comments
        b = a;
        type.BeginReading(a);
        // trim any trailing whitespace
        while (*b == ' ') --b;
    }

    if (a != b) {
        nsReadingIterator<char> p = a;
        if (FindCharInReadable(';', p, b)) {
            // the content-type has additional fields
            mContentType = Substring(a, p);

            // is there a charset field?
            a = b;
            if (FindInReadable(NS_LITERAL_CSTRING("charset="), p, a)) {
                // set the charset
                mContentCharset = Substring(a, b);
            }
        }
        else
            mContentType = Substring(a, b);
    }

    return NS_OK;
}
