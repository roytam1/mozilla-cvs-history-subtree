#include "nsHttpChunkedDecoder.h"
#include "nsHttp.h"

//-----------------------------------------------------------------------------
// nsHttpChunkedDecoder <public>
//-----------------------------------------------------------------------------

nsresult
nsHttpChunkedDecoder::HandleChunkedContent(char *buf,
                                           PRUint32 count,
                                           PRUint32 *countRead)
{
    LOG(("nsHttpChunkedDecoder::HandleChunkedContent [count=%u]\n", count));

    *countRead = 0;

    while (count) {
        if (mChunkRemaining) {
            PRUint32 amt = PR_MIN(mChunkRemaining, count);

            count -= amt;
            mChunkRemaining -= amt;

            *countRead += amt;
            buf += amt;
        }

        if (count) {
            PRUint32 bytesConsumed = 0;

            nsresult rv = ParseChunkRemaining(buf, count, &bytesConsumed);
            if (NS_FAILED(rv)) return rv;

            count -= bytesConsumed;

            if (count) {
                // shift buf by bytesConsumed
                memmove(buf, buf + bytesConsumed, count);
            }
        }
    }

    return NS_OK;
}

//-----------------------------------------------------------------------------
// nsHttpChunkedDecoder <private>
//-----------------------------------------------------------------------------

nsresult
nsHttpChunkedDecoder::ParseChunkRemaining(char *buf,
                                          PRUint32 count,
                                          PRUint32 *countRead)
{
    NS_PRECONDITION(mChunkRemaining == 0, "chunk remaining should be zero");

    char *p;

    // we may have a partial line from the last time we were called
    if (mLineBuf.IsEmpty()) {
        p = PL_strstr(buf, "\r\n");
        if (p) {
            *p = 0;
            p = buf;
        }
    }
    else {
        p = PL_strstr(buf, "\r\n");
        // it would be really sad if this ever returned null, b/c it
        // would mean we were reading really small chunks from the net.
        if (p) {
            *p = 0;
            // append to the line buf and use that as the buf to parse.
            mLineBuf.Append(buf);
            p = (char *) mLineBuf.get();
        }
    }

    if (!p) {
        mLineBuf.Append(buf, count);
        *countRead = count;
        return NS_OK;
    }

    // indicate that the entire line has been consumed... the \r has been
    // nulled out, so we have to add 2 to include the \r\n in the count read.
    *countRead = PL_strlen(buf) + 2;

    buf = p;
    if ((p = PL_strchr(buf, ';')) != nsnull) {
        // ignore any chunk-extensions
        *p = 0;
    }

    if (!sscanf(buf, "%x", &mChunkRemaining)) {
        LOG(("sscanf failed parsing hex on string [%s]\n", buf));
        return NS_ERROR_UNEXPECTED;
    }

    if ((mChunkRemaining == 0) && (*buf != 0)) {
        // XXX need to add code to consume "trailer" headers
        mReachedEOF = PR_TRUE;
    }

    // clear the line buffer if it is not already empty
    if (!mLineBuf.IsEmpty())
        mLineBuf.SetLength(0);

    return NS_OK;
}
