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

// The purpose of this file is to facilitate addition and subtraction
// of supported devices from the system.
#ifdef PROTO
#include "../include/Header.h"
#include "../Printer/DJ400.h"
#include "../Printer/DJ6XX.h"
#include "../Printer/DJ540.h"
#include "../Printer/DJ600.h"
#include "../Printer/DJ660.h"
#include "../Printer/DJ690.h"
#include "../Printer/DJ895.h"
#include "../Printer/broadway.h"
#else

#include "Header.h"
#ifdef _DJ400
#include "DJ400.h"
#endif
#if defined(_DJ6xx) || defined(_DJ6xxPhoto)
#include "DJ6XX.h"
#include "DJ660.h"
#include "DJ690.h"
#endif
#ifdef _DJ600
#include "DJ6XX.h"
#include "DJ600.h"
#endif
#ifdef _DJ540
#include "DJ6XX.h"
#include "DJ540.h"
#endif
#ifdef _DJ8xx
#include "DJ895.h"
#endif
#ifdef _DJ9xx
#include "broadway.h"
#endif
#endif   // end not PROTO


// Table of strings returned by printer firmware

char model400[]="HP DeskJet 4";
char model540[]="DESKJET 540";
char model600[]="DESKJET 600";
char model610[]="DESKJET 61";
char model640[]="DESKJET 64";
char model690[]="DESKJET 69";
char model660[]="DESKJET 66";
char model670[]="DESKJET 67";
char model680[]="DESKJET 68";
char model6xx[]="DESKJET 6";
char model810[]="DESKJET 81";
char model830[]="DESKJET 83";
char model840[]="DESKJET 84";
char model880[]="DESKJET 88";
char model895[]="DESKJET 895";
char model930[]="DESKJET 93";
char model950[]="DESKJET 95";
char model970[]="DESKJET 97";


PRINTER_TYPE BuiltIn[]={
#ifdef _DJ400
    DJ400,
#endif
#ifdef _DJ540
    DJ540,
#endif
#ifdef _DJ600
    DJ600,
#endif
#ifdef _DJ6xx
    DJ6xx,
#endif
#ifdef _DJ6xxPhoto
    DJ6xxPhoto,
#endif
#ifdef _DJ8xx
    DJ8xx,
#endif
#ifdef _DJ9xx
    DJ9xx 
#endif
};

PRINTER_TYPE DevIDtoPT(unsigned int StringIndex)
{
    if (StringIndex<3)
        return (PRINTER_TYPE)StringIndex;
    
    if (StringIndex<6)
        return DJ6xxPhoto;

    if (StringIndex<10)
        return DJ6xx;

    if (StringIndex<15)
        return DJ8xx;

        return DJ9xx;

}


DeviceRegistry::DeviceRegistry()
	: device(UNSUPPORTED)
{

	ModelString[0]=model400;
	ModelString[1]=model540;
	ModelString[2]=model600;

	ModelString[3]=model610;
	ModelString[4]=model640;
    ModelString[5]=model690;

	ModelString[6]=model660;
	ModelString[7]=model670;
	ModelString[8]=model680;
    ModelString[9]=model6xx;
	
	ModelString[10]=model810;
	ModelString[11]=model830;
	ModelString[12]=model840;
	ModelString[13]=model880;
	ModelString[14]=model895;

	ModelString[15]=model930;
	ModelString[16]=model950;
	ModelString[17]=model970;

    DBG1("DeviceRegistry created\n");
}

DeviceRegistry::~DeviceRegistry()
{
    DBG1("deleting DeviceRegistry\n");
}

DRIVER_ERROR DeviceRegistry::SelectDevice(const PRINTER_TYPE Model) 
{ 
    if (Model > MAX_PRINTER_TYPE)
        return UNSUPPORTED_PRINTER;
    device = Model; 

 return NO_ERROR;
}


DRIVER_ERROR DeviceRegistry::SelectDevice(char* model, char* pens, SystemServices* pSS)
// used by PrintContext constructor
// based on this 'model' string, we will search for the enum'd value
// and set this enum'd value in 'device'
{

#if defined(DEBUG) && (DBG_MASK & DBG_LVL1)
    printf("DR::SelectDevice: model= '%s'\n",model);
    printf("DR::SelectDevice: pens= '%s'\n",pens);
#endif

	unsigned int i=0; // counter for model type
    unsigned int j=0; // counter for string parsing
    char pen1 = '\0';   // black/color(for CCM)/photo(for 690) pen
    char pen2 = '\0';   // color/non-existent(for CCM) pen

    BOOL match=FALSE;

    DRIVER_ERROR err = NO_ERROR;

    while (!match && (i < MAX_ID_STRING)) 
    {
        if (! strncmp(model,ModelString[i],strlen(ModelString[i])) ) 
			match=TRUE;
        else 
            i++;
    }

    if (i >= MAX_ID_STRING)
    {
    // The devID model string did not have a match for a known printer
    //  so let's look at the pen info for clues

        // if we don't have pen info (VSTATUS) it's presumably
        //  either sleek, DJ4xx or non-HP
        if ( pens[0] != '\0' )
        {
            // Venice (and Broadway?) printers return penID $X0$X0
            //  when powered off
            if(pens[1] == 'X')
            {
                DBG1("DR:(Unknown Model) Need to do a POWER ON to get penIDs\n");
                
                WORD length=sizeof(Venice_Power_On);
                err = pSS->ToDevice(Venice_Power_On, &length);
                ERRCHECK;

                err = pSS->FlushIO();
                ERRCHECK;

                // give the printer some time to power up
                if (pSS->BusyWait((DWORD)1000) == JOB_CANCELED)
                return JOB_CANCELED;

                // we must re-query the devID
                err=GetPrinterModel(model,pens,pSS);
                ERRCHECK;
            }

            // Arggghh.  The pen(s) COULD be missing
            do
            {
                // get pen1 - penID format is $HB0$FC0
                pen1=pens[1];
            
	            // get pen2 - if it exists
	            j=2;
                BOOL NO_PEN2 = FALSE;
	            while(pens[j] != '$')   // handles variable length penIDs
                {
                    j++; 
                    if ( pens[j] == '\0' )
                    // never found a pen2
                    {
                        pen2 = '\0';
                        NO_PEN2 = TRUE;
                        break;
                    }
                }
                if (NO_PEN2 == FALSE)
                {
                    j++;
                    pen2 = pens[j];
                }

                if(pen1 == 'A' || pen2 == 'A')
                {
                    if(pen1 == 'A')
                    {
                        // 2-pen printer with both pens missing
                        if(pen2 == 'A')
                            pSS->DisplayPrinterStatus(DISPLAY_NO_PENS);

                        // 1-pen printer with missing pen
                        else if(pen2 == '\0')
                            pSS->DisplayPrinterStatus(DISPLAY_NO_PEN_DJ600);

                        // 2-pen printer with BLACK missing
                        else pSS->DisplayPrinterStatus(DISPLAY_NO_BLACK_PEN);
                    }
                    // 2-pen printer with COLOR missing
                    else if(pen2 == 'A')
                            pSS->DisplayPrinterStatus(DISPLAY_NO_COLOR_PEN);

                    if (pSS->BusyWait(500) == JOB_CANCELED)
                        return  JOB_CANCELED;

                    // we must re-query the devID
                    err=GetPrinterModel(model,pens,pSS);
                    ERRCHECK;
                }

            } while(pen1 == 'A' || pen2 == 'A');

            // now that we have pens to look at, let's do the logic
            //  to instantiate the 'best-fit' driver

            if(pen1 == 'H') // Hobbes (BLACK)
            {
                // check for a 850/855/870
                if(pen2 == 'M') device=UNSUPPORTED; // Monet (COLOR)

                else if (strncmp(model,"DESKJET 890",11) == 0)
                    device=UNSUPPORTED; // 890 has same pens as Venice!

                else if(pen2 == 'N') device=DJ9xx; // Chinook (COLOR)

                // It must be a Venice derivative or will hopefully at
                // least recognize a Venice print mode
                else device=DJ8xx;
            }
            else if(pen1 == 'C') // Candide (BLACK)
            {
                // check for 1-pen printer
                if(pen2 == '\0') device=DJ600;
                // must be a 2-pen 6xx-derivative
                else device=DJ6xx;
            }
            else if(pen1 == 'M') // Multi-dye load
            {
                // must be a 690-derivative
                device=DJ6xxPhoto;
            }

            // check for 540-style pens?
            //  D = Kukla color, E = Triad black

            else device=UNSUPPORTED;
        }
    }
    // we found a match for the model string
    else
        device = DevIDtoPT(i);

    // Early Venice printer do not yet have full bi-di so check
    // the model to avoid a communication problem.
    if ( ( (strncmp(model,"DESKJET 81",10) == 0)
        || (strncmp(model,"DESKJET 83",10) == 0)
        || (strncmp(model,"DESKJET 88",10) == 0)
        || (strncmp(model,"DESKJET 895",11) == 0) )
        && (pSS->IOMode.bUSB) )
    {
        DBG1("This printer has limited USB status\n");
        pSS->IOMode.bStatus = FALSE;
        pSS->IOMode.bDevID = FALSE;
    }


    if (device == UNSUPPORTED) return UNSUPPORTED_PRINTER;
    else return NO_ERROR;
}

DRIVER_ERROR DeviceRegistry::InstantiatePrinter(Printer*& p, SystemServices* pSS)
// Instantiate a printer object and return a pointer p based on the previously
// set 'device' variable
{ 
    p=NULL;

DBG1("DR::InstantiateGeneral: device= ");

    switch(device) {

#ifdef _DJ400
	    case(DJ400): 
                     p=new DeskJet400(pSS);
                     DBG1("DJ400\n");
                     break;
#endif

#ifdef _DJ540
	    case(DJ540): 
                     p=new DeskJet540(pSS);
                     DBG1("DJ540\n");
                     break;
#endif

#ifdef _DJ600
	    case(DJ600): 
                     p=new DeskJet600(pSS);
                     DBG1("DJ600\n");
                     break;
#endif

#ifdef _DJ6xx
	    case(DJ6xx):
                     p=new DeskJet660(pSS);
                     DBG1("DJ6xx\n");
                     break;
#endif

#ifdef _DJ6xxPhoto
        case(DJ6xxPhoto):
                     p=new DeskJet690(pSS);
                     DBG1("DJ6xxPhoto\n");
                     break;
#endif
 

#ifdef _DJ8xx
        case(DJ8xx):
                     p=new DeskJet895(pSS);
                     DBG1("DJ8xx\n");
                     break;
#endif

#ifdef _DJ9xx
            case(DJ9xx):
                     p=new Broadway(pSS);
                     DBG1("DJ9xx\n");
                     break;
#endif
        default:     
                     return UNSUPPORTED_PRINTER;
                     DBG1("Unsupported\n");
                     break;
    }

    NEWCHECK(p);

    return p->constructor_error;
}



DRIVER_ERROR DeviceRegistry::GetPrinterModel(char* strModel, char* strPens, SystemServices* pSS)
{	
	DRIVER_ERROR err;
	BYTE DevIDBuffer[DevIDBuffSize];

	err = pSS->ReadDeviceID(DevIDBuffer, DevIDBuffSize);
	ERRCHECK;   // should be either NO_ERROR or BAD_DEVICE_ID

    return ParseDevIDString((const char*)DevIDBuffer, strModel, strPens);

}

DRIVER_ERROR DeviceRegistry::ParseDevIDString(const char* sDevID, char* strModel, char* strPens)
{
    int i;	// simple counter
	char* pStr = NULL;	// string pointer used in parsing DevID

	// get the model name
    // - note: I'm setting pStr to the return of strstr
    //   so I need to increment past my search string
	if ( (pStr = strstr(sDevID+2,"MODEL:")) )
		pStr+=6;
	else
		if ( (pStr=strstr(sDevID+2,"MDL:")) )
			pStr+=4;
		else return BAD_DEVICE_ID;


	// my own version of strtok to pull out the model string here
	i = 0;
	while ( (pStr[i] != ';') && (pStr[i] != '\0') )
		strModel[i] = pStr[i++];
	strModel[i] = '\0';

	// now get the pen info
    if( (pStr=strstr(sDevID+2,"VSTATUS:")) ) 
      {
        pStr+=8;
        i=0;
	    while ( (pStr[i] != ',') && (pStr[i] != ';') && (pStr[i] != '\0') )
		    strPens[i] = pStr[i++];
	    strPens[i] = '\0';
      }
	else   // no VSTATUS for 400 and sleek printers
        strPens[0] = '\0';

	return NO_ERROR;
}

PRINTER_TYPE DeviceRegistry::EnumDevices(unsigned int& currIdx) const
// Returns next model; UNSUPPORTED when finished
{  
    PRINTER_TYPE pt;
    int k= sizeof(BuiltIn) / sizeof(PRINTER_TYPE);

	if (currIdx >=  k)
		return UNSUPPORTED;
                   
    pt = BuiltIn[currIdx];

    currIdx++;
	
	return pt;
}
