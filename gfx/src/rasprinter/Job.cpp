//
//  Copyright (c) 2000, Hewlett-Packard Co.
//  All rights reserved.
//  
//  This software is licensed solely for use with HP products.  Redistribution
//  and use with HP products in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//  
//  -	Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//  -	Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//  -	Neither the name of Hewlett-Packard nor the names of its contributors
//      may be used to endorse or promote products derived from this software
//      without specific prior written permission.
//  -	Redistributors making defect corrections to source code grant to
//      Hewlett-Packard the right to use and redistribute such defect
//      corrections.
//  
//  This software contains technology licensed from third parties; use with
//  non-HP products is at your own risk and may require a royalty.
//  
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
//  CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
//  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
//  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//  DISCLAIMED. IN NO EVENT SHALL HEWLETT-PACKARD OR ITS CONTRIBUTORS
//  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
//  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
//  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
//  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
//  DAMAGE.
//

#ifdef PROTO
#include "../include/Header.h"
#include "../imaging/open/imaging.h"
#include "../imaging/closed/imaging.h"
#else
#include "Header.h"
#include "imaging.h"
#endif


extern float frac(float f);

#ifdef PROTO
extern BOOL argProprietary;
#endif

#ifndef PROTO
extern Imager* Create_Imager( SystemServices* pSys, PrintMode* pPM,
                unsigned int iInputWidth, unsigned int iNumRows[], unsigned int HiResFactor );
#endif


#define Bytes(x) ((x/8)+(x%8))
//////////////////////////////////////////////////////////////////////////
// The Job object receives data for a contiguous set of pages targeted at
// a single printer, with a single group of settings encapsulated in the
// PrintContext. 
// At least one page will be ejected for a job, if any data at all 
// is printed (no half-page jobs). If settings are to be changed, this
// must be done between jobs.
//
// The Job constructor is responsible for retrieving or instantiating,
// and aligning, the various components of the system:
//
// 1. the Printer, retrieved from PrintContext
// 2. the color processor (Imager)
// 3. the Translator (with its components GraphicsTranslator and TextTranslator)
// 4. the TextManager
//
// The Job constructor (via SetupTranslator) also calls Translator::SendHeader 
// to put the printer into the desired PCL state.

Job::Job(PrintContext* pPC)
	: thePrintContext(pPC), CurrentMode(pPC->CurrentMode), 
      skipcount(0), RowsInput(0), DataSent(FALSE)
#ifdef USAGE_LOG
	, UText(0), UTextCount(0)
#endif
{ 
#ifdef CAPTURE
	Capture_Job(pPC);
#endif

    if (!thePrintContext->PrinterSelected())
      {
        constructor_error = NO_PRINTER_SELECTED; 
        return;
      }
    thePrinter = thePrintContext->thePrinter;


    // flush any garbage out of the printer's buffer
    thePrinter->Flush();

    InputDPI = thePrintContext->InputResolution();

	// create Imager object
	constructor_error=SetupColor();
	CERRCHECK;
    
	// create Translator object
	constructor_error = SetupTranslator();
	CERRCHECK;

#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
    unsigned int pixelsX = (unsigned int)(thePrintContext->printablewidth() * 
                            thePrintContext->CurrentMode->BaseResX);
    unsigned int pixelsY = (unsigned int)(thePrintContext->printableheight() * 
                            thePrintContext->CurrentMode->BaseResY);

	theTextManager=new TextManager( theTranslator, pixelsX,pixelsY );
	CNEWCHECK(theTextManager);
	constructor_error = theTextManager->constructor_error;
	CERRCHECK;
#endif

    if (!thePrintContext->ModeAgreesWithHardware(TRUE))
        constructor_error = WARN_MODE_MISMATCH;

#ifdef USAGE_LOG
	UHeader[0]='\0';
#endif
}

DRIVER_ERROR Job::SetupColor()
// Subroutine of Job constructor to handle Imager initialization.
// Imager needs:
//	the SystemServices pointer
//  the input width of each raster (pagewidth @InputDPI, usually 300)
//  a multiplier for planes that need doubling (tripling,etc.) to feed printer mode
//  the colormap
//  the number of inks (colors)
//  the depth (i.e. number of bits used to represent levels) for each color
{
	int i;
    
    // rows per call for mixed-res only
    unsigned int numrows[MAXCOLORPLANES];
    unsigned int ResBoost;   
   
    for (i=0; i<MAXCOLORPLANES; i++)
        if (CurrentMode->MixedRes)
            numrows[i] =  CurrentMode->ResolutionY[i] / InputDPI;
        else numrows[i]=1;

    ResBoost = CurrentMode->BaseResX / InputDPI;
    RowMultiple = CurrentMode->BaseResY / InputDPI;

#ifdef PROTO
    if (!argProprietary)
        pImager = new Imager_Open(thePrintContext->pSS, CurrentMode,
                                  thePrintContext->InputRasterWidth, numrows,ResBoost);
    else
        pImager = new Imager_Prop(thePrintContext->pSS, CurrentMode,
                                  thePrintContext->InputRasterWidth, numrows,ResBoost);
#else
    pImager = Create_Imager(thePrintContext->pSS, CurrentMode,
                                  thePrintContext->InputRasterWidth, numrows,ResBoost);
#endif


	NEWCHECK(pImager);
	return pImager->constructor_error;

}

DRIVER_ERROR Job::SetupTranslator()
// Subroutine of Job constructor to handle Translator initialization;
// presupposes Imager has been created
{
	DRIVER_ERROR err;
// create Translator object to handle command-language formatting
	// note: Translator is incomplete until its Graphics and Text components
	// have been installed 
	theTranslator = new Translator(thePrintContext->pSS,thePrinter,thePrintContext);
	NEWCHECK(theTranslator);
	err=theTranslator->constructor_error;	
	ERRCHECK;

	// install graphical component of Translator
	// HRES may later be selected using Printer and/or UI
	err = theTranslator->InstallGraphicsTranslator(pImager,thePrintContext->InputRasterWidth, thePrinter->bUseCompression);
	ERRCHECK;
	
	err = theTranslator->SendHeader();
	ERRCHECK;

	// update CAPy in case header used it
	CAPy=theTranslator->GetCAPy();

#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
	err = theTranslator->InstallTextTranslator();
#endif
    return err;
}


Job::~Job()
{ 
#ifdef CAPTURE
	Capture_dJob();
#endif
	
	// Client isn't required to call NewPage at end of last page, so
	// we may need to eject a page now.
	if (DataSent)
		newpage();

#ifdef USAGE_LOG
	theTranslator->PrintDotTotals();
#endif

	// Tell printer that job is over.
	theTranslator->EndJob();
		  
// Delete the 4 components created in Job constructor.
#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
DBG1("deleting TextManager\n");
	if (theTextManager)
		delete theTextManager;
#endif

DBG1("deleting translator\n");
	if (theTranslator)
		delete theTranslator;
DBG1("deleting umpqua\n");
	if (pImager)
		delete pImager;

DBG1("done with ~Job\n");

}
//////////////////////////////////////////////////////////////////////////////

DRIVER_ERROR GraphicsTranslator::MergeHiResBin(BYTE* data)
// Used by SendRasters --
// do OR of this data with existing Kplane(s)
// In case of multi-level ink output, map to darkest level, 
// i.e. write to all Kplanes.
{
	int i,j,row;
	BYTE b;

	if (data==NULL)
		return SYSTEM_ERROR;	// should never happen

	int ByteSize = Bytes(RasterWidth);

	for (j=0; j < theImager->ColorDepth[K]; j++)
        for (row=0; row < theImager->NumRows[K]; row++)
	  {
		BYTE* k = theImager->ColorPlane[K][row][j];
		BYTE* pdata=data;
	
		for (i=0; i<ByteSize; i++)
		  {
			b= *pdata++;
			b = b | *k;
			*k++ = b;
		  }
	  }
 return NO_ERROR;  
}

///////////////////////////////////////////////////////////////////////////////////
//
DRIVER_ERROR Job::SendRasters(
					BYTE* ImageData,		// RGB24 data for one raster
					BYTE* BinaryTextData	// 1-bit black data for one raster
								)
// This is how graphical data is sent to the driver.
// Either of the ptrs may be NULL to indicate a blank raster. 

// We do not check for rasters where data is supplied but happens 
// to be all blank (white); caller should have used NULL in this case.
{
#ifdef CAPTURE
	Capture_SendRasters(ImageData,BinaryTextData);
#endif

	DRIVER_ERROR err=NO_ERROR;

    RowsInput++;
    
    // need to put out some data occasionally in case of all-text page,
    // so printer will go ahead and start printing the text before end of page
    if ((ImageData==NULL) && (BinaryTextData ==NULL))
    {
        skipcount++;
        if (skipcount >= 200)
        {
            skipcount=0;     
            ImageData=pImager->BlankRaster;
        }
    }
    else skipcount=0;

    for (int k=0; k < RowMultiple; k++)
    {
        // we maintain and reset CAPy ourselves in case of intervening text calls
        if (k==0)   // only adjust CAPy once per input raster 
                    // because CAPy addresses 300 dpi grid
        {
	        if ( ((ImageData==NULL) && (BinaryTextData==NULL))  // skip row if null data
                // in non-gui mode, we just send it every time anyway
                // (needs to be re-investigated if gui mode allows text)
                || (!thePrinter->UseGUIMode(thePrintContext->CurrentPrintMode())) 
               )
           
              {
                // remember CAPy addresses 300 dpi grid
                if ((InputDPI == 300) || ((RowsInput % (InputDPI/300))==0) )
                {
                    err=theTranslator->SendCAPy(CAPy);
                    ERRCHECK;
                }
              }
            // keep our internal counter in sync with rasters sent
            if ((InputDPI == 300) || ((RowsInput % (InputDPI/300))==0) )
                CAPy++;  
        }
	    
	    if (ImageData!=NULL)
		    pImager->Process(ImageData);

	    if (BinaryTextData != NULL)
		    theTranslator->MergeHiResBin(BinaryTextData); 

	    if ((ImageData!=NULL) || (BinaryTextData != NULL))	// if any data at all
	      {
		    err= theTranslator->SendRaster();				// do PCL encapsulation
		    ERRCHECK;
		    DataSent=TRUE;		// so we know we need a formfeed when job ends
	      }
	    else pImager->Restart();	// skipping white space,so re-init 
    }
 
 return err;
}

DRIVER_ERROR Job::newpage()
// (for internal use, called by external NewPage)
{
	DRIVER_ERROR err = theTranslator->FormFeed();
	ERRCHECK;

// re-init the Imager buffer
	pImager->Restart();	

// reset vertical cursor counter
    if (thePrinter->UseGUIMode(thePrintContext->CurrentPrintMode()) )
// Venice in GUImode doesn't accept top-margin setting, so we use CAP for topmargin
      { 
        unsigned int ResMultiple = (InputDPI / 300);
        // see Header895::StartSend() for computation to get 88
        CAPy = 88 / ResMultiple;
        theTranslator->SendCAPy( CAPy );
      }
	else CAPy=0;

    skipcount = RowsInput = 0;

// reset flag used to see if formfeed needed
	DataSent=FALSE;

    if (!thePrintContext->ModeAgreesWithHardware(TRUE))
        return WARN_MODE_MISMATCH;

#ifdef USAGE_LOG
	theTranslator->PrintDotCount(UHeader);
	theTranslator->NextPage();
	UTextCount=UText=0;
#endif

	return NO_ERROR;
}

DRIVER_ERROR Job::NewPage()
// External entry point
{
#ifdef CAPTURE
	Capture_NewPage();
#endif
	return newpage();
}

//////////////////////////////////////////////////////////////////////
#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
DRIVER_ERROR Job::TextOut(const char* pTextString, unsigned int iLenString, 
				  const Font& font, int iAbsX, int iAbsY ) 
// This is how ASCII data is sent to the driver.
// Everything is really handled by the TextManager, including error checking.
{
#ifdef CAPTURE
	Capture_TextOut(pTextString, iLenString, font, iAbsX, iAbsY);
#endif


	DRIVER_ERROR err=theTextManager->TextOut(pTextString,iLenString,font,iAbsX,iAbsY);
	ERRCHECK;

	DataSent=TRUE;

#ifdef USAGE_LOG
	if (iLenString > UTextSize)
	  {	iLenString = UTextSize-1;
		UHeader[iLenString]='\0';
	  }
	if (UTextCount<2)
	  {
		strcpy(UHeader,pTextString);
		UText += iLenString;
	  }
	if (UTextCount==1)
		UHeader[UText]='\0';
	UTextCount++;
	
#endif

	return err;
}
#endif	

