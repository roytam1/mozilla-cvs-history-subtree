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

#ifdef _DJ9xx

#ifdef PROTO
#include "../imaging/resources.h"
#include "../include/Header.h"
#include "../include/IO_defs.h"
#include "../Printer/DJ895.h"
#include "../Printer/broadway.h"
extern int argProprietary;
#else
#include "Header.h"
#include "IO_defs.h"
#include "DJ895.h"
#include "broadway.h"
extern BYTE* GetHT3x3_4();
extern BYTE* GetHT6x6_4_970();
#endif

extern unsigned long ulMapBROADWAY_KCMY[ 9 * 9 * 9 ];
extern unsigned long ulMapBROADWAY_Gossimer_Normal_KCMY[ 9 * 9 * 9 ]; 


Broadway::Broadway(SystemServices* pSS, BOOL proto)
: DeskJet895(pSS,NUM_DJ6XX_FONTS,proto)
{
	bCheckForCancelButton = TRUE;

    ModeCount=3;

    if(pMode[DEFAULTMODE_INDEX] != NULL)
      delete pMode[DEFAULTMODE_INDEX];
    pMode[DEFAULTMODE_INDEX] = new BroadwayMode1();

    if(pMode[SPECIALMODE_INDEX] != NULL)
      delete pMode[SPECIALMODE_INDEX];
    pMode[SPECIALMODE_INDEX] = new BroadwayMode2();

    pMode[GRAYMODE_INDEX]->bFontCapable=TRUE;       // undo DJ895
}

Broadway::~Broadway()
{ }

BroadwayMode1::BroadwayMode1()
: PrintMode(ulMapBROADWAY_KCMY)
// 600x600x1 K
// 300x300x2 CMY
{
    
    ColorDepth[K]=1;  // 600x600x1 K          
    
    for (int i=1; i < 4; i++) 
        ColorDepth[i]=2;    // 300x300x2 CMY
          
    ResolutionX[K]=ResolutionY[K]=600;

    MixedRes = TRUE;

#ifdef PROTO
    if (!argProprietary)  
        ColorFEDTable = (BYTE*) HT300x3004level_open;
    else
        ColorFEDTable = (BYTE*) HT300x3004level_prop;
#else
    ColorFEDTable = GetHT3x3_4();
#endif
}

BroadwayMode2::BroadwayMode2()
: PrintMode(ulMapBROADWAY_Gossimer_Normal_KCMY) 
// 600x600x1 K
// 600x600x2 CMY
{
    int i;
    ColorDepth[K]=1;  // 600x600x1 K          
    
    for (i=1; i < 4; i++) 
        ColorDepth[i]=2;    // 300x300x2 CMY

    for (i=0; i < 4; i++) 
        ResolutionX[i]=ResolutionY[i]=600;

    BaseResX = BaseResY = 600;
    MixedRes = FALSE;

    medium = mediaGlossy;
    theMediaSize=sizePhoto;

#ifdef PROTO
    if (!argProprietary)
        ColorFEDTable = (BYTE*) HT600x6004level970_open;
    else 
        ColorFEDTable = (BYTE*) HT600x6004level970_prop;
#else
    ColorFEDTable = GetHT6x6_4_970();
#endif

    strcpy(ModeName, "Photo");
    bFontCapable=FALSE;
}

unsigned int Broadway::GetModeCount(void)
{
    if (!pSS->IOMode.bDevID || PhotoTrayInstalled(FALSE) )
        return 3;
    else return 2;
}

BOOL Broadway::UseGUIMode(unsigned int PrintModeIndex)
{
  return (PrintModeIndex==SPECIALMODE_INDEX);
}

Header900::Header900(Translator* t)
	: Header895(t)
{ }

Header* Broadway::SelectHeader(Translator* t)
{ 
	return new Header900(t); 
}

DRIVER_ERROR Header900::Send()
{
	DRIVER_ERROR err;
	//BOOL bDuplex = FALSE;

	StartSend();

	// this code will look for the duplexer enabled in the device ID and send the right
	// escape to the printer to enable duplexing.  At this time, however, we are not
	// going to support duplexing.  One, it is not supported with PCL3, which we need
	// for device font support.  Second, we don't have the resources to reformat the page
	// for book duplexing and can only do tablet.

    /*BYTE bDevIDBuff[DevIDBuffSize];
    err = theTranslator->pSS->ReadDeviceID(bDevIDBuff, DevIDBuffSize);
    ERRCHECK;

    // look for duplex code in bDevIDBuff
	Duplex = DuplexEnabled(bDevIDBuff);

	if(bDuplex)
    {
        err = thePrinter->Send((const BYTE*)EnableDuplex,sizeof(EnableDuplex));
	    ERRCHECK;
    }*/

	err = ConfigureRasterData();
	ERRCHECK;						
					
	err=Graphics();		// start raster graphics and set compression mode

	return err;
}   

BOOL Header900::DuplexEnabled(BYTE* bDevIDBuff)
{
	char* pStrVstatus = NULL;
	char* pStrDuplex = NULL;
	char* pStrSemicolon = NULL;

	if((pStrVstatus = strstr((char*)bDevIDBuff + 2,"VSTATUS:"))) 
		pStrVstatus += 8;
	else
		return FALSE;

	pStrDuplex = pStrVstatus;
	pStrSemicolon = pStrVstatus;

	// now parse VSTATUS parameters to find if we are in simplex or duplex
	if(!(pStrSemicolon = strstr((char*)pStrVstatus,";")))
		return FALSE;

	if(pStrDuplex = strstr((char*)pStrVstatus,"DP"))
		if(pStrDuplex < pStrSemicolon)
			return TRUE;
	if(pStrDuplex = strstr((char*)pStrVstatus,"SM"))
		if(pStrDuplex < pStrSemicolon)
			return FALSE;

	DBG1("didn't find SM or DP!!\n");
	return FALSE;
}

BOOL Broadway::ModeAllowable(unsigned int ModeIndex)
{ 
    BOOL special= (ModeIndex==SPECIALMODE_INDEX);
    BOOL installed = PhotoTrayInstalled(FALSE);
    
    return (special == installed);
}

BOOL Broadway::PhotoTrayInstalled(BOOL QueryPrinter)
{
    DRIVER_ERROR err;
	char* pStrVstatus = NULL;
	char* pStrPhotoTray = NULL;
	char* pStrSemicolon = NULL;

    BYTE bDevIDBuff[DevIDBuffSize];
    if (QueryPrinter)
    {
        err=pSS->ReadDeviceID(bDevIDBuff, DevIDBuffSize);
        if (err!=NO_ERROR)
            return FALSE;
    }
    else pSS->GetDeviceID(bDevIDBuff, DevIDBuffSize);
        

	if((pStrVstatus = strstr((char*)bDevIDBuff + 2,"VSTATUS:"))) 
		pStrVstatus += 8;
	else
		return FALSE;

	pStrPhotoTray = pStrVstatus;
	pStrSemicolon = pStrVstatus;

	// now parse VSTATUS parameters to find if we are in simplex or duplex
	if(!(pStrSemicolon = strstr((char*)pStrVstatus,";")))
		return FALSE;

	if(pStrPhotoTray = strstr((char*)pStrVstatus,"PH"))
		if(pStrPhotoTray < pStrSemicolon)
			return TRUE;
	if(pStrPhotoTray = strstr((char*)pStrVstatus,"NR"))
		if(pStrPhotoTray < pStrSemicolon)
			return FALSE;

	DBG1("didn't find PH or NR!!\n");
	return FALSE;
}


DISPLAY_STATUS Broadway::ParseError(BYTE status_reg)
{
	// check for Broadway-specific errors
	if(IOMode.bDevID)
    {
        if(PhotoTrayInstalled(TRUE))
	    {
		    DBG1("Broadway::ParseError, Photo tray installed\n");
		    return DISPLAY_ERROR_TRAP;
	    }
	}
	
	// check for errors in common with base class DJ895
	return DeskJet895::ParseError(status_reg);
}

#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
Font* Broadway::RealizeFont(const int index,const BYTE bSize,
						   const TEXTCOLOR eColor,
						   const BOOL bBold,const BOOL bItalic,
						   const BOOL bUnderline)

{ 
    
    return Printer::RealizeFont(index,bSize,eColor,bBold,bItalic,bUnderline);
}
#endif




#endif  // _DJ930 || _DJ950 || _DJ970