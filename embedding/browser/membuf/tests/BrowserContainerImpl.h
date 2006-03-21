// License Block

#ifndef BROWSER_CONTAINER_IMPL_H
#define BROWSER_CONTAINER_IMPL_H

// plugin headers
#include "BrowserContainer.h"
#include "BrowserGlue.h"

// mozilla headers
#include "nsStringAPI.h"

// For the membuf implementation - uses BITMAP
#include <membufAllegroDefines.h>
#include "allegro.h"

/**
 * This class represents the interface to be implemented by an
 *  application that wants to embed Firefox using the membuf
 *  graphics implementation.
 */
class BrowserContainerImpl : public BrowserContainer
{
  public:

    BrowserContainerImpl();
    virtual ~BrowserContainerImpl();

    // BrowserContainer Interface

    /**
     * The browser will call this method to give the embedding
     *   application a handle to the data. The data will be in
     *   the format of a linear bitmap used by the Allegro
     *   BITMAP stuct.   
     */
    virtual nsresult SetBuffer(unsigned char* aBuffer);
    
    /**
     * The browser will call this method to give the embedding
     *   application a handle to the actual BITMAP struct being
     *   used. The embeddor can then query the BITMAP for color
     *   depth, size etc.
     */
    virtual nsresult SetBitmap(BITMAP* aBitmap);

    /**
     * Callback method that gets called when the browser has
     *   finished drawing to the bitmap.
     */
    virtual nsresult BitmapUpdated();

    // BrowserContainerImpl methods
    //   these would be methods the container would expose to the
    //   the rest of it's codebase in order to pass information
    //   on to the browser.

    nsresult Init( PRInt32 aWidth, PRInt32 aHeight );
    nsresult Run();
    nsresult SetDimensions( PRUint32 aWidth, PRUint32 aHeight );
    nsresult LoadURI( char *aURI );
    nsresult SetOutputFile( char *aFilename );
    nsresult CreateBrowser();
    nsresult DumpBitmapToFile();

  private:

    BrowserGlue     *mBrowser;
    BITMAP          *mBitmap;
    unsigned char   *mBuffer;
    nsCString        mFilename;
    nsCString        mURI;

};

#endif /* BROWSER_CONTAINER_IMPL_H */

// EOF

