#ifndef nsHttpChunkedDecoder_h__
#define nsHttpChunkedDecoder_h__

#include "nsError.h"
#include "nsString.h"

class nsHttpChunkedDecoder
{
public:
    nsHttpChunkedDecoder() : mChunkRemaining(0), mReachedEOF(0) {}
   ~nsHttpChunkedDecoder() {}

    PRBool ReachedEOF() { return mReachedEOF; }

    // Called by the transaction to handle chunked content.
    nsresult HandleChunkedContent(char *buf,
                                  PRUint32 count,
                                  PRUint32 *countRead);
private:
    nsresult ParseChunkRemaining(char *buf,
                                 PRUint32 count,
                                 PRUint32 *countRead);

private:
    PRUint32     mChunkRemaining;
    nsCString    mLineBuf; // may hold a partial line
    PRPackedBool mReachedEOF;
};

#endif
