/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include <stdio.h>
#include <stdlib.h>
#include "xlibrgb.h"
#include "nsXPrintContext.h"


//#undef XPRINT_ON_SCREEN

// why is this not in the constructor?
Display * nsXPrintContext::mDisplay = (Display *)0;

static int xerror_handler(Display *display, XErrorEvent *ev) {
    char errmsg[80];
    XGetErrorText(display, ev->error_code, errmsg, 80);
    fprintf(stderr, "lib_xprint: Warning (X Error) -  %s\n", errmsg);
    return 0;
}

/** ---------------------------------------------------
 *  Default Constructor
 */
nsXPrintContext::nsXPrintContext()
{
   mPContext = (XPContext )0;
   mPrintServerName = (char *)0;
   mPrinterName     = (char *)0;
   mScreen = (Screen *)0;
   mVisual = (Visual *)0;
   mGC     = (GC )0;
   mDrawable = (Drawable )0;
   mDepth = 0;
   mAlphaPixmap = 0;
   mImagePixmap = 0;
   mBandHeight = 0; // No banding
#ifdef RAS_PRINTER
   pSS = NULL;
   pPC = NULL;
   pJob = NULL;
#endif
}

/** ---------------------------------------------------
 *  Destructor
 */
nsXPrintContext::~nsXPrintContext()
{

#ifndef RAS_PRINTER
  // end the document
  EndDocument();
  // Cleanup things allocated along the way
//  XpDestroyContext(mDisplay, mPContext);
#endif
  XCloseDisplay(mDisplay);
}

NS_IMETHODIMP 
nsXPrintContext::Init(nsIDeviceContextSpecXP *aSpec)
{
  int prefDepth = 8;
#if defined(XPRINT_ON_SCREEN) || defined(RAS_PRINTER)

#ifdef DEBUG_TLOGUE
 printf("Init RAS_PRINTER or XPRINT_ON_SCREEN...\n");
#endif

  mPrintResolution = 300;
  prefDepth = 24;
  if (nsnull == mDisplay)
     mDisplay  = (Display *)XOpenDisplay(NULL);
  mScreen = XDefaultScreenOfDisplay(mDisplay);
  xlib_rgb_init_with_depth(mDisplay, mScreen, prefDepth);
  mScreenNumber = XDefaultScreen(mDisplay);

#ifdef RAS_PRINTER
  if (NS_OK != SetupRasterPrintJob(aSpec)) {
    printf("Failure in SetupRasterPrintJob\n");
    return NS_ERROR_FAILURE;
  }
#else
  mWidth = 1000;
  mHeight = 700;
  SetupWindow(0, 0, mWidth, mHeight);
  mTextZoom = 1.0f; 
  XMapWindow(mDisplay, mDrawable);
#endif // RAS_PRINTER

#else
  char *printservername = (char *)0;
  if (!(printservername =  getenv("XPDISPLAY"))) {
	printservername = strdup("localhost:1");
  }
  if (nsnull == mDisplay) {
     if (!(mDisplay = XOpenDisplay(printservername))) {
        fprintf(stderr,"failed to open display '%s'\n", printservername);
	return NS_ERROR_FAILURE;
     }
  }
  unsigned short width, height;
  XRectangle rect;

  SetupPrintContext(aSpec);
  mScreen = XpGetScreenOfContext(mDisplay, mPContext);
  mScreenNumber = XScreenNumberOfScreen(mScreen);
  xlib_rgb_init_with_depth(mDisplay, mScreen, prefDepth);

  XpGetPageDimensions(mDisplay, mPContext, &width, &height, &rect);

  mWidth = rect.width;
  mHeight = rect.height;
  SetupWindow(rect.x, rect.y, mWidth, mHeight);

  // mGC =  XDefaultGCOfScreen(mScreen);
  mTextZoom = 2.0f; 
#endif  // ! XPRINT_ON_SCREEN || RAS_PRINTER
  mPDisplay = mDisplay; 
  (void)XSetErrorHandler(xerror_handler);
  XSynchronize(mDisplay, True);
  return NS_OK;
}

NS_IMETHODIMP
nsXPrintContext::SetupWindow(int x, int y, int width, int height)
{
  XSetWindowAttributes xattributes;
  long xattributes_mask;
  Window parent_win;
  XVisualInfo *visual_info;
  unsigned long gcmask;
  XGCValues gcvalues;

  visual_info = xlib_rgb_get_visual_info();

  parent_win = RootWindow(mDisplay, mScreenNumber);
  xattributes.background_pixel = WhitePixel (mDisplay, mScreenNumber);
  xattributes.border_pixel = BlackPixel (mDisplay, mScreenNumber);
  xattributes_mask |= CWBorderPixel | CWBackPixel;

#ifdef _USE_PRIMITIVE_CALL_
  mVisual = visual_info->visual;
  mDepth = visual_info->depth;
 
  mDrawable = (Drawable) XCreateWindow(mDisplay, parent_win, x, y, width, 
			height, 2,
			mDepth, InputOutput, mVisual, xattributes_mask, 
			&xattributes );
#else
  mDepth  = XDefaultDepth(mDisplay, mScreenNumber);
  mVisual = XDefaultVisual(mDisplay, mScreenNumber);

#ifdef DEBUG_TLOGUE
  printf("mVisual->red=%lu,green=%lu,blue=%lu,bprgb=%d\n",mVisual->red_mask,mVisual->green_mask,mVisual->blue_mask,mVisual->bits_per_rgb);
#endif

#ifdef RAS_PRINTER
  mDrawable = (Drawable) XCreatePixmap(mDisplay,parent_win,width,height,mDepth);
#else
 mDrawable = (Drawable)XCreateSimpleWindow(mDisplay,
                                parent_win,
                                x, y,
                                width, height,
                                0, BlackPixel(mDisplay, mScreenNumber),
                                WhitePixel(mDisplay, mScreenNumber));
#endif // RAS_PRINTER
#endif // !_USE_PRIMITIVE_CALL

  gcmask = GCBackground | GCForeground | GCFunction ;
  gcvalues.background = WhitePixel(mDisplay, mScreenNumber);
  gcvalues.foreground = BlackPixel(mDisplay, mScreenNumber);
  gcvalues.function = GXcopy;
  mGC     = XCreateGC(mDisplay, mDrawable, gcmask, &gcvalues);

  return NS_OK;
}

NS_IMETHODIMP
nsXPrintContext::SetupPrintContext(nsIDeviceContextSpecXP *aSpec)
{
  XPPrinterList plist;
  int 		plistcnt;
  PRBool isAPrinter;
  int printSize;
  float top, bottom, left, right;
  char *buf;
  char cbuf[128];
  const int loglength = 128;
  char logname[loglength];

  // Get the Attributes
  aSpec->GetToPrinter( isAPrinter );
  aSpec->GetSize( printSize );
  aSpec->GetTopMargin( top );
  aSpec->GetBottomMargin( bottom );
  aSpec->GetLeftMargin( left );
  aSpec->GetRightMargin( right );

  
  // Check the output type
  if (isAPrinter == PR_TRUE) {
     aSpec->GetCommand( &buf );
  } else {
     aSpec->GetPath( &buf );
  }
     
  plist     =  XpGetPrinterList(mDisplay, buf, &plistcnt);
  mPContext =  XpCreateContext(mDisplay, plist[0].name );
  XpFreePrinterList(plist);
  XpSetContext(mDisplay, mPContext);

  // Set the Job attribute
  if (!getlogin_r(logname, loglength)) {
     sprintf(logname, "Mozilla-User");
  }
  sprintf(cbuf,"*job-owner: %s", logname);
  XpSetAttributes(mDisplay, mPContext,
                  XPJobAttr,(char *)cbuf,XPAttrMerge);

  // Set the Document Attributes
  // XpSetAttributes(mDisplay,mPContext, XPDocAttr,(char *)"*content-orientation: landscape",XPAttrMerge);

  mPrintResolution = 300;
  
  char *print_resolution = XpGetOneAttribute(mDisplay, mPContext, XPDocAttr,
					(char *)"default-printer-resolution");
  if (print_resolution) {
     char *tmp_str = strrchr(print_resolution, ':');
     if (tmp_str) {
        tmp_str++;
        mPrintResolution = atoi(tmp_str);
     }
     XFree(print_resolution);
  }
  // Check the output type
  if (isAPrinter == PR_TRUE) {
     XpStartJob(mDisplay, XPSpool );
  } else {
     XpStartJob(mDisplay, XPGetData );
  } 
  
  return NS_OK;
} 

#ifdef RAS_PRINTER 
NS_IMETHODIMP
nsXPrintContext::SetupRasterPrintJob(nsIDeviceContextSpecXP *aSpec)
{
  int printSize = NS_LETTER_SIZE;
  PRBool IsGrayscale = PR_FALSE;
  char *PortName = "/dev/usb/lp0"; // NULL; <- REVISIT: Remove hardcoded value

  mTextZoom = 3.0f;

  // this value is dependent on available memory:  X gives us 32bits/pixel
  // * 2400 pixels/raster (for letter or A4 paper at 300dpi)
  // = 9600 bytes/raster which is then multiplied by mBandHeight (# rasters/band)
  mBandHeight = 100; // = 960000 bytes

  aSpec->GetSize( printSize );
  aSpec->GetGrayscale( IsGrayscale );
  //aSpec->GetPath(&PortName);

#ifdef DEBUG_TLOGUE
  printf("Init HP Services to '%s'\n",PortName);
#endif
  // instantiate SystemServices, check for instantiation and constructor success
  pSS = new HPLinuxSS(PortName);
  if (nsnull != pSS)
  {
    if (NO_ERROR != pSS->constructor_error)
      return NS_ERROR_FAILURE;
  }
  else return NS_ERROR_FAILURE;

  // instantiate HP PrintContext, check for instantiation and constructor success
  pPC = new PrintContext(pSS);
  if (nsnull != pPC)
  {
    if (NO_ERROR != pPC->constructor_error)
      return NS_ERROR_FAILURE;
  }
  else return NS_ERROR_FAILURE;

  if( !(pPC->PrinterSelected()) ) {
    pSS->DisplayPrinterStatus(DISPLAY_NO_PRINTER_FOUND);

    // wait for user to cancel the job, otherwise they
    // might miss the error message
    while (pSS->BusyWait(500) != JOB_CANCELED);

    return NS_ERROR_FAILURE;
  }

  switch(printSize)
  {
    case(NS_LETTER_SIZE): pPC->SetPaperSize(LETTER);
                          break;
    case(NS_A4_SIZE): pPC->SetPaperSize(A4);
                          break;
    case(NS_LEGAL_SIZE): pPC->SetPaperSize(LEGAL);
                          break;
    default: pPC->SetPaperSize(LETTER);
                          printf("No Valid Paper Selection: Default to LETTER ");
                          break;
  }

  mWidth = pPC->InputPixelsPerRow();
  mHeight = int( pPC->PrintableHeight() * 300 ); // height in inches * dpi

  if (PR_TRUE == IsGrayscale)
    pPC->SelectPrintMode(GRAYMODE_INDEX);

  SetupWindow(0,0,mWidth,mBandHeight);

  // instantiate job, check for instantiation and constructor success
  pJob = new Job(pPC);
  if (nsnull != pJob)
  {
    if (NO_ERROR != pJob->constructor_error)
      return NS_ERROR_FAILURE;
  }
  else return NS_ERROR_FAILURE;

  return NS_OK;

}
#endif

NS_IMETHODIMP
nsXPrintContext::BeginDocument()
{
  printf("XPrint: BeginDocument\n");
  return NS_OK;
}

NS_IMETHODIMP 
nsXPrintContext::BeginPage()
{
#ifndef RAS_PRINTER
  XpStartPage(mDisplay, mDrawable);
  // Move the print window according to the given margin
  // XMoveWindow(mDisplay, mDrawable, 100, 100);
#endif

  return NS_OK;
}

NS_IMETHODIMP 
nsXPrintContext::StartBand()
{

#ifdef RAS_PRINTER
  // We need to clear our pixmap at init and between bands
  XSetForeground(mDisplay,mGC,WhitePixel(mDisplay,mScreenNumber));
  XFillRectangle(mDisplay,mDrawable,mGC,0,0,mWidth,mBandHeight);
  XSetForeground(mDisplay,mGC,BlackPixel(mDisplay,mScreenNumber));
#endif

  return NS_OK;
}

NS_IMETHODIMP 
nsXPrintContext::EndBand()
{

#ifdef RAS_PRINTER
  XImage* x_image = XGetImage(mDisplay, mDrawable, 0, 0, mWidth, mBandHeight, 0xffffff, ZPixmap);
  XInitImage(x_image);

  unsigned short *pix16 = NULL;

#ifdef DEBUG_TLOGUE
  //printf("w=%d,h=%d,xoff=%d,f=%d,data=0x%x,bo=%d,bu=%d,bbo=%d,bp=%d,d=%d,bpl=%d,bpp=%d,red=0x%lx,green=0x%lx,blue=0x%lx\n",
  //x_image->width,x_image->height,x_image->xoffset,x_image->format,x_image->data,
  //x_image->byte_order,x_image->bitmap_unit,x_image->bitmap_bit_order,x_image->bitmap_pad,
  //x_image->depth,x_image->bytes_per_line,x_image->bits_per_pixel,x_image->red_mask,x_image->green_mask,x_image->blue_mask);
#endif

  BYTE RGBtriplet[14400];  // this raster will handle up to 600dpi x 8" 24bit RGB
  int i=0, j=0, k=0, ras_value=0, iFactor=1;

  for (i = 0; i<mBandHeight; i++)
  {
    ras_value = 0;

    if(x_image->depth == 24)
    {
      iFactor = 4;

      // transform the 32bit RGB pixel to 24bit RGB
      for (j=i*mWidth*iFactor, k=0; j<(i+1)*mWidth*iFactor; j+=iFactor, k+=3)
      {

         // Redhat/Intel - BGRX -> RGB
         RGBtriplet[k]   = (BYTE)(x_image->data[j+2]);
         RGBtriplet[k+1] = (BYTE)(x_image->data[j+1]);
         RGBtriplet[k+2] = (BYTE)(x_image->data[j]);
/*
         // XRGB -> RGB (just remove padding byte)
         RGBtriplet[k]   = (BYTE)(x_image->data[j+1]);
         RGBtriplet[k+1] = (BYTE)(x_image->data[j+2]);
         RGBtriplet[k+2] = (BYTE)(x_image->data[j+3]);
*/
         ras_value += RGBtriplet[k]+RGBtriplet[k+1]+RGBtriplet[k+2];
      }
    }
    else if(x_image->depth == 16)
    {
      iFactor = 2;

      // transform the 16bit (5-6-5) RGB pixel to 24bit RGB
      for (j=i*mWidth*iFactor, k=0; j<(i+1)*mWidth*iFactor; j+=iFactor, k+=3)
      {

         // 16bit
         pix16 = (unsigned short *)(&(x_image->data[j]));
         RGBtriplet[k]   = BYTE( ( *pix16 & 0xF800 ) >> 11)*255/31;
         RGBtriplet[k+1] = BYTE( ( *pix16 & 0x07E0 ) >>  5)*255/63;
         RGBtriplet[k+2] = BYTE( ( *pix16 & 0x001F ) >>  0)*255/31;

         ras_value += RGBtriplet[k]+RGBtriplet[k+1]+RGBtriplet[k+2];
      }
    }
    else return NS_ERROR_FAILURE;  // 8-bit is not supported

    // don't send empty rasters into the imaging pipeline - HUGE speed increase
    if(ras_value == 255*mWidth*3)
    {
      pJob->SendRasters( NULL, NULL );
//      printf("NULL ");
    }
    else pJob->SendRasters( RGBtriplet, NULL );

//    printf("Raster: %d\n", i);
    
  }

  XDestroyImage(x_image);
  x_image = NULL;
#endif

  return NS_OK;
}

NS_IMETHODIMP 
nsXPrintContext::EndPage()
{
#ifdef RAS_PRINTER
  pJob->NewPage();
#else
  XpEndPage(mDisplay);
#endif
  return NS_OK;
}

NS_IMETHODIMP 
nsXPrintContext::EndDocument()
{
printf("XPrint: EndDocument\n");
#ifdef RAS_PRINTER
  if (pJob != NULL) delete pJob;
  if (pPC != NULL) delete pPC;
  if (pSS != NULL) delete pSS;
  
  XFreePixmap(mDisplay,(Pixmap)mDrawable);
  XFlush(mDisplay);
#else
  XFlush(mDisplay);
  XpEndJob(mDisplay);
  // Cleanup things allocated along the way
  XpDestroyContext(mDisplay, mPContext);
  // XCloseDisplay(mDisplay);
#endif
  return NS_OK;
}

NS_IMETHODIMP
nsXPrintContext::DrawImage(nsIImage *aImage,
		PRInt32 aSX, PRInt32 aSY, PRInt32 aSWidth, PRInt32 aSHeight,
		PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight)
{
   PRUint8 *image_bits = aImage->GetBits();
   PRInt32 row_bytes   = aImage->GetLineStride();
   
   // XpSetImageResolution(mDisplay, mPContext, new_res, &prev_res);
   xlib_draw_gray_image(mDrawable,
                      mGC,
                      aDX, aDY, aDWidth, aDHeight,
                      XLIB_RGB_DITHER_MAX,
                      image_bits + row_bytes * aSY + 3 * aDX,
                      row_bytes);

   // XpSetImageResolution(mDisplay, mPContext, prev_res, &new_res);
  return NS_OK;
}


// Draw the bitmap, this draw just has destination coordinates
NS_IMETHODIMP
nsXPrintContext::DrawImage(nsIImage *aImage,
                 PRInt32 aX, PRInt32 aY,
                 PRInt32 aWidth, PRInt32 aHeight)
{
  PRInt32 width 	= aImage->GetWidth();
  PRInt32 height 	= aImage->GetHeight();
  PRUint8 *alphaBits 	= aImage->GetAlphaBits();
  PRInt32  alphaRowBytes = aImage->GetAlphaLineStride();
  PRUint8 *image_bits 	= aImage->GetBits();
  PRInt32 row_bytes   	= aImage->GetLineStride();

  // XXX kipp: this is temporary code until we eliminate the
  // width/height arguments from the draw method.
  if ((aWidth != width) || (aHeight != height)) {
     aWidth = width;
     aHeight = height;
  }

  XImage *x_image = nsnull;
  GC      gc;
  XGCValues gcv;

  // Create gc clip-mask on demand
  if ((alphaBits != nsnull) && (mAlphaPixmap == 0)) {
    if (!mAlphaPixmap) {
      mAlphaPixmap = XCreatePixmap(mDisplay, 
				RootWindow(mDisplay, mScreenNumber),
				aWidth, aHeight, 1);
    }

    // Make an image out of the alpha-bits created by the image library
    x_image = XCreateImage(mDisplay, mVisual,
                           1, /* visual depth...1 for bitmaps */
                           XYPixmap,
                           0, /* x offset, XXX fix this */
                           (char *)alphaBits,  /* cast away our sign. */
                           aWidth,
                           aHeight,
                           32,/* bitmap pad */
                           alphaRowBytes); /* bytes per line */

    x_image->bits_per_pixel=1;

    /* Image library always places pixels left-to-right MSB to LSB */
    x_image->bitmap_bit_order = MSBFirst;

    /* This definition doesn't depend on client byte ordering
       because the image library ensures that the bytes in
       bitmask data are arranged left to right on the screen,
       low to high address in memory. */
    x_image->byte_order = MSBFirst;
#if defined(IS_LITTLE_ENDIAN)
    // no, it's still MSB XXX check on this!!
    //      x_image->byte_order = LSBFirst;
#elif defined (IS_BIG_ENDIAN)
    x_image->byte_order = MSBFirst;
#else
#error ERROR! Endianness is unknown;
#endif
    // Write into the pixemap that is underneath gdk's mAlphaPixmap
    // the image we just created.
    memset(&gcv, 0, sizeof(XGCValues));
    gcv.function = GXcopy;
    gc = XCreateGC(mDisplay, mAlphaPixmap, GCFunction, &gcv);

    XPutImage(mDisplay, mAlphaPixmap, gc, x_image, 0, 0, 0, 0,
              aWidth, aHeight);
    XFreeGC(mDisplay, gc);

    // Now we are done with the temporary image
    x_image->data = 0;          /* Don't free the IL_Pixmap's bits. */
    XDestroyImage(x_image);
  }
  
  if (nsnull == mImagePixmap) {
    // Create an off screen pixmap to hold the image bits.
    mImagePixmap = XCreatePixmap(mDisplay,
				RootWindow(mDisplay, mScreenNumber),
                                aWidth, aHeight,
                                mDepth);
    XSetClipOrigin(mDisplay, mGC, 0, 0);
    XSetClipMask(mDisplay, mGC, None);
    GC gc;
    XGCValues xvalues;
    unsigned long xvalues_mask;

    xvalues.function = GXcopy;
    xvalues.fill_style = FillSolid;
    xvalues.arc_mode = ArcPieSlice;
    xvalues.subwindow_mode = ClipByChildren;
    xvalues.graphics_exposures = True;
    xvalues_mask = GCFunction | GCFillStyle | GCArcMode | GCSubwindowMode | GCGraphicsExposures;

    gc = XCreateGC(mDisplay, mImagePixmap, xvalues_mask, &xvalues); 
    // XpSetImageResolution(mDisplay, mPContext, new_res, &prev_res);
#ifdef _USE_PRIMITIVE_CALL_
    xlib_draw_gray_image(mImagePixmap,
                      gc,
                      0, 0, aWidth, aHeight,
                      XLIB_RGB_DITHER_MAX,
                      image_bits,
                      row_bytes);
#else
    xlib_draw_rgb_image (mImagePixmap,
                         gc,
                         0, 0, aWidth, aHeight,
                         XLIB_RGB_DITHER_NONE,
                         image_bits, row_bytes);
#endif
   //  XpSetImageResolution(mDisplay, mPContext, prev_res, &new_res);
  }
  if (nsnull  != mAlphaPixmap)
  {
    // set up the gc to use the alpha pixmap for clipping
    XSetClipOrigin(mDisplay, mGC, aX, aY);
    XSetClipMask(mDisplay, mGC, mAlphaPixmap);
  }

  // copy our off screen pixmap onto the window.
  XCopyArea(mDisplay,                  // display
            mImagePixmap,              // source
            mDrawable,    		// dest
            mGC,          		// GC
            0, 0,                    // xsrc, ysrc
            aWidth, aHeight,           // width, height
            aX, aY);                     // xdest, ydest

  if (mAlphaPixmap != nsnull) {
    XSetClipOrigin(mDisplay, mGC, 0, 0);
    XSetClipMask(mDisplay, mGC, None);
  }

  return NS_OK;
}

NS_IMETHODIMP nsXPrintContext::GetPrintResolution(int &aPrintResolution) const
{
  aPrintResolution = mPrintResolution;
  return NS_OK;
}

NS_IMETHODIMP nsXPrintContext::SetForegroundColor(nscolor aColor)
{
  
  xlib_rgb_gc_set_foreground(mGC,
                NS_RGB(NS_GET_B(aColor), NS_GET_G(aColor), NS_GET_R(aColor)));
  return NS_OK;
}
