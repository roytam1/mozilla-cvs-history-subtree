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

// HPPrintAPI.h
// Definitions and structures needed by applications

#ifndef HPPRINTAPI_H
#define HPPRINTAPI_H

#include "models.h"
// ** Defines

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef BOOL
  typedef int BOOL;
  #ifndef TRUE
  #define TRUE 1
  #endif
  #ifndef FALSE
  #define FALSE 0
  #endif
#endif

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

#ifndef LOWORD
#define LOWORD(l)   ((WORD) (l))
#endif

#ifndef HIWORD
#define HIWORD(l)   ((WORD) (((DWORD) (l) >> 16) & 0xFFFF))
#endif

#ifdef BLACK_PEN
#undef BLACK_PEN
#endif

#ifdef NO_ERROR
#undef NO_ERROR
#endif

typedef int DRIVER_ERROR;

typedef int MediaType;		// for use in connection with PCL media-type command
							// values are PCL codes
#define mediaPlain 0
#define mediaBond 0
#define mediaSpecial 2
#define mediaGlossy 3
#define mediaTransparency 4

typedef int MediaSize;		// for use in connection with PCL media-size command
							// values are PCL codes
#define sizeUSLetter 2
#define sizeUSLegal 3
#define sizeA4 26
#define sizeNum10Env 81
#define sizePhoto   99  // BOGUS NUMBER I MADE UP
	
typedef int MediaSource;	// for use in connection with PCL media-source command
							// values are PCL codes		
#define sourceTray1 1
//#define sourceTray2 4		// no second tray in current supported list
#define sourceManual 2
#define sourceManualEnv 3
	
typedef int Quality;		// for use in connection with PCL quality-mode command
							// values are PCL codes	
#define qualityPresentation 1
#define qualityNormal 0
#define qualityDraft -1


#define COURIER_INDEX 1
#define CGTIMES_INDEX 2
#define LETTERGOTHIC_INDEX 3
#define UNIVERS_INDEX 4


#define MAX_CHAR_SET 5
#define MAX_POINTSIZES 5

#define GRAYMODE_INDEX 0
#define DEFAULTMODE_INDEX 1
#define SPECIALMODE_INDEX 2


// ** JOB related structures/enums

enum PEN_TYPE { BLACK_PEN, COLOR_PEN, BOTH_PENS, MDL_PEN, MDL_BOTH, NO_PEN, DUMMY_PEN };
#define MAX_PEN_TYPE 4      // base-0, ending with MDL_BOTH (NOT NO_PEN)

// the PAPER_SIZE enum is directly supported by PSM in PrintContext
enum PAPER_SIZE { UNSUPPORTED_SIZE =-1, LETTER = 0, A4 = 1, LEGAL = 2, PHOTO_SIZE = 3}; 

enum PRINTER_FUNC { CLEAN_PEN = 0 };


// ** TEXT related structures/enums

enum TEXTCOLOR {

	WHITE_TEXT,
	CYAN_TEXT,
	MAGENTA_TEXT,
	BLUE_TEXT,
	YELLOW_TEXT,
	GREEN_TEXT,
	RED_TEXT,
	BLACK_TEXT

};


enum TEXTORIENT {
// currently only portrait fonts are supported

	PORTRAIT,
	LANDSCAPE,
	BOTH

};

#define MAX_FONT_SIZES 10 	// max # of fonts to be realized at one time 

// ** I/O related stuff
#define TIMEOUTVAL 		500		// in msec, ie 0.5 sec

typedef WORD PORTID;
typedef void * PORTHANDLE;

enum MODE1284
{
	COMPATIBILITY,
	NIBBLE,
	ECP
};

//////////////////////////////////////////////////////////////////////////////////////
//  values of DRIVER_ERROR
// first of 2 hex digits indicates category

// general or system errors

#define NO_ERROR			    0x0	    // everything okay	
#define JOB_CANCELED		    0x1		// CANCEL chosen by user
#define SYSTEM_ERROR		    0x2     // something bad that should not have happened		
#define ALLOCMEM_ERROR          0x3     // failed to allocate memory
#define NO_PRINTER_SELECTED     0x4     // indicates improper calling sequence or unidi
#define INDEX_OUT_OF_RANGE      0x5     // what it says
#define ILLEGAL_RESOLUTION      0x6     // tried to set resolution at unacceptable value
#define NULL_POINTER            0x7     // supplied ptr was null
// build-related
// (items either absent from current build, or just bad index from client code)

#define UNSUPPORTED_PRINTER     0x10    // selected printer-type unsupported in build
#define UNSUPPORTED_PEN         0x11    // selected pen-type unsupported
#define TEXT_UNSUPPORTED        0x12    // no text allowed in current build
#define GRAPHICS_UNSUPPORTED    0x13    // no graphics allowed in current build
#define UNSUPPORTED_FONT        0x14    // font selection failed
#define	ILLEGAL_COORDS          0x15    // bad (x,y) passed to TextOut
#define UNSUPPORTED_FUNCTION    0x16    // bad selection for PerformPrinterFunction

// I/O related 
			
#define IO_ERROR                0x20                                            
#define BAD_DEVICE_ID		    0x21

// WARNINGS
// convention is that values < 0 can be ignored (at user's peril)
#define WARN_MODE_MISMATCH      -1      // printmode selection incompatible with pen, tray, etc.
#define WARN_DUPLEX             -2      // duplexer installed; our driver can't use it
#define WARN_PHOTOTRAY          -3      // phototray not used by our driver

///////////////////////////////////////////////////////////////////////////////////////

// ** Printer Status return values


enum DISPLAY_STATUS {    // used for DisplayPrinterStatus

	DISPLAY_PRINTING,
	DISPLAY_PRINTING_COMPLETE,
	DISPLAY_PRINTING_CANCELED,
	DISPLAY_OFFLINE,
	DISPLAY_BUSY,
	DISPLAY_OUT_OF_PAPER,
	DISPLAY_TOP_COVER_OPEN,
	DISPLAY_ERROR_TRAP,
	DISPLAY_NO_PRINTER_FOUND,
	DISPLAY_NO_PEN_DJ400,
	DISPLAY_NO_PEN_DJ600,
	DISPLAY_NO_COLOR_PEN,
	DISPLAY_NO_BLACK_PEN,
	DISPLAY_NO_PENS,
	DISPLAY_PHOTO_PEN_WARN,
	DISPLAY_PRINTER_NOT_SUPPORTED,
	DISPLAY_COMM_PROBLEM,
	DISPLAY_CANT_ID_PRINTER,
	ACCEPT_DEFAULT		// internal driver use only

};

const char ModelName[7][11]={"DJ400","DJ540","DJ600","DJ6xx","DJ6xxPhoto","DJ8xx","DJ9xx"}; 


					
//////////////////////////////////////////////////////////////////////
// objects needed by client of Slimhost++ driver

//forward declarations
class DeviceRegistry;
class Header;
class Printer;
class TextManager;
class Translator;
class Imager;
class Scaler;
class Scripter;

// items from wtv_interp.h
#define NUMBER_PLANES   3 // 3 for RGB 4 for alphaRGB
#define NUMBER_RASTERS  3 // The number of Rasters to Buffer

// until resolution issue resolved
#define	PRINTABLEREGIONX 2400
#define PRINTABLEREGIONY 3000

struct fOptSubSig
{
    float pi;
    const float *means;
};

struct fOptClassSig
{
    int nsubclasses;
    float variance;
    float inv_variance;
    float cnst;
    struct fOptSubSig *OptSubSig;
};

struct fOptSigSet
{
    int nbands;
	struct fOptClassSig *OptClassSig;
};
typedef struct
{
		int				Width;
		int				ScaleFactorMultiplier;
		int				ScaleFactorDivisor;
		int				CallerAlloc;		 //	Who does the memory alloc.
		int             Remainder;           // For use in non integer scaling cases
		int             Repeat;				 // When to send an extra output raster
		int             RastersinBuffer;     // # of currently buffered rasters 
		unsigned char*  Bufferpt[NUMBER_RASTERS];
		int				BufferSize;
		unsigned char*	Buffer;
		struct fOptSigSet OS;
		struct fOptSubSig rsOptSubSigPtr1[45];
		struct fOptClassSig OCS;
		float **joint_means;
		float ***filter;
		float filterPtr1[45];
		float filterPtr2[45][9];
		float joint_meansPtr1[45];

} RESSYNSTRUCT;

class IO_MODE 
{
public:
 //NO_FEEDBACK, STATUS_ONLY, BIDI, USB
	BOOL bDevID;
	BOOL bStatus;
	BOOL bUSB;
};

#define DevIDBuffSize 255	

#define DevIDBuffSize 255		// size of buffer used by SetDevInfo

////////////////////////
class SystemServices
{
friend class PrintContext;
friend class Printer;   // for saved device strings
public:
	SystemServices();
	virtual ~SystemServices();					

    // check for validty of constructed object
	DRIVER_ERROR constructor_error;	

    // must include in derived class constructor (if using bi-di)
    DRIVER_ERROR InitDeviceComm();   

	/////////////////////////////////////////////////////////////////////
    IO_MODE IOMode;    

    virtual DRIVER_ERROR FlushIO() { return 0; }

    virtual DRIVER_ERROR AbortIO() { return 0; }

    virtual void DisplayPrinterStatus (DISPLAY_STATUS ePrinterStatus)=0;

    virtual DRIVER_ERROR BusyWait(DWORD msec)=0;

    virtual DRIVER_ERROR ReadDeviceID(BYTE* strID, int iSize)=0;

    virtual BYTE* AllocMem (int iMemSize)=0;

	virtual void FreeMem (BYTE* pMem)=0;

    virtual BOOL PrinterIsAlive();

    virtual void GetStatusInfo(BYTE* bStatReg)=0;

    virtual DRIVER_ERROR ToDevice(const BYTE* pBuffer, WORD* wCount)=0;

    virtual DRIVER_ERROR FromDevice(char* pReadBuff, WORD* wReadCount)=0;

    // override this function to implement DJ400 
    virtual DRIVER_ERROR GetECPStatus(BYTE *pStatusString,int *pECPLength, int ECPChannel);

    virtual BOOL YieldToSystem (void)=0;

    virtual BYTE GetRandomNumber()=0;

    virtual DWORD GetSystemTickCount (void)=0;

    virtual float power(float x, float y)=0;

// utilities ///////////////////////////////////////////////////////
    // call FreeMem after checking for null ptr
	DRIVER_ERROR FreeMemory(void *ptr);    
    DRIVER_ERROR GetDeviceID(BYTE* strID, int iSize);


#if defined(CAPTURE) || defined(PROTO)
    Scripter *pScripter;
    DRIVER_ERROR InitScript(const char* FileName, BOOL ascii, BOOL read=FALSE);
    DRIVER_ERROR EndScript();
    BOOL Capturing;
    BOOL replay;
#endif


protected:
    // reconcile printer's preferred settings with reality
    virtual void AdjustIO(IO_MODE IM) 
        { IOMode.bStatus=IM.bStatus && IOMode.bStatus; 
          IOMode.bDevID =IM.bDevID  && IOMode.bDevID; }

    BYTE strDevID[DevIDBuffSize]; // save whole DevID string

private:

	PORTID ePortID;

    DeviceRegistry* DR;

    char strModel[200]; // to contain the MODEL (MDL) from the DevID 
    char strPens[64];   // to contain the VSTATUS penID from the DevID 
    
};


#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)

// font names
const char sCourier[]="Courier";
const char sCGTimes[]="CGTimes";
const char sLetterGothic[]="LetterGothic";
const char sUnivers[]="Univers";
const char sBad[]="Bad";

class Font
// This is the base class for Fonts.
// It is not abstract, so that clients can request a font generically,
// but its constructor is not public -- Fonts are rather created
// through RealizeFont -- thus it is effectively "abstract" in this sense.
// Example:
//			Font* myFont = myJob->RealizeFont(FIXED_SERIF,12,...);
//
// Note that Printer initially constructs a dummy font (with default values)
// for each of its typefaces, so that the Is<x>Allowed functions can be 
// invoked (for EnumFonts) prior to choosing specific instances. 
// Then the Clone function is invoked by
// Printer::RealizeFont to provide instances to client.
{
friend class Printer;
friend class TextManager;
public:
	// constructors are protected -- clients use Job::RealizeFont()
	virtual ~Font();

	// public functions

	// the base class version is really for printer fonts
	virtual DRIVER_ERROR GetTextExtent(const char* pTextString,const int iLenString,
								int& iHeight, int& iWidth);

////// these functions allow access to properties of derived classes

	// return typeface name
	virtual const char* GetName() const { return sBad; }

	// functions to tell what treatments are possible
	virtual BOOL IsBoldAllowed() const { return FALSE; }			
	virtual BOOL IsItalicAllowed() const { return FALSE; }		
	virtual BOOL IsUnderlineAllowed() const { return FALSE; }		
	virtual BOOL IsColorAllowed() const { return FALSE; }	
	virtual BOOL IsProportional() const { return FALSE; }
	virtual BOOL HasSerif() const { return FALSE; }


	// return pitch for given point size
	virtual BYTE GetPitch(const BYTE pointsize) const
		{ return 0; }	// default for proportionals

////// these data members give the properties of the actual instance
	// as set by the user
	int			iPointsize;
	BOOL		bBold;		// boolean TRUE to request bold
	BOOL		bItalic;	// boolean TRUE to request italic
	BOOL		bUnderline;	// boolean TRUE to request underline
	TEXTCOLOR	eColor;		// enum
	int			iPitch;

	// string designating character set (as recognized by firmware)
	//
	// REVISIT: shouldn't really have Translator data here; we
	// should have an enum here, which is interpreted by Translator
	char charset[MAX_CHAR_SET];	
	
	BOOL PrinterBased;

	virtual int Index() { return -1; };
	// items for spooling
//	virtual BOOL Equal(Font* f);
//	virtual DRIVER_ERROR Store(FILE* sp, int& size);
//	virtual int SpoolSize();

protected:
	// constructor, invoked by derivative constructors
	Font(int SizesAvailable,BYTE size=0,
			BOOL bold=FALSE, BOOL italic=FALSE, BOOL underline=FALSE,
			TEXTCOLOR color=BLACK_TEXT,BOOL printer=TRUE,
			unsigned int pvres=300,unsigned int phres=300);
	
	// copy constructor used by RealizeFont
	Font(const Font& f,const BYTE bSize,
		 const TEXTCOLOR color, const BOOL bold,
		 const BOOL italic, const BOOL underline);
	
	// return a clone with a different character set
	// base class version should not be called -- this should be pure virtual!
	virtual Font* CharSetClone(char* NewCharSet) const;

	int numsizes;	// number of available pointsizes
	// return array of sizes allowed
	virtual BYTE* GetSizes() const { return (BYTE*)NULL; }
	// return index of pointsize from array of available pointsizes
	virtual int Ordinal(unsigned int /* pointsize */) const 
		{ return 0; }

	// match arbitrary input size to one we have
	int AssignSize(int Size);
	void Subst_Char(int& bCurrChar)const;	

	// pointers to the arrays containing widths for a given font
	//  separated into Lo (32..127) & Hi (160..255)	
	const BYTE *pWidthLo[MAX_POINTSIZES];	
	const BYTE *pWidthHi[MAX_POINTSIZES];

	unsigned int PrinterVRes;
	unsigned int PrinterHRes;

	
private:
#ifdef CAPTURE

	void Capture_dFont(const unsigned int ptr);

#endif

};   

class ReferenceFont : public Font
// The main purpose of this class is to hide the destructor, since
// the fonts that live with the Printer and are returned by EnumFont
// are meant to remain alive for the life of the Printer.
{
friend class Printer;           // deletes from its fontarray
friend class DeskJet400;        // replaces fontarray from base class
public:
    ReferenceFont(int SizesAvailable,BYTE size=0,
			BOOL bold=FALSE, BOOL italic=FALSE, BOOL underline=FALSE,
			TEXTCOLOR color=BLACK_TEXT,BOOL printer=TRUE,
			unsigned int pvres=300,unsigned int phres=300);
protected:
    ~ReferenceFont();  

    // copy constructor used by RealizeFont
	ReferenceFont(const ReferenceFont& f,const BYTE bSize,
		 const TEXTCOLOR color, const BOOL bold,
		 const BOOL italic, const BOOL underline);

};
#endif      // if fonts used

class PrintMode;

class PrintContext
{
friend class Job;         // access to private (non-instrumented) versions of routines
friend class Header;      // access to private (non-instrumented) versions of routines
friend class Translator;    // access to current printmode
friend class GraphicsTranslator; // access to current printmode
friend class DeskJet690;  // to override media and quality settings
public:
	PrintContext(SystemServices *pSysServ, unsigned int PixelsPerRow=0,
                 PAPER_SIZE ps = LETTER);        	

	virtual ~PrintContext();

	DRIVER_ERROR constructor_error;


    void Flush(int FlushSize);

	// used when constructor couldn't instantiate printer (no DevID) -- instantiate now
	DRIVER_ERROR SelectDevice(const PRINTER_TYPE Model);

    unsigned int GetModeCount();
    DRIVER_ERROR SelectPrintMode(const unsigned int index);
    unsigned int CurrentPrintMode() { return CurrentModeIndex; }
    char* GetModeName();

    PRINTER_TYPE SelectedDevice(); 


#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
	ReferenceFont* EnumFont(int& iCurrIdx); 
		//	{ return thePrinter->EnumFont(iCurrIdx); } 
	virtual Font* RealizeFont(const int index,const BYTE bSize,
							const TEXTCOLOR eColor=BLACK_TEXT,
							const BOOL bBold=FALSE,const BOOL bItalic=FALSE,
							const BOOL bUnderline=FALSE);
		//	{ return thePrinter->RealizeFont(eFont,bSize,eColor,
		//									bBold,bItalic,bUnderline); }
#endif

    // return the enum for the next model in DR (UNSUPPORTED when finished)
	PRINTER_TYPE EnumDevices(unsigned int& currIdx) const;
       
	// PerformPrinterFunction (clean pen, etc.)
	// this is the preferred function to call
	DRIVER_ERROR PerformPrinterFunction(PRINTER_FUNC eFunc); 
	
    ///////////////////////////////////////////////////////////////////////
    // routines to change settings
    DRIVER_ERROR SetPaperSize(PAPER_SIZE ps); 
    // these are dependent on printer model in use, thus can err
    DRIVER_ERROR SetInputPixelsPerRow(unsigned int PixelsPerRow);
    //
    // routines to query selections ///////////////////////////////////////
    BOOL PrinterSelected() { return !(thePrinter==NULL); }
    BOOL PrinterFontsAvailable(unsigned int PrintModeIndex);    // return FALSE if no printer
    int InputPixelsPerRow() { return InputRasterWidth; }
    PAPER_SIZE GetPaperSize();

    const char* PrinterModel();
    const char* PrintertypeToString(PRINTER_TYPE pt); // returns string for use in UI

    unsigned int InputResolution() { return InputRes; }
	DRIVER_ERROR SetInputResolution(unsigned int Res);
    unsigned int EffectiveResolution();       // res we need in current mode

    // get settings pertaining to the printer
    // note:these return zero if no printer selected
    // all results in inches
    float PrintableWidth();                         
	float PrintableHeight();                        
	float PhysicalPageSizeX();
	float PhysicalPageSizeY();
	float PrintableStartX();
	float PrintableStartY();

    // SPECIAL API -- NOT TO BE USED IN CONNECTION WITH JOB
    DRIVER_ERROR SendPrinterReadyData(BYTE* stream, unsigned int size);
	
	DeviceRegistry* DR;     // unprotected for replay system

private:

	SystemServices* pSS;
	Printer* thePrinter;
    PrintMode* CurrentMode;
    unsigned int CurrentModeIndex;
	
    unsigned int InputRes;                 // input resolution
    unsigned int InputRasterWidth;
    unsigned int PageWidth;             // pixel width of printable area

    struct PaperSizeMetrics
    {
        // all values are in inches
	    float	fPhysicalPageX;
	    float	fPhysicalPageY;
	    float	fPrintablePageX;
	    float	fPrintablePageY;
		float   fPrintableStartY;
    } PSM[4];  // the size of this struct is directly related to the PAPER_SIZE enum

    void InitPSMetrics();   // used by constructors


	// internal versions of public functions
	float printablewidth();
	float printableheight();
	unsigned int printerunitsY();

    BOOL ModeAgreesWithHardware(BOOL QueryPrinter);

           
    // code savers
    void SetWidths(); 
    DRIVER_ERROR SetMode(unsigned int ModeIndex);
    BOOL SelectDefaultMode();

	
#ifdef CAPTURE
	void Capture_PrintContext(unsigned int PixelsPerRow,
                              PAPER_SIZE ps,IO_MODE IOMode);
    void Capture_SelectDevice(const PRINTER_TYPE Model);
    void Capture_SelectPrintMode(unsigned int modenum);
    void Capture_SetPaperSize(PAPER_SIZE ps);
	void Capture_RealizeFont(const unsigned int ptr,const unsigned int index,const BYTE bSize,
							const TEXTCOLOR eColor=BLACK_TEXT,
							const BOOL bBold=FALSE,const BOOL bItalic=FALSE,
							const BOOL bUnderline=FALSE);
	void Capture_SetPixelsPerRow(unsigned int iWidth);
    void Capture_SetInputResolution(unsigned int Res);
	void Capture_dPrintContext();

#endif

};
class Job
{
public:
	Job(PrintContext* pPC);
													
	virtual ~Job();
	
	DRIVER_ERROR constructor_error;		// caller must check upon return

	DRIVER_ERROR SendRasters(BYTE* ImageData=(BYTE*)NULL, BYTE* BinaryTextData=(BYTE*)NULL);
	
#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)	
	DRIVER_ERROR TextOut(const char* pTextString, unsigned int iLenString,
							const Font& font, int iAbsX, int iAbsY );
	// return theTextManager->TextOut(pTextString,iLenString,font,iAbsX,iAbsY);
#endif

	DRIVER_ERROR NewPage();

private:

	PrintContext* thePrintContext;
	Printer* thePrinter;
	Translator* theTranslator;
	Imager* pImager;

#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
	TextManager* theTextManager;
#endif

    PrintMode* CurrentMode;

    unsigned int InputDPI;
    unsigned int skipcount;
    unsigned int RowsInput;

	unsigned int CAPy;			// maintains cursor-pos for graphics purposes,
						// independent of intervening text positioning
	BOOL DataSent;

    unsigned int RowMultiple;   // used for sending rows more than once

	DRIVER_ERROR newpage();
	DRIVER_ERROR SetupColor();
	DRIVER_ERROR SetupTranslator();

#ifdef CAPTURE
	void Capture_Job(PrintContext* pPC);
	void Capture_dJob();
	void Capture_SendRasters(BYTE* ImageData, BYTE* BinaryTextData);
#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
	void Capture_TextOut(const char* pTextString, unsigned int iLenString,
							const Font& font, unsigned int iAbsX, unsigned int iAbsY );
#endif
	void Capture_NewPage();
#endif

#ifdef USAGE_LOG
	int UTextCount;
	int UText;
#define UTextSize 100
	char UHeader[UTextSize*2];
#endif
};

class Scaler
{
public:
    // constructor protected -- use Create_Scaler()
	virtual ~Scaler();
	unsigned int Process(const BYTE* raster_in=(BYTE*)NULL);
	
	DRIVER_ERROR constructor_error;

	// output buffer
	BYTE *pOutputBuffer;

	unsigned int GetOutputWidth();
	unsigned int GetInputWidth();

    float ScaleFactor;
	
	
protected:
    Scaler(SystemServices* pSys,unsigned int inputwidth,unsigned int numerator,unsigned int denominator);
	SystemServices* pSS;
	unsigned int ResSyn(const unsigned char *raster_in);
	int create_out(BOOL simple);
	virtual void rez_synth(RESSYNSTRUCT *ResSynStruct, unsigned char *raster_out)=0;
	void Pixel_ReplicateF(int color, int h_offset, int v_offset, 
						  unsigned char **out_raster, int plane);
    virtual void InitInternals() { }

	RESSYNSTRUCT* pRSstruct;
	BOOL scaling;		// false iff ScaleFactor==1.0
	BOOL ReplicateOnly;	// true iff 1<ScaleFactor<2	
	

	unsigned int iOutputWidth;
	unsigned int iInputWidth;


private:
#ifdef CAPTURE
	void Capture_Scaler(unsigned int inputwidth,unsigned int numerator,unsigned int denominator);
	void Capture_Process(const BYTE* raster_in);
	void Capture_dScaler();
#endif
};

// For client code to create either an open or closed Scaler
extern Scaler* Create_Scaler(SystemServices*, int, int, int);

#endif // HPPRINTAPI_H
