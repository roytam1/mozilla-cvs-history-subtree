#include "nsIFileStream.h"

#include "nsFileStream.h"

static NS_DEFINE_IID(kIFileInputStreamIID,       NS_IFILEINPUTSTREAM_IID);
static NS_DEFINE_IID(kIFileOutputStreamIID,      NS_IFILEOUTPUTSTREAM_IID);

//========================================================================================
class FileOutputStreamImpl
    : public nsIFileOutputStream
    , public nsOutputFileStream
//========================================================================================
{
    public:
        FileOutputStreamImpl()
        {
            NS_INIT_REFCNT();
        }
        FileOutputStreamImpl(
            const nsFileSpec& inFile,
            PRInt32 nsprMode /*default = (PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE)*/,
            PRInt32 accessMode /*Default = 0700 (octal)*/)
            : nsOutputFileStream(inFile, nsprMode, accessMode)
        {
            NS_INIT_REFCNT();
        }

        virtual ~FileOutputStreamImpl()
        {
        }

        // nsISupports interface
        NS_DECL_ISUPPORTS

        // nsIBaseStream interface
        NS_IMETHOD Close()
        {
            close();
            return NS_OK;
        }

        // nsIOutputStream interface
        NS_IMETHOD Write(const char* aBuf,
                     PRUint32 aOffset,
                     PRUint32 aCount,
                     PRUint32 *aWriteCount)
        {
            NS_PRECONDITION(aBuf != nsnull, "null ptr");
            NS_PRECONDITION(aWriteCount != nsnull, "null ptr");
            seek(aOffset);
            PRInt32 written = write(aBuf + aOffset, aCount);
            if (written == -1) {
                *aWriteCount = 0;
                return NS_ERROR_FAILURE; // XXX right error code?
            }
            else {
                *aWriteCount = written;
                return NS_OK;
            }
        }

    private:
        PRInt32                                   mLength;
}; // FileOutputStreamImpl

//========================================================================================
class FileInputStreamImpl
    : public nsIFileInputStream
    , public nsInputFileStream
//========================================================================================
{
    public:
        FileInputStreamImpl()
            : mLength(-1)
        {
            NS_INIT_REFCNT();
        }
        FileInputStreamImpl(
            const nsFileSpec& inFile,
            PRInt32 nsprMode /*Default = PR_RDONLY*/,
            PRInt32 accessMode /*Default = 0700 (octal)*/)
            : nsInputFileStream(inFile, nsprMode,accessMode)
            , mLength(PR_Available(GetFileDescriptor()))
        {
            NS_INIT_REFCNT();
        }

        virtual ~FileInputStreamImpl()
        {
        }

        // nsISupports interface
        NS_DECL_ISUPPORTS

        // nsIBaseStream interface
        NS_IMETHOD Close()
        {
           close();
            return NS_OK;
        }

        // nsIInputStream interface
        NS_IMETHOD GetLength(PRUint32 *aLength)
        {
            NS_PRECONDITION(aLength != nsnull, "null ptr");
            if (!aLength)
                return NS_ERROR_NULL_POINTER;
            if (mLength < 0)
                return NS_FILE_RESULT(NS_ERROR_UNEXPECTED);
            *aLength = mLength;
            return NS_OK;
        }
        
        NS_IMETHOD Read(char* aBuf,
                     PRUint32 aOffset,
                     PRUint32 aCount,
                     PRUint32 *aReadCount)
        {
            NS_PRECONDITION(aBuf != nsnull, "null ptr");
            if (!aBuf)
                return NS_ERROR_NULL_POINTER;
            NS_PRECONDITION(aReadCount != nsnull, "null ptr");
            if (!aReadCount)
                return NS_ERROR_NULL_POINTER;
            seek(aOffset);
            PRInt32 bytesRead = read(aBuf, aCount);
            if (bytesRead == -1)
            {
                *aReadCount = 0;
                return NS_ERROR_FAILURE; // XXX right error code?
            }
            else
            {
                *aReadCount = bytesRead;
                return NS_OK;
            }
        }

    private:
        PRInt32                                  mLength;
}; // FileInputStreamImpl

NS_IMPL_ISUPPORTS(FileOutputStreamImpl, kIFileOutputStreamIID);
NS_IMPL_ISUPPORTS(FileInputStreamImpl, kIFileInputStreamIID);

//----------------------------------------------------------------------------------------
NS_BASE nsresult NS_NewInputConsoleStream(
    nsIFileInputStream** aInstancePtrResult)
// Factory method to get an nsInputStream from the console.
//----------------------------------------------------------------------------------------
{
    NS_PRECONDITION(aInstancePtrResult != nsnull, "null ptr");
    if (! aInstancePtrResult)
        return NS_ERROR_NULL_POINTER;

    FileInputStreamImpl* stream = new FileInputStreamImpl;
    if (! stream)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(stream);
    *aInstancePtrResult = stream;
    return NS_OK;
}

//----------------------------------------------------------------------------------------
NS_BASE nsresult NS_NewInputFileStream(
    nsIFileInputStream** aInstancePtrResult,
    const nsFileSpec& inFile,
    PRInt32 nsprMode /*Default = PR_RDONLY*/,
    PRInt32 accessMode /*Default = 0700 (octal)*/)
// Factory method to get an nsInputStream from a file
// (see nsFileStream.h for more info on the parameters).
//----------------------------------------------------------------------------------------
{
    NS_PRECONDITION(aInstancePtrResult != nsnull, "null ptr");
    if (! aInstancePtrResult)
        return NS_ERROR_NULL_POINTER;

    FileInputStreamImpl* stream = new FileInputStreamImpl(inFile, nsprMode, accessMode);
    if (! stream)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(stream);
    *aInstancePtrResult = stream;
    return NS_OK;
}

//----------------------------------------------------------------------------------------
NS_BASE nsresult NS_NewOutputConsoleStream(
    nsIFileOutputStream** aInstancePtrResult)
// Factory method to get an nsOutputStream to the console.
//----------------------------------------------------------------------------------------
{
    NS_PRECONDITION(aInstancePtrResult != nsnull, "null ptr");
    if (! aInstancePtrResult)
        return NS_ERROR_NULL_POINTER;

    FileOutputStreamImpl* stream = new FileOutputStreamImpl;
    if (! stream)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(stream);
    *aInstancePtrResult = stream;
    return NS_OK;
}

//----------------------------------------------------------------------------------------
NS_BASE nsresult NS_NewOutputFileStream(
    nsIFileOutputStream** aInstancePtrResult,
    const nsFileSpec& inFile,
    PRInt32 nsprMode /*default = (PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE)*/,
    PRInt32 accessMode /*Default = 0700 (octal)*/)
// Factory method to get an nsOutputStream to a file.
// (see nsFileStream.h for more info on the parameters).
//----------------------------------------------------------------------------------------
{
    NS_PRECONDITION(aInstancePtrResult != nsnull, "null ptr");
    if (! aInstancePtrResult)
        return NS_ERROR_NULL_POINTER;

    FileOutputStreamImpl* stream = new FileOutputStreamImpl(inFile, nsprMode, accessMode);
    if (! stream)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(stream);
    *aInstancePtrResult = stream;
    return NS_OK;
}