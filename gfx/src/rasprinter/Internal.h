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

#ifndef INTERNAL_H
#define INTERNAL_H

/////////////////////////////////////////////////////////////////////////
// contains all class declarations
// for Slimhost++ driver
//
// merged in from file "Objects.h" 5/18/98
/////////////////////////////////////////////////////////////////////////
// these correspond to PCL codes
typedef int TYPEFACE;
#define COURIER 3
#define LETTERGOTHIC 6
#define CGTIMES 4101
#define UNIVERS 52

// character set names for PCL
#define LATIN1 "0N"		// aka ECMA94
#define PC8 "10U"
#define HP_LEGAL "1U"

#define MAXDYES 6
#define MAXCOLORDEPTH 3
#define MAXCOLORPLANES	6	// current max anticipated, 6 for 690 photopen
#define MAXCOLORROWS 2      // multiple of high-to-low for mixed-resolution cases

// used to encourage consistent ordering of color planes
#define K	0
#define C	1
#define M	2
#define Y	3
#define Clight	4
#define Mlight	5

#define RANDSEED 77


#define DEFAULT_SLOW_POLL_COUNT 30
#define DEFAULT_SLOW_POLL_BIDI 3

//////////////////////////////////////////

enum STYLE_TYPE { UPRIGHT, ITALIC };

enum WEIGHT_TYPE { NORMAL, BOLD };


///////////////////////////////////////////////////////////////////

#define MAX_ESC_SEQ 40
#define MAX_RASTERSIZE	10000	// REVISIT


// very frequently used fragments made into macros for readability
#define CERRCHECK if (constructor_error != NO_ERROR) {DBG1("CERRCHECK fired\n"); return;}
#define ERRCHECK if (err != NO_ERROR) {DBG1("ERRCHECK fired\n"); return err;}
#define NEWCHECK(x) if (x==NULL) return ALLOCMEM_ERROR;
#define CNEWCHECK(x) if (x==NULL) { constructor_error=ALLOCMEM_ERROR; return; }



//////// STATIC DATA ////////////////////////////////////////////////////////////////
// escape sequences -- see PCL Implementor's Guide or Software Developer's PCL Guides
// for documentation
#define ESC 0x1b

const char UEL[] = {ESC, '%', '-','1','2','3','4','5','X' };
const char EnterLanguage[] = {'@','P','J','L',' ','E','N','T','E','R',' ',
						'L','A','N','G','U','A','G','E','=' };
const char PCL3[] = {'P','C','L','3' };
const char PCLGUI[] = {'P','C','L','3','G','U','I' };					
const char Reset[] = {ESC,'E'};
const char crdStart[] =	{ESC, '*', 'g'};			// configure raster data command
const char crdFormat = 2; // only format for 600
const char grafStart[] = {ESC, '*', 'r', '1', 'A'};	// raster graphics mode
const char grafMode0[] = {ESC, '*', 'b', '0', 'M'};	// compression methods
const char grafMode9[] =	{ESC, '*', 'b', '9', 'M'};
const char grafMode2[] =	{ESC, '*', 'b', '2', 'M'};
const char SeedSame[] =	{ESC, '*', 'b', '0', 'S'};
//const char EjectPage[] = {ESC, '&', 'l', '0', 'H'};	// not needed by us; will pick if no page already picked
const char BlackExtractOff[] = {ESC, '*', 'o', '5', 'W', 0x04, 0xC, 0, 0, 0 };
const char LF = '\012';
const BYTE Venice_Power_On[] = {ESC, '%','P','u','i','f','p','.',
        'p','o','w','e','r',' ','1',';',
        'u','d','w','.','q','u','i','t',';',ESC,'%','-','1','2','3','4','5','X' };
/*const BYTE Venice_Pre_Pick[] = {ESC, '&', 'l', -2, 'H'};
{ESC, '%','P','m','e','c','h','.',
        'l','o','a','d','_','p','a','p','e','r',';',
        'u','d','w','.','q','u','i','t',';' };//,ESC,'%','-','1','2','3','4','5','X' };
*/
const char EnableDuplex[] = { ESC,'&','l', '2', 'S'};
const char NoDepletion[] = {ESC, '*', 'o', '1', 'D'};
const char NoGrayBalance[] = {ESC, '*', 'b', '2', 'B'};

//////////////////////////////////////////
	

///////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
// DeviceRegistry, for isolating all device dependencies
// The data is contained in Registry.cpp

// This object encapsulates all model-specific data for a build.
// Its features are presented to client through the PrintContext.
class DeviceRegistry
{
public:
	DeviceRegistry();
    virtual ~DeviceRegistry();


    // get model string from DevID string
    DRIVER_ERROR ParseDevIDString(const char* sDevID, char* strModel, char* strPens);

	// return the string representing the next model (NULL when finished)
	PRINTER_TYPE EnumDevices(unsigned int& currIdx) const;
	
	// set "device" to index of entry
	virtual DRIVER_ERROR SelectDevice(char* model, char* pens, SystemServices* pSS);

    virtual DRIVER_ERROR SelectDevice(const PRINTER_TYPE Model);
	
	virtual DRIVER_ERROR GetPrinterModel(char* strModel, char* strPens, SystemServices* pSS);

	char* ModelString[MAX_ID_STRING];

	// create a Printer object as pointee of p, using the given SystemServices
	// and the current value of device; still needs to be configured
	virtual DRIVER_ERROR InstantiatePrinter(Printer*& p,SystemServices* pSS);

	
	unsigned int device;							// ordinal of device from list
   
};

////////////////////////////////////////////////
typedef struct 
{
	const unsigned long *ulMap1;
	const unsigned long *ulMap2;
} ColorMap;


///////////////////////////////////////////////////////////////////////////

class Compressor
// Abstract base class for compression methods
{
public:
	Compressor(SystemServices* pSys, BOOL useseed);
	virtual ~Compressor();
	virtual DRIVER_ERROR Compress(const char* input, unsigned int& size)=0;

	void SetSeedRow(char* seed) { SeedRow=seed; }

	DRIVER_ERROR constructor_error;

	SystemServices* pSS;
	// buffer is public for use by GraphicsTranslator
	BYTE* compressBuf;		// output buffer
	char* SeedRow;
	BOOL UseSeedRow;
};

class Mode9 : public Compressor
{
public:
	Mode9(SystemServices* pSys,unsigned int RasterSize);
	virtual ~Mode9();
	DRIVER_ERROR Compress(const char* input, unsigned int& size);
};

class Mode2 : public Compressor
{
public:
	Mode2(SystemServices* pSys,unsigned int RasterSize);
	virtual ~Mode2();
	DRIVER_ERROR Compress(const char* input, unsigned int& size);
};

////////////////////////////////////////////////////////////////////////////
// Translator section
//
// Translator encapsulates generic command-language syntax,
// and is the container class for:
//	 one or both of GraphicsTranslator and TextTranslator;
//	 and a Printer-specific Header object.

class GraphicsTranslator
// Does encapsulation work specific to graphics data
{
friend class Header;			// needs access to ColorLevels, compress
friend class Translator;
public:
	GraphicsTranslator(SystemServices* pSys,Printer* pP, PrintContext* pPC,
                       Imager* pImager, unsigned int iRasterWidth, BOOL bCompress);
	virtual ~GraphicsTranslator();

	DRIVER_ERROR SendRaster();

	DRIVER_ERROR MergeHiResBin(BYTE* data);

	DRIVER_ERROR constructor_error;

#ifdef USAGE_LOG
#define MAXPAGES 10
	unsigned int DotCount[MAXPAGES][MAXDYES];
	unsigned short page;
	void PrintDotCount(char* Header);
	void PrintDotTotals();
	void CountDots(unsigned int dye);
	BOOL NextPage(); 
#endif

private:
	SystemServices* pSS;
	Printer* thePrinter;
    PrintContext* thePrintContext;
	Imager* theImager;

	Compressor* theCompressor;		// NULL if compress==FALSE
	BOOL compress;					// using compression?
	char* SeedRaster;				// buffer used by compression	
	void ResetSeedRaster();			// routine to set it up for next call to Raster
	
	unsigned int RasterSize;					// data size for a raster, 
									// based on current printer/pen
	unsigned int dyeCount;
	unsigned int MaxColorDepth;
	unsigned int ResolutionX[MAXCOLORPLANES];
	unsigned int ResolutionY[MAXCOLORPLANES];	
	BOOL guiMode;					// using GUI command?	
	unsigned int RasterWidth;				// in pixels
	
};

#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
class TextTranslator
// Does encapsulation work specific to ascii data, including
// handling of fonts and treatments.
{
public:
	TextTranslator(int quality,unsigned int colorplanes);
	virtual ~TextTranslator();

	const BYTE* ColorSequence(TEXTCOLOR eColor);	
	BYTE ColorCode(TEXTCOLOR eColor);
	int TypefaceCode(const char* FontName);
	const BYTE* PointsizeSequence(unsigned int iPointsize);
	const BYTE* StyleSequence(BOOL bItalic);
	const BYTE* WeightSequence(BOOL bBold);
	const BYTE* CompleteSequence(const Font& font);
	const BYTE* UnderlineSequence();
	const BYTE* DisableUnderlineSequence();
	
	// "transparent mode" escape to treat control code (BYTE b) as normal char
	int TransparentChar(unsigned int iMaxLen, BYTE b, BYTE* outbuff);	

	DRIVER_ERROR constructor_error;


private:
	int qualcode;						// pcl code for text quality
	BYTE EscSeq[MAX_ESC_SEQ];			// storage for the command
	unsigned int iNumPlanes;						// color planes, based on pen
	BYTE ColorCode1(TEXTCOLOR eColor);	// if iNumPlanes==1 (black)
	BYTE ColorCode3(TEXTCOLOR eColor);	// if iNumPlanes==3 (CMY)
	BYTE ColorCode4(TEXTCOLOR eColor);	// if iNumPlanes==4 (KCMY)
	
};

#endif

///////////////////////////////////////////////////////////////////////
class Header
// Composes a Translator header stream, embodying specific requirements
// of the Printer.
{
friend class Translator;
public:
	Header(Translator* t);
			
	virtual DRIVER_ERROR Send()=0;

	virtual DRIVER_ERROR EndJob();

protected:
	Translator* theTranslator;
	Printer* thePrinter;
    PrintContext* thePrintContext;
    PrintMode* thePrintMode;
	/// routines to set values of internal variables
	void SetMediaType(MediaType mtype);
    void SetMediaSize(PAPER_SIZE papersize);
	void SetMediaSource(MediaSource msource);
	void SetQuality(Quality qual);	
	void SetSimpleColor();

	// components of a header
	DRIVER_ERROR Margins();
	virtual DRIVER_ERROR Graphics();
	DRIVER_ERROR Simple();
	DRIVER_ERROR Modes();
	DRIVER_ERROR ConfigureRasterData();

	// common escapes, plus mode and margin setting
	virtual DRIVER_ERROR StartSend();

////// data members /////////////////////////////////
	unsigned int ResolutionX[MAXCOLORPLANES];
	unsigned int ResolutionY[MAXCOLORPLANES];
	unsigned int dyeCount;

	unsigned int CAPy;	// may be moved during header; retrieved by Job

	// escape sequence constants 
	char SimpleColor[6]; BYTE sccount;		// color command string, and its size
	char mediatype[5]; BYTE mtcount;		// mediatype string, and its size
	char mediasize[6]; BYTE mscount;		// mediasize string, and its size
	char mediasource[5]; BYTE msrccount;	// mediasource string, and its size
	char quality[6]; BYTE qualcount;		// quality string, and its size	
	BYTE QualityCode();			// returns just the variable byte of quality

};

class Header400 : public Header
{
public:
	Header400(Translator* t);
	DRIVER_ERROR Send();

};


class Header6XX : public Header
{
friend class Header890;
public:
	Header6XX(Translator* t);
	virtual DRIVER_ERROR Send();
protected:

};

class Header600 : public Header6XX
{
public:
	Header600(Translator* t);
	DRIVER_ERROR Send();

};

class Header690 : public Header
{
public:
	Header690(Translator* t);
    DRIVER_ERROR Send();
};

class Header540 : public Header
{
public:
	Header540(Translator* t);
	DRIVER_ERROR Send();

};


class Header895 : public Header
{
public:
	Header895(Translator* t);
	virtual DRIVER_ERROR Send();

protected:
	DRIVER_ERROR Graphics();
	DRIVER_ERROR StartSend();
};

class Header900 : public Header895
{
public:
	Header900(Translator* t);
	virtual DRIVER_ERROR Send();

protected:
	BOOL DuplexEnabled(BYTE* bDevIDBuff);

};


class Translator
// Translator encapsulates generic command-language syntax,
// and is the container class for:
//   one or both of GraphicsTranslator and TextTranslator;
//   and a Printer-specific Header object.
//
// When Translator is constructed, it has a Header, but no
// GraphicsTranslator or TextTranslator, since either may be
// missing depending on the type of Job. 
// The Job base constructor creates Translator, while the constructor
// for GraphicsJob installs a GT, the constructor for TextJob
// installs a TT, and a MixedJob installs both.
// It is also the responsibility of the concrete <sub>Job constructor to
// invoke Header::Send(), AFTER the TT and/or GT has been installed.
{
friend class Header;
friend class Header895;
friend class Header900;
public:
   // installs Header and Connection
	Translator(SystemServices* pSys, Printer* pP, PrintContext* pPC);

    virtual ~Translator();

	// you're not done constructing until you call one or both of these
	DRIVER_ERROR InstallGraphicsTranslator(Imager* pU, unsigned int iRasterWidth, BOOL bUseCompression);
	DRIVER_ERROR InstallTextTranslator();
	
	DRIVER_ERROR constructor_error;
// routines to send commands to printer //////////////////////////////////
#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
	// calls using data from TextTranslator ////////////////////////
	DRIVER_ERROR TextOut(const char* pTextString, unsigned int LenString, 
				const Font& font, BOOL sendfont=FALSE, 
				int iAbsX=-1, int iAbsY=-1);
	DRIVER_ERROR SendFont(const Font& font);
	DRIVER_ERROR SendColorSequence(const TEXTCOLOR eColor);
	DRIVER_ERROR SendPointsize(const unsigned int iPointsize);
	DRIVER_ERROR SendStyle(const BOOL bItalic);
	DRIVER_ERROR SendWeight(const BOOL bBold);
	DRIVER_ERROR SendUnderline();
	DRIVER_ERROR SendCompleteSequence(const Font& font);
	DRIVER_ERROR DisableUnderline();
	
	int TransparentChar(unsigned int iMaxLen, BYTE b, BYTE* outbuff)
		{ if (TT==NULL) return 0;  // internal use only; no error check
		  return TT->TransparentChar(iMaxLen,b,outbuff); }
#endif

	// calls using GraphicsTranslator //////////////////////////////
	DRIVER_ERROR SendRaster()
		{ if (GT==NULL) return GRAPHICS_UNSUPPORTED; 
		  return GT->SendRaster(); }

	DRIVER_ERROR MergeHiResBin(BYTE* data)
		{ if (GT==NULL) return GRAPHICS_UNSUPPORTED; 
		  return GT->MergeHiResBin(data); }

	// generic ones ////////////////////////////////////////////////
	DRIVER_ERROR FormFeed();

	DRIVER_ERROR SendCAP(unsigned int iAbsX,unsigned int iAbsY);
	DRIVER_ERROR SendCAPy(unsigned int sAbsY);

	DRIVER_ERROR SendHeader() { return theHeader->Send(); }

	virtual DRIVER_ERROR EndJob() { return theHeader->EndJob(); }

// routines for accessing private or component values from other classes
//	int GetPageWidth()
//		{ return theHeader->PageWidth; }

	BYTE GetQualityCode()
		{ return theHeader->QualityCode(); }

	BOOL IsCompressionOn() { return GT->compress; }

	unsigned int GetCAPy() { return theHeader->CAPy; }

	unsigned int FirstColorPlane(); 
	unsigned int ColorLevels(unsigned int ColorPlane);

#ifdef USAGE_LOG
	void PrintDotCount(char* str) { GT->PrintDotCount(str); }
	void PrintDotTotals() { GT->PrintDotTotals(); }
	BOOL NextPage() { return GT->NextPage(); }
#endif

private:
	SystemServices* pSS;
	Header* theHeader;
	// one or both of these two must be non-NULL and valid
	GraphicsTranslator* GT;
#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
	TextTranslator* TT;
#endif
	// the printer we translate for, and our connection to it
	Printer* thePrinter;

    PrintContext* thePrintContext;
	
	// data members /////////////////////////////////////////
#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
	// items for avoiding redundant font resets 
	// (cheaper than copying whole font)
	TEXTCOLOR lastcolor;
	char lastname[20];
	char lastcharset[MAX_CHAR_SET];
	unsigned int lastpointsize;
	BOOL lastitalic;
	BOOL lastbold;
	void SetLast(const Font& font);
#endif
};
// end of Translator section ///////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)

class TextMapper
// Component of TextManager
// This class encapsulates mappings that may be 
// peculiar to different partners or data sources.
// The use is as follows:
//  1. invoke Map
//  2. now access SubstLen and charset
//
// Currently sets charset to LATIN1 at construction.
{
public:
	TextMapper(Translator* t);

	// main function -- puts alternate string in buffer
	virtual void Map(BYTE b,BYTE* bSubst);

	// public members for access after call to Map()
	unsigned int SubstLen;
	char charset[MAX_CHAR_SET];

protected:
	Translator* theTranslator;
};

class GenericMapper : public TextMapper
{
public:
	GenericMapper(Translator* t);
	void Map(BYTE b,BYTE* bSubst);
};
/////////////////////////////////////////////////////////////////////

class TextManager
// Component of TextJob
{
public:
	TextManager(Translator* t,unsigned int PrintableX, unsigned int PrintableY);
	virtual ~TextManager();
	
	virtual DRIVER_ERROR TextOut(const char* pTextString, unsigned int iLenString, 
				const Font& font, int iAbsX=-1, int iAbsY=-1);
	Translator* theTranslator;

	DRIVER_ERROR constructor_error;

protected:
	
	unsigned int PrintableRegionX;
	unsigned int PrintableRegionY;
	
	DRIVER_ERROR CheckCoords(unsigned int iAbsX, unsigned int iAbsY );

	// This function is here to provide a generic call which may want
	// to send font (general case) or not (SimpleText case)
// OBSOLETE: no longer have SimpleTextManager, so fonts always sent
//	DRIVER_ERROR CallTextOut(const char* pTextString, int iLenString, 
//				const Font& font, int iAbsX=-1, int iAbsY=-1)
//		{ return theTranslator->TextOut(pTextString, iLenString, font,
//										TRUE, // send font
//	
	TextMapper* theMapper;

};

#endif     // FONTS
///////////////////////////////////////////////////////////////////////////



#endif // INTERNAL_H
