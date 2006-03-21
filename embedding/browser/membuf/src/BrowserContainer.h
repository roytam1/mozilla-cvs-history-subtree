// License Block

#ifndef BROWSER_CONTAINER_H
#define BROWSER_CONTAINER_H

// mozilla headers
#include "nscore.h"  // for nsresult

// For the BITMAP implementation 
#include "membufAllegroDefines.h"
#include <allegro.h>

/**
 * This class represents the interface to be implemented by an
 *  application that wants to embed Firefox using the membuf
 *  graphics implementation.
 */
class BrowserContainer
{
  public:

    BrowserContainer() {}
    virtual ~BrowserContainer() {}

    /**
     * The browser will call this method to give the embedding
     *   application a handle to the data. The data will be in
     *   the format of a linear bitmap used by the Allegro
     *   BITMAP struct.
     */
    virtual nsresult SetBuffer(unsigned char* aBuffer) = 0;

    /**
     * The browser will call this method to give the embedding
     *   application a handle to the actual BITMAP struct being
     *   used. The embeddor can then query the BITMAP for color
     *   depth, size etc.
     */
    virtual nsresult SetBitmap(BITMAP* aBitmap) = 0;

    /**
     * Callback method that gets called when the browser has
     *   finished drawing to the bitmap.
     */
    virtual nsresult BitmapUpdated() = 0;
};

#endif /* BROWSER_CONTAINER_H */

// EOF

