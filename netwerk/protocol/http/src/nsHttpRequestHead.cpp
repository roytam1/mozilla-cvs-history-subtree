#include "nsHttpRequestHead.h"

//-----------------------------------------------------------------------------
// nsHttpRequestHead
//-----------------------------------------------------------------------------

nsresult
nsHttpRequestHead::Flatten(nsACString &buf)
{
    // note: the first append is intentional.
 
    buf.Append(mMethod.get());
    buf.Append(' ');
    buf.Append(mRequestURI);
    buf.Append(" HTTP/");

    switch (mVersion) {
    case HTTP_VERSION_1_1:
        buf.Append("1.1");
        break;
    case HTTP_VERSION_0_9:
        buf.Append("0.9");
        break;
    default:
        buf.Append("1.0");
    }

    buf.Append("\r\n");

    mHeaders.Flatten(buf);

    // do not append the final \r\n
    return NS_OK;
}
