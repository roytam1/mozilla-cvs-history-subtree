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
extern BOOL argProprietary;
#else
#include "Header.h"
extern BOOL Proprietary();
#endif

extern char DeveloperString[32];

const char ClearCaseID[17] = "001.000_08-22-00";


char result[300];
 

char* Version(int bCompressed)
{

if (bCompressed)
{ 
    unsigned int bits=0;
#ifdef PROTO
    if (argProprietary)
#else
    if (Proprietary())
#endif
      bits = bits | 0x80000000;

#ifdef CAPTURE
  bits = bits | 0x40000000;
#endif
#ifdef _LITTLE_ENDIAN
  bits = bits | 0x20000000;
#endif
  // leaving room for 1 here
#ifdef _COURIER
  bits = bits | 0x8000000;
#endif
#ifdef _CGTIMES
  bits = bits | 0x4000000;
#endif
#ifdef _LTRGOTHIC
  bits = bits | 0x2000000;
#endif
#ifdef _UNIVERS
  bits = bits | 0x1000000;
#endif



#ifdef _DJ400
  bits = bits | 0x800000;
#endif
#ifdef _DJ540
  bits = bits | 0x400000;
#endif
#ifdef _DJ600
  bits = bits | 0x200000;
#endif
#ifdef _DJ6xx
  bits = bits | 0x100000;
#endif
#ifdef _DJ6xxPhoto
  bits = bits | 0x80000;
#endif
#ifdef _DJ8xx
  bits = bits | 0x40000;
#endif
#ifdef _DJ9xx
  bits = bits | 0x20000;
#endif

  // room left for 17 more here

  sprintf(result,"%0x", bits);

}
else
{
    
 strcpy(result,DeveloperString);
 strcat(result,"!!");
 strcat(result,ClearCaseID);
 strcat(result," ");

#ifdef PROTO
    if (argProprietary)
#else
    if (Proprietary())
#endif
    strcat(result,"prop ");
 else strcat(result,"open ");

#ifdef CAPTURE
  strcat(result,"debug ");
#else
  strcat(result,"normal ");
#endif
#ifdef _LITTLE_ENDIAN
  strcat(result,"little_endian ");
#else
  strcat(result,"big_endian ");
#endif
#if defined(_CGTIMES) || defined(_COURIER) || defined(_LTRGOTHIC) || defined(_UNIVERS)
  strcat(result,"fonts:");
#else
  strcat(result,"no_fonts");
#endif
#ifdef _COURIER
  strcat(result,"C");
#endif
#ifdef _CGTIMES
  strcat(result,"T");
#endif
#ifdef _LTRGOTHIC
  strcat(result,"L");
#endif
#ifdef _UNIVERS
  strcat(result,"U");
#endif

  strcat(result," ");

#ifdef _DJ400
  strcat(result,ModelName[DJ400]); strcat(result," ");
#endif
#ifdef _DJ540
  strcat(result,ModelName[DJ540]); strcat(result," ");
#endif
#ifdef _DJ600
  strcat(result,ModelName[DJ600]); strcat(result," ");
#endif
#ifdef _DJ6xx
  strcat(result,ModelName[DJ6xx]); strcat(result," ");
#endif
#ifdef _DJ6xxPhoto
  strcat(result,ModelName[DJ6xxPhoto]); strcat(result," ");
#endif
#ifdef _DJ8xx
  strcat(result,ModelName[DJ8xx]); strcat(result," ");
#endif
#ifdef _DJ9xx
  strcat(result,ModelName[DJ9xx]);
#endif

}
  
  return result;
}