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
#include "../imaging/imager.h"
#else
#include "Header.h"
#include "imager.h"
#endif


extern BYTE EscCopy(BYTE *dest,const char *s,int num, char end);

//////////////////////////////////////////////////////////////////////////////////////
// Translator design
//
// The Translator object handles all printer-command-language setup and
// formatting of data. In theory, the driver can be converted from PCL
// to JetSend or PostScript or whatever, just by replacing the Translator
// and its subcomponents.
// The components are:
//	1. the Header	-- commands sent to the printer for initialization and
//		configuration, prior to graphical or text data; the Header is
//		specific to the printer model and mode.
//  2. the GraphicsTranslator	-- responsible for formatting all raster data
//		(it in turn may have a Compressor component)
//  3. the TextTranslator	-- responsible for formatting all ASCII data
//
// The idea of separating out graphics and text components is that it is possible
// to have only graphics or only ASCII data for a given Job, in which case the
// unused component may remain NULL in the Translator container. As a result,
// the Translator is created in two stages: first create the container class,
// then install either or both of the subcomponents. All this is handled
// by the Job.
//
// Translator constructor just copies pointers to SystemServices, PrintContext
// and Printer (which have already been validated by Job), initializes two
// other variables and creates the Header object, which is returned by the
// printer class.
Translator::Translator(SystemServices* pSys, Printer* pP, PrintContext* pPC)
	: pSS(pSys), thePrinter(pP), thePrintContext(pPC)
{ 
	// Set the 2 optional components to NULL, awaiting determination by Job 
	GT=NULL;

#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
	TT=NULL;
	lastname[0]='\0';		// init variable used to check for changed typeface
#endif

	// install the appropriate Header for the Printer;
	// when mode parameters come into play, they will be passed along from
	// the PrintContext at this point
	theHeader=thePrinter->SelectHeader(this);
	if (theHeader==NULL)
		constructor_error=SYSTEM_ERROR;
	else constructor_error=NO_ERROR;

}

Translator::~Translator()
{
	if (GT) delete GT;
#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
	if (TT) delete TT;
#endif

	if (theHeader) delete theHeader;

}

// routines to complete Translator construction, called by Job constructor
DRIVER_ERROR Translator::InstallGraphicsTranslator(Imager* pU, unsigned int iRasterWidth, BOOL bCompress)
{

	GT=new GraphicsTranslator(pSS, thePrinter, thePrintContext, pU, iRasterWidth, bCompress);

	if (GT==NULL)
		return ALLOCMEM_ERROR;
	return GT->constructor_error; 
}


/////////////////////////////////////////////////////////////////////////////////
// GRAPHICS SECTION

// GraphicsTranslator constructor
// sets up various internal variables and buffers,
// and creates the Compressor object based on the printer.
GraphicsTranslator::GraphicsTranslator(SystemServices* pSys,Printer* pP, PrintContext* pPC,
										Imager* pU, 
										unsigned int iRasterWidth,	// in pixels
										BOOL bCompress)
	: pSS(pSys), thePrinter(pP), thePrintContext(pPC), theImager(pU),
	  compress(bCompress),		// hardwired for now
	  SeedRaster(NULL),
	  dyeCount(pPC->CurrentMode->dyeCount), RasterWidth(iRasterWidth)
{ 
 constructor_error= NO_ERROR;

 unsigned int MaxRows=0;
 unsigned int MaxWidth=0;
 MaxColorDepth=0;			// used in connection with SeedRaster
 for (int i=0; i < MAXCOLORPLANES; i++)
   {
	 ResolutionX[i] = thePrintContext->CurrentMode->ResolutionX[i];
	 if (theImager->ColorDepth[i] > MaxColorDepth)
		MaxColorDepth=theImager->ColorDepth[i];
     if (theImager->NumRows[i] > MaxRows)
         MaxRows = theImager->NumRows[i];
     if (theImager->OutputWidth[i] > MaxWidth)
         MaxWidth = theImager->OutputWidth[i];
   }

 guiMode = thePrinter->UseGUIMode(thePrintContext->CurrentPrintMode());

 // pre-calculate size of a raster -- 
 // needed for SeedRaster allocation and compression buffer
	RasterSize = MaxColorDepth		
                * MaxRows
				* dyeCount			// number of colors
				* MaxWidth;		// pixels to output

 if (compress)
    {
		theCompressor = thePrinter->CreateCompressor(RasterSize,MaxWidth);
		if (theCompressor==NULL)
			compress=FALSE;
		else constructor_error=theCompressor->constructor_error;
		CERRCHECK;
		
		if (theCompressor->UseSeedRow)
		  {
			SeedRaster=(char*)pSS->AllocMem(RasterSize);
			 if (SeedRaster==NULL)
			   {
				 constructor_error=ALLOCMEM_ERROR;
				 return;
			   }
			 memset(SeedRaster,0,RasterSize);
		  }
    }

////////////////
#ifdef USAGE_LOG
	page=0;
	for (int j=0; j<MAXPAGES; j++)
		for (int k=0; k<MAXDYES; k++)
			DotCount[j][k]=0;	
#endif
////////////////
}


GraphicsTranslator::~GraphicsTranslator()
{ 
	if (SeedRaster)					
		pSS->FreeMem((BYTE*)SeedRaster);
	
	if (compress)
		delete theCompressor;
}


////////////////////////////////////////////////////////////////////////
// This is the main routine for PCL encapsulation of graphics data.
//
DRIVER_ERROR GraphicsTranslator::SendRaster()
// Sends graphics data to the printer for all planes in this raster.
// Data comes from the Imager object -- it sends <Imager::PlaneSize> bytes
// for each <Imager::ColorDepth> layer of each <dyeCount>, starting from
// Imager::StartPlane. The data is copied from Imager::ColorPlane[color][level].
// If compression is being used, theCompressor is invoked to produce the
// final data-stream for the printer.
// Each color-level's worth of data is preceded by the ESC*b#V or ESC*b#W
// command.
//
// Calls: ResetSeedRaster, theCompressor::Compress(), thePrinter::Send().
//
// Returns errorcode from thePrinter::Send().
{ 
 char scratch[20];
 DRIVER_ERROR err;		
  
 int start=theImager->StartPlane;
   // for each color...
  for ( int iColor = start; iColor < (dyeCount+start); iColor++ )
	{		
#ifdef USAGE_LOG
	  CountDots(iColor);
#endif	
    // for each row of a higher-res color (relative to the base resolution)
    for (int iRow=0; iRow < theImager->NumRows[iColor]; iRow++)

	 // for each plane of a multi-level color...
	 for ( int iPlane = 0; iPlane < theImager->ColorDepth[iColor]; ++iPlane )
		{
			unsigned int planeSize;
			BYTE *currRasterPtr;
							
			currRasterPtr = theImager->ColorPlane[iColor][iRow][iPlane];			
            // output size equals one bit per pixel
			planeSize = 
                theImager->OutputWidth[iColor]/8 +(theImager->OutputWidth[iColor] % 8);		

            if (compress)	
			 {				
				if (theCompressor->UseSeedRow)
					theCompressor->SetSeedRow(
						&SeedRaster[ (iColor-start)*MaxColorDepth*RasterWidth + 
                                     iPlane*RasterWidth] 
                                              );
				  
				err = theCompressor->Compress((const char*)currRasterPtr, planeSize );
				ERRCHECK;

				currRasterPtr = theCompressor->compressBuf;					
			 }
				
			
			// Check for last PCL plane
			int scratchLen;
			if ((iColor == (dyeCount+start - 1)) && 
				(iPlane == theImager->ColorDepth[iColor] - 1))					
				scratchLen = sprintf(scratch, "%c%c%c%d%c", '\033', '*', 'b',
									 planeSize, 'W');											
			else					
			    scratchLen = sprintf(scratch, "%c%c%c%d%c", '\033', '*', 'b', 
									 planeSize, 'V');											
							
			err = thePrinter->Send((const BYTE*)scratch,scratchLen);
			ERRCHECK;
				
			err = thePrinter->Send(currRasterPtr,planeSize);
			ERRCHECK;
			
			
		} // for iPlane
	} // for iColor
 
  if (compress)
	  if (theCompressor->UseSeedRow)
		ResetSeedRaster();

  return NO_ERROR;
}

void GraphicsTranslator::ResetSeedRaster()
// SeedRaster is used by PCL Mode9 compression in GraphicsTranslator::SendRaster().
// It represents the previous raster's data.
// Data comes from Imager colorplanes.
{
 ASSERT(SeedRaster!=NULL);

 memset(SeedRaster,0,RasterSize);

 unsigned int start=theImager->StartPlane;

 for (unsigned int iColor=start; iColor < theImager->ColorPlaneCount + start; iColor++)
    for (unsigned int iRow=0; iRow < theImager->NumRows[iColor]; iRow++)
	    for (unsigned int j=0;j < theImager->ColorDepth[iColor]; j++)
	        memcpy(&SeedRaster[(iColor-start)*MaxColorDepth*RasterWidth + j*RasterWidth], 
				    theImager->ColorPlane[iColor][iRow][j], 
                    theImager->OutputWidth[iColor]/8 +
                        (theImager->OutputWidth[iColor]%8)
                    );

}

// Color planes are ordered KCMYClMl (K=0,C=1,etc.)
// In case there is no K data, the first plane is C=1.
// Used in setting up CRD in Header.
unsigned int Translator::FirstColorPlane()
{ 
	if (GT==NULL)
		return 0;	// just for safety
	return GT->theImager->StartPlane; 
}

unsigned int Translator::ColorLevels(unsigned int ColorPlane) 
{
    // convert from number of bits to levels
	int bits=thePrintContext->CurrentMode->ColorDepth[ColorPlane];
	int res=1;
	for (int i=0; i<bits; i++)
		res=res*2;
	
	return  res;
}


////////////////////////////////////////////////////////////////////////
// Miscellaneous

		  
DRIVER_ERROR Translator::FormFeed()
{
	BYTE FF=12;
	return thePrinter->Send((const BYTE*)&FF,1);
}

DRIVER_ERROR Translator::SendCAP(unsigned int iAbsX, unsigned int iAbsY)
{ int len; BYTE temp[20];

	len= EscCopy(temp,"*p",iAbsX,'X');
	len += EscCopy(&temp[len],"*p",iAbsY,'Y');
	 
 return thePrinter->Send( temp, len);
}

DRIVER_ERROR Translator::SendCAPy(unsigned int iAbsY)
{ int len; BYTE temp[10];

	len= EscCopy(temp,"*p",iAbsY,'Y');
	
 return thePrinter->Send( temp, len);
} 

///////////////////////////////////////////////////////////////////
#ifdef USAGE_LOG
void GraphicsTranslator::PrintDotCount(char* str)
{
    FILE* fp = fopen("PUMP.TXT","wa");
	fprintf(fp,"Dot counts for page#%d, <%s>:\n",page+1,str);
	fprintf(fp,"  K : %d\n",DotCount[page][K]);
	fprintf(fp,"  C : %d\n",DotCount[page][C]);
	fprintf(fp,"  M : %d\n",DotCount[page][M]);
	fprintf(fp,"  Y : %d\n",DotCount[page][Y]);
	if (DotCount[page][Clight])
		fprintf(fp,"  Cl : %d\n",DotCount[page][Clight]);
	if (DotCount[page][Mlight])
		fprintf(fp,"  Ml : %d\n",DotCount[page][Mlight]);
    fclose(fp);

}

void GraphicsTranslator::PrintDotTotals()
{
	int k,c,m,y,cl,ml;
	k=c=m=y=cl=ml=0;
    FILE* fp = fopen("PUMP.TXT","aa");
	fprintf(fp,"Totals for job:\n");
	for (int i=0;i<page;i++)
	{
		k += DotCount[i][K];
		c += DotCount[i][C];
		m += DotCount[i][M];
		y += DotCount[i][Y];
		cl += DotCount[i][Clight];
		ml += DotCount[i][Mlight];
	}

	fprintf(fp,"  K : %d\n",k);
	fprintf(fp,"  C : %d\n",c);
	fprintf(fp,"  M : %d\n",m);
	fprintf(fp,"  Y : %d\n",y);
	if (DotCount[page][Clight])
		fprintf(fp,"  Cl : %d\n",cl);
	if (DotCount[page][Mlight])
		fprintf(fp,"  Ml : %d\n",ml);
    fclose(fp);

}

BOOL GraphicsTranslator::NextPage()
{ 
	if (page<MAXPAGES) 
		{ page++; return TRUE; } 
	else return FALSE; 
}


void GraphicsTranslator::CountDots(unsigned int dye)
{
    BYTE* p; BYTE* p2;
    unsigned int planeSize = 
                theImager->OutputWidth[dye]/8 +(theImager->OutputWidth[dye] % 8);

    for (int iRow=0; iRow < theImager->NumRows[dye]; iRow++)
    {
        if (theImager->ColorDepth[dye]==1)      // binary colorlevel case
          {
	        p = theImager->ColorPlane[dye][iRow][0];
	        for (int i=0; i < planeSize; i++)
	          {
		        BYTE b = *p++;
		        for (int j=0; j<8; j++)
		          {
			        if (b % 2)
                        DotCount[page][dye]++;
			        b = b >> 1;
		          }
	          }
          }
        else    // special code for hifipe
                // uses knowledge of Venice mapping from levels to dots
         {
            p = theImager->ColorPlane[dye][iRow][0];
            p2 = theImager->ColorPlane[dye][iRow][1];
            for (int i=0; i < planeSize; i++)
              {
                BYTE b = *p++; BYTE b2 = *p2++;
                for (int j=0; j<8; j++)
		          {
			        if (b2 % 2)     // high bit
                        if (b % 2)      // level 3 = 4 drops                   
                            DotCount[page][dye]+=4;
                        else            // level 2 = 2 drops
                            DotCount[page][dye]+=2;
                    else
                        if (b % 2)      // level 1 = 1 drop
                            DotCount[page][dye]++;

			        b = b >> 1;
                    b2 = b2 >> 1;
		          }
              }
         }
    }
}

#endif