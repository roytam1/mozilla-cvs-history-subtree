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

// PrintContext
//
// The PrintContext is the second item created in the
// driver calling sequence, following SystemServices.
// It provides the interface for inquiring about
// supported devices, and for setting all optional
// properties of the imaging pipeline as determined by
// given devices.

#ifdef PROTO
#include "../include/Header.h"
#else
#include "Header.h"
#endif


#ifdef PROTO
	extern int argColor;
#endif

extern PAPER_SIZE MediaSizeToPaper(MediaSize msize);
extern MediaSize PaperToMediaSize(PAPER_SIZE psize);

////////////////////////////////////////////////////////////////////
// PrintContext constructor
// In the normal case where bidirectional communication
// was established by SystemServices, successful
// construction results in instantiation of the proper
// Printer class; otherwise client will have to complete
// the process with a subsequent call to SelectDevice.
// The last two parameters are optional. In the case where
// PixelsPerRow has the default setting of zero,
// the value of printableregionX will be used.
//
PrintContext::PrintContext(SystemServices * pSysServ, 
                           unsigned int PixelsPerRow, 
                           PAPER_SIZE ps)
    : CurrentMode((PrintMode*)NULL), pSS(pSysServ), 
      thePrinter((Printer*)NULL), InputRes(300), InputRasterWidth(PixelsPerRow), 
	  CurrentModeIndex(1)   
{
#ifdef CAPTURE
    Capture_PrintContext(PixelsPerRow,ps,pSS->IOMode);
#endif


    InitPSMetrics();    // create array of paper sizes
    
    DR = pSS->DR;

    constructor_error = NO_ERROR;

    if (!pSS->IOMode.bDevID)     // SystemServices couldn't establish good DevID
        return;             // leave in incomplete state
    
    if ( (constructor_error=DR->SelectDevice(pSS->strModel,pSS->strPens,pSS)) != NO_ERROR)
	  {
        if (constructor_error == UNSUPPORTED_PRINTER)
          {
			pSS->DisplayPrinterStatus(DISPLAY_PRINTER_NOT_SUPPORTED);
            //wait to read message
			while (pSS->BusyWait(500) != JOB_CANCELED) ;   // nothing.....
            return;
          }
        else { DBG1("PrintContext - error in SelectDevice\n"); return; }
	  }

    // Device selected... now instantiate a printer object 
	if ( (constructor_error = DR->InstantiatePrinter(thePrinter,pSS)) != NO_ERROR) 
	  { DBG1("PrintContext - error in InstantiatePrinter\n"); return; }

	pSS->AdjustIO(thePrinter->IOMode);

    if (SelectDefaultMode())     
        SetWidths();        // set InputRasterWidth, PageWidth
    else constructor_error = SYSTEM_ERROR;
}

DRIVER_ERROR PrintContext::SetMode(unsigned int ModeIndex) 
{ 
    if (ModeIndex>=GetModeCount())
        return INDEX_OUT_OF_RANGE;

    CurrentModeIndex=ModeIndex; 
    CurrentMode = thePrinter->GetMode(ModeIndex); 
    if (CurrentMode==NULL)
        return SYSTEM_ERROR;
 return NO_ERROR;
}

BOOL PrintContext::SelectDefaultMode()
// return TRUE if mode set okay
{
    BOOL modeOK[MAX_PRINTMODES];
    unsigned int maxmodes = GetModeCount();

    int i;
    for (i=0; i < maxmodes; i++)
    {   
        modeOK[i]=FALSE;
        if (SetMode(i) == NO_ERROR)
            if (ModeAgreesWithHardware(FALSE))
                modeOK[i]=TRUE;
    }
    
    i=0;
    if ((maxmodes>0) && modeOK[DEFAULTMODE_INDEX])
        SetMode(DEFAULTMODE_INDEX);
    else if (modeOK[GRAYMODE_INDEX])
            SetMode(GRAYMODE_INDEX);
         else 
         {   i=SPECIALMODE_INDEX;
             while ((i<=MAX_PRINTMODES) && (SetMode(i)!=NO_ERROR))
                    i++;
         }
    return (i<maxmodes);
}

char* PrintContext::GetModeName()
{
  if (CurrentMode==NULL)
    return (char*)NULL;
  return CurrentMode->ModeName;
}

void PrintContext::SetWidths()  // MUST HAVE CURRENTMODE SET
// set PageWidth and InputRasterWidth based on print mode
{
   PageWidth = CurrentMode->BaseResX * (unsigned int)printablewidth();      

    if (InputRasterWidth==0)
        InputRasterWidth=PageWidth;

}

BOOL PrintContext::ModeAgreesWithHardware(BOOL QueryPrinter)
// no check for null printer
{
    BOOL agree=FALSE;
    PEN_TYPE ePen;

    if (pSS->IOMode.bDevID==FALSE)
        return TRUE;
 
    if (thePrinter->ParsePenInfo(ePen,QueryPrinter) != NO_ERROR)
        return FALSE;

    for (int i=0; i < MAX_COMPATIBLE_PENS; i++)
        if (ePen == CurrentMode->CompatiblePens[i])
            agree = TRUE;

    if (!agree)
        return FALSE;

    return thePrinter->ModeAllowable(CurrentModeIndex);

}

DRIVER_ERROR PrintContext::SetInputResolution(unsigned int Res)
{
#ifdef CAPTURE
Capture_SetInputResolution(Res);
#endif
    if ((Res!=300) && (Res!=600))
        return ILLEGAL_RESOLUTION;

    if (CurrentMode)
        if (Res > CurrentMode->BaseResX)
            return ILLEGAL_RESOLUTION;

    InputRes=Res;
  
  return NO_ERROR;
}

void PrintContext::InitPSMetrics()
{
// this array is directly linked to PAPER_SIZE enum
// note: fPrintablePageY is related to fPrintableStartY to allow for a 2/3" bottom margin.
//  If fPrintableStartY is altered, fPrintablePageY should be correspondingly updated.

PSM[LETTER].fPhysicalPageX = 8.5;
PSM[LETTER].fPhysicalPageY = 11.0;
PSM[LETTER].fPrintablePageX = 8.0;
PSM[LETTER].fPrintablePageY = 10.0;
PSM[LETTER].fPrintableStartY = 0.3333;

PSM[A4].fPhysicalPageX = 8.27;
PSM[A4].fPhysicalPageY = 11.69;
PSM[A4].fPrintablePageX = 8.0;
PSM[A4].fPrintablePageY = 10.7;
PSM[A4].fPrintableStartY = 0.3333;

PSM[LEGAL].fPhysicalPageX = 8.5;
PSM[LEGAL].fPhysicalPageY = 14.0;
PSM[LEGAL].fPrintablePageX = 8.0;
PSM[LEGAL].fPrintablePageY = 13.0;
PSM[LEGAL].fPrintableStartY = 0.3333;

// Corresponds to 4x6" photo paper used in the 9xx series photo tray.
// The apparent 1/8" bottom margin is allowed because of a pull-off tab on the media.
PSM[PHOTO_SIZE].fPhysicalPageX = 4.0;
PSM[PHOTO_SIZE].fPhysicalPageY = 6.0;
PSM[PHOTO_SIZE].fPrintablePageX = 3.75;
PSM[PHOTO_SIZE].fPrintablePageY = 5.75;
PSM[PHOTO_SIZE].fPrintableStartY = 0.125;
}
   

PrintContext::~PrintContext()
{ 
#ifdef CAPTURE
Capture_dPrintContext();
#endif
DBG1("deleting PrintContext\n");

    if (thePrinter)
        delete thePrinter;
}
///////////////////////////////////////////////////////////////////////
// Functions to report on device-dependent properties.
// Note that mixed-case versions of function names are used
// for the client API; lower-case versions are for calls
// made by the driver itself, to avoid the CAPTURE instrumentation.
///////////////////////////////////////////////////////////////////////
float PrintContext::PhysicalPageSizeX()   // returned in inches
{ 
	if (thePrinter==NULL)
		return 0.0;
	return PSM[ MediaSizeToPaper(CurrentMode->theMediaSize) ].fPhysicalPageX; 
}

float PrintContext::PhysicalPageSizeY()   // returned in inches
{ 
	if (thePrinter==NULL)
		return 0.0;
	return PSM[ MediaSizeToPaper(CurrentMode->theMediaSize) ].fPhysicalPageY; 
}

float PrintContext::PrintableWidth()	// returned in inches
// for external use
{ 
	return printablewidth();
}

/////////////////////////////////////////////////////////////////////
// NOTE ON RESOLUTIONS: These functions access ResolutionX[C],
//  where C is the conventional index for Cyan. The assumption 
//  is that Res[C]=Res[M]=Res[Y], AND that Res[K]>=Res[C]
/////////////////////////////////////////////////////////////////////

float PrintContext::printablewidth()	
// for internal use
{ 
	if (thePrinter==NULL)
		return 0.0;
	return PSM[ MediaSizeToPaper(CurrentMode->theMediaSize) ].fPrintablePageX;  
}

float PrintContext::PrintableHeight()     // returned in inches
// for external use
{ 
	return printableheight();
}

float PrintContext::printableheight()	
// for internal use
{ 
	if (thePrinter==NULL)
		return 0.0;
	return PSM[ MediaSizeToPaper(CurrentMode->theMediaSize) ].fPrintablePageY; 
}


float PrintContext::PrintableStartX() // returned in inches
{ 
	if (thePrinter==NULL)
		return 0;
    // this return value centers the printable page horizontally on the physical page
    float physwidth =  PSM[ MediaSizeToPaper(CurrentMode->theMediaSize) ].fPhysicalPageX;
    float printable =  PSM[ MediaSizeToPaper(CurrentMode->theMediaSize) ].fPrintablePageX;
	return ((physwidth - printable) / 2.0 ); 
}

float PrintContext::PrintableStartY() // returned in inches
{ 
	if (thePrinter==NULL)
		return 0;
	return PSM[ MediaSizeToPaper(CurrentMode->theMediaSize) ].fPrintableStartY;
}


unsigned int PrintContext::printerunitsY()
// internal version
{ 
	if (thePrinter==NULL)
		return 0;
	return CurrentMode->ResolutionY[C]; 
}

///////////////////////////////////////////////////////////////////////////////////////

DRIVER_ERROR PrintContext::PerformPrinterFunction(PRINTER_FUNC eFunc)
{

#if defined(DEBUG) && (DBG_MASK & DBG_LVL1)
printf("PerformPrinterFunction:   eFunc = %d\n", eFunc);
#endif

	if (thePrinter==NULL)
        return NO_PRINTER_SELECTED;

	if (eFunc==CLEAN_PEN)
	{
printf("PerformPrinterFunction is CLEAN_PEN, flushing and calling CleanPen\n");
		thePrinter->Flush();
	    return thePrinter->CleanPen();
	}
		
	DBG1("PerformPrinterFunction: Unknown function\n");
	return UNSUPPORTED_FUNCTION;
				
}


	
PRINTER_TYPE PrintContext::EnumDevices(unsigned int& currIdx) const 
{ return DR->EnumDevices(currIdx); }



BOOL PrintContext::PrinterFontsAvailable(unsigned int PrintModeIndex)
{ 
#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
    PrintMode* pPM;
	if (thePrinter==NULL) 
        return SYSTEM_ERROR;	// called too soon
    pPM = thePrinter->GetMode(PrintModeIndex);
	return pPM->bFontCapable;
#else
    return FALSE;
#endif
}


DRIVER_ERROR PrintContext::SelectDevice(const PRINTER_TYPE Model)
// used by client when SystemServices couldn't get DevID
// this is the place where printer gets instantiated for unidi
{
#ifdef CAPTURE
Capture_SelectDevice(Model);
#endif
    DRIVER_ERROR err;

    if (thePrinter)     // if printer exists due to bidi or previous call
        delete thePrinter;

    err = DR->SelectDevice(Model);
    ERRCHECK;
   
    if ( (err = DR->InstantiatePrinter(thePrinter,pSS)) != NO_ERROR) 
	  { DBG1("PrintContext - error in InstantiatePrinter\n"); return err; }

	pSS->AdjustIO(thePrinter->IOMode);

    if (SelectDefaultMode())     
        SetWidths(); 
    else return WARN_MODE_MISMATCH;


 return NO_ERROR;
}

///////////////////////////////////////////////////////////////////////////

DRIVER_ERROR PrintContext::SetInputPixelsPerRow(unsigned int PixelsPerRow)
{
#ifdef CAPTURE
Capture_SetPixelsPerRow(PixelsPerRow);
#endif
    if (thePrinter)
      {
        if (InputRasterWidth==PixelsPerRow)
            return NO_ERROR;            // already set
        if (PixelsPerRow > PageWidth)
            return ILLEGAL_COORDS;         // or something

        // new value is legal
        InputRasterWidth=PixelsPerRow;
        if (InputRasterWidth==0)         // default setting
             // nothing else needs to change in this case
            return NO_ERROR;
        
      }
    else InputRasterWidth=PixelsPerRow;

    return NO_ERROR;
}

unsigned int PrintContext::EffectiveResolution()
{
    if (CurrentMode==NULL)
        return 0;

    return CurrentMode->BaseResX;
}

unsigned int PrintContext::GetModeCount()
{
  if (thePrinter==NULL)
      return 0;

  return thePrinter->GetModeCount();
}

DRIVER_ERROR PrintContext::SelectPrintMode(const unsigned int index)
{
#ifdef CAPTURE
Capture_SelectPrintMode(index);
#endif

    if (thePrinter==NULL)
        return NO_PRINTER_SELECTED;

    unsigned int count = GetModeCount();
    if (index > (count-1))
        return INDEX_OUT_OF_RANGE;

    CurrentMode= thePrinter->GetMode(index);
    CurrentModeIndex = index;

    if (InputRes > CurrentMode->BaseResX)
            return ILLEGAL_RESOLUTION;

    if (!ModeAgreesWithHardware(FALSE))
        return WARN_MODE_MISMATCH;
    // notice that requested mode is set even if it is wrong for the pen

    SetWidths();        // set InputRasterWidth, PageWidth

  return NO_ERROR;
}

DRIVER_ERROR PrintContext::SetPaperSize(PAPER_SIZE ps)
{ 
#ifdef CAPTURE
Capture_SetPaperSize(ps);
#endif
    if (CurrentMode==NULL)
        return NO_PRINTER_SELECTED;
    CurrentMode->theMediaSize= PaperToMediaSize( ps ); 
return NO_ERROR;
}

PAPER_SIZE PrintContext::GetPaperSize() 
{ 
    if (CurrentMode==NULL)
        return UNSUPPORTED_SIZE;
    return MediaSizeToPaper(CurrentMode->theMediaSize); 
}
    
PRINTER_TYPE PrintContext::SelectedDevice() 
{ 
    if (thePrinter==NULL) 
        return UNSUPPORTED; 
    return (PRINTER_TYPE)DR->device; 
}
     
const char* PrintContext::PrinterModel()
{
    if ((pSS==NULL) || (thePrinter==NULL))
        return (const char*)NULL;

    return pSS->strModel;
}

const char* PrintContext::PrintertypeToString(PRINTER_TYPE pt)
{
    return ModelName[pt];
}

//////////////////////////////////////////////////////////////
DRIVER_ERROR PrintContext::SendPrinterReadyData(BYTE* stream, unsigned int size)
{
    if (stream==NULL)
        return NULL_POINTER;

    if (thePrinter==NULL)
        return NO_PRINTER_SELECTED;

    return thePrinter->Send(stream,size);
}

void PrintContext::Flush(int FlushSize) { if(thePrinter != NULL) thePrinter->Flush(FlushSize); }
