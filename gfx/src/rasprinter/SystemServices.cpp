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

// SystemServices
//
// Contains constructor, destructor and higher-level members
// of the class.
//
// The SystemServices object is used to encapsulate
// memory-management, I/O, and other utilities provided by
// the operating system or host system supporting the driver.
// It is an abstract base class, requiring implementation of
// the necessary routines by the host.
//
// Creation of SystemServices is the first step in the calling
// sequence for the driver, followed by creation of the PrintContext
// and the Job.
// The derived class constructor must include a call to the member
// function InitDeviceComm, which establishes communication with
// the printer if possible.
//
#ifdef PROTO
#include "../include/Header.h"
#include "../include/IO_defs.h"
#else
#include "Header.h"
#include "IO_defs.h"
#endif

// Constructor instantiates the DeviceRegistry.
// Real work done in InitDeviceComm, called from derived
// class constructor.
SystemServices::SystemServices()
    : constructor_error(NO_ERROR)
#ifdef CAPTURE
    , Capturing(FALSE), pScripter(NULL)
#endif
{

    strModel[0] = strPens[0] = '\0';

#ifdef PROTO
	DR = new ProtoRegistry();     
#else
	DR = new DeviceRegistry();
#endif
	// DR can't fail

	IOMode.bDevID=FALSE;
	IOMode.bStatus=FALSE;
    IOMode.bUSB=FALSE;

}

SystemServices::~SystemServices()
{ 

    delete DR;

#if defined(CAPTURE) || defined(PROTO)
    if (pScripter)
        delete pScripter;
#endif

DBG1("deleting SystemServices\n");
}

///////////////////////////////////////////////////////////////////
// Function to determine whether printer is responsive.
// Calls host-supplied GetStatusInfo
//
// NOTE: This implementation is appropriate for Parallel bus only.
//
BOOL SystemServices::PrinterIsAlive()
{
	BYTE status_reg;
    GetStatusInfo(&status_reg);

#define DJ6XX_OFF		(0xF8)
#define DJ400_OFF		(0xC0)
// sometimes the DJ400 reports a status byte of C8 when it's turned off
#define DJ400_OFF_BOGUS	(0xC8)
#define DEVICE_IS_OK(reg) (!((reg == DJ6XX_OFF) || (reg == DJ400_OFF) || (reg == DJ400_OFF_BOGUS)))

#if defined(DEBUG) && (DBG_MASK & DBG_LVL1)
	printf("status reg is 0x%02x\n",status_reg);
	if (DEVICE_IS_OK(status_reg))
		DBG1("PrinterIsAlive: returning TRUE\n");
	else
		DBG1("PrinterIsAlive: returning FALSE\n");
#endif

	return (DEVICE_IS_OK(status_reg));
}

// Same as host-supplied FreeMem, with extra safety check
// for null pointer.
DRIVER_ERROR SystemServices::FreeMemory(void *ptr)
{
	if (ptr == NULL)
		return SYSTEM_ERROR;

//		printf("FreeMemory freeing %p\n",ptr);
	FreeMem((BYTE*)ptr);

 return NO_ERROR;
}

DRIVER_ERROR SystemServices::GetDeviceID(BYTE* strID, int iSize)
// for use when string doesn't have to be re-fetched from printer
{
    if (DevIDBuffSize > iSize)
		return SYSTEM_ERROR;
 //   strcpy((char*)strID, (const char*)strDevID);
// sometimes there's a zero near the beginning; this way may copy more bytes than needed,
// but at least it gets them all, and doesn't overflow buffer
    for (int i=0; i<iSize; i++)
        strID[i] = strDevID[i];

  return NO_ERROR;
}

////////////////////////////////////////////////////////////////////
DRIVER_ERROR SystemServices::InitDeviceComm()
// Must be called from derived class constructor.
// (Base class must be constructed before system calls
//  below can be made.)
// Opens the port, looks for printer and 
// dialogues with user if none found;
// then attempts to read and parse device ID string --
// if successful, sets IOMode.bDevID to TRUE (strings stored
// for retrieval by PrintContext).
// Returns an error only if user cancelled. Otherwise
// no error even if unidi.
//
// Calls: OpenPort,PrinterIsAlive,DisplayPrinterStatus,BusyWait,
//   ReadDeviceID,DeviceRegistry::ParseDevIDString.
// Sets:    hPort,IOMode, strModel, strPens
{
    DRIVER_ERROR err = NO_ERROR;
	BOOL bPrinterIsAlive = TRUE;
    BOOL ErrorDisplayed = FALSE;

    // make sure a printer is there, turned on and connected
	// before we go any further.  This takes some additional checking
	// due to the fact that the 895 returns a status byte of F8 when
	// it's out of paper, the same as a 600 when it's turned off.
	if(PrinterIsAlive() == FALSE)
	{
		bPrinterIsAlive = FALSE;
	}
	else IOMode.bStatus=TRUE;

	// Now we have to see if we've got a printer that's really turned
	// off or an 895 that's out of paper and hasn't been reset or loaded
	// with paper
	err = ReadDeviceID(strDevID, DevIDBuffSize);

	// ReadDeviceID returns BAD_DEVICE_ID if it fails
	if(err != NO_ERROR && bPrinterIsAlive == FALSE)
	{
		// Printer is turned off
		while(PrinterIsAlive() == FALSE)
		{
			DBG1("PrinterIsAlive returned FALSE\n");
			ErrorDisplayed = TRUE;
			DisplayPrinterStatus(DISPLAY_NO_PRINTER_FOUND);

			if(BusyWait(500) == JOB_CANCELED)
				return JOB_CANCELED;
		}
		if(ErrorDisplayed == TRUE)
		{
			DisplayPrinterStatus(DISPLAY_PRINTING);
			// if they just turned on/connected the printer,
			// delay a bit to let it initialize
			if(BusyWait(2000) == JOB_CANCELED)
				return JOB_CANCELED;
			else
				IOMode.bStatus = TRUE;
			err = ReadDeviceID(strDevID, DevIDBuffSize);
		}
	}
	else
		IOMode.bStatus = TRUE;
		
	// now try to identify printer
	if (err!=NO_ERROR)
        return NO_ERROR;    // proceed along unidi path

    err = DR->ParseDevIDString((const char*)strDevID, strModel, strPens);
    if (err==NO_ERROR)
        IOMode.bDevID=TRUE;

	return NO_ERROR;
}


// This function is only relevant for DeskJet 400 support and is therefore
// basically 'stubbed out' since the DJ400 is now supported only on some
// grandfather-ed systems.  Systems that DO support the DJ400 MUST implement
// this function in their derived SystemServices class.
DRIVER_ERROR SystemServices::GetECPStatus(BYTE *pStatusString,int *pECPLength, int ECPChannel)
{
    pStatusString = NULL;
    *pECPLength = 0;

    return UNSUPPORTED_FUNCTION;
}
