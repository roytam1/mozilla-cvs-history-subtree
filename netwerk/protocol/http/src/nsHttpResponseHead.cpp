#include "nsHttpResponseHead.h"
#include "nsPrintfCString.h"

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
nsHttpResponseHead::Parse(char *block)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsHttpResponseHead::ParseStatusLine(char *line)
{
    //
    // Parse Status-Line:: HTTP-Version SP Status-Code SP Reason-Phrase CRLF
    //
    nsresult rv;
 
    // HTTP-Version
    rv = ParseVersion(line);
    if (NS_FAILED(rv)) return rv;
    
    if ((mVersion == HTTP_VERSION_0_9) || !(line = PL_strchr(line, ' '))) {
        mStatus = 200;
        mStatusText = "OK";
        goto end;
    }
    
    // Status-Code
    mStatus = atoi(++line);
    if (mStatus == 0) {
        LOG(("mal-formed response status; assuming status = 200\n"));
        mStatus = 200;
    }

    // Reason-Phrase is whatever is remaining of the line
    if (!(line = PL_strchr(line, ' '))) {
        LOG(("mal-formed response status line; assuming statusText = 'OK'\n"));
        mStatusText = "OK";
    }
    else
        mStatusText = ++line;

end:
    LOG(("Have status line [version=%d status=%d statusText=%s]\n",
        mVersion, mStatus, mStatusText.get()));
    return NS_OK;
}

nsresult
nsHttpResponseHead::ParseHeaderLine(char *line)
{
    char *p = PL_strchr(line, ':');

    if (p) {
        *p = 0; // overwrite ':' char
        nsHttpAtom atom = nsHttp::ResolveAtom(line);
        if (atom) {
            // skip over whitespace
            do {
                ++p;
            } while (*p == ' ');

            // assign response header
            mHeaders.SetHeader(atom, p);

            // handle some special case headers...
            if (atom == nsHttp::Content_Length)
                mContentLength = atoi(p);
            else if (atom == nsHttp::Content_Type)
                ParseContentType(p);
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
nsHttpResponseHead::ParseContentType(char *type)
{
    char *p = PL_strchr(type, '(');

    if (p) {
        // we don't care about comments
        *p = 0;
        // trim any trailing whitespace
        do {
            --p;
        } while ((*p == ' ') && (p > type));
    }

    if (p != type) {
        // check if the content-type has additional fields...
        if ((p = PL_strchr(type, ';')) != nsnull) {
            // null out the ';' char
            *p = 0;
            // is there a charset field?
            if ((p = PL_strstr(p, "charset=")) != nsnull)
                mContentCharset = p + 8;
        }
        mContentType = type;
    }

    return NS_OK;
}
