/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "plstr.h"
#include "nsColor.h"
#include "nsColorNames.h"
#include "nsString.h"
#include "nscore.h"
#include "nsCoord.h"
#include "nsUnitConversion.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIScreen.h"
#include "nsIScreenManager.h"
#include <math.h>

static int ComponentValue(const char* aColorSpec, int aLen, int color, int dpc)
{
  int component = 0;
  int index = (color * dpc);
  if (2 < dpc) {
    dpc = 2;
  }
  while (--dpc >= 0) {
    char ch = ((index < aLen) ? aColorSpec[index++] : '0');
    if (('0' <= ch) && (ch <= '9')) {
      component = (component * 16) + (ch - '0');
    } else if ((('a' <= ch) && (ch <= 'f')) || 
               (('A' <= ch) && (ch <= 'F'))) {
      // "ch&7" handles lower and uppercase hex alphabetics
      component = (component * 16) + (ch & 7) + 9;
    }
    else {  // not a hex digit, treat it like 0
      component = (component * 16);
    }
  }
  return component;
}

extern "C" NS_GFX_(PRBool) NS_HexToRGB(const nsString& aColorSpec, nscolor* aResult)
{
  // XXXldb nsStackString<10>
  NS_LossyConvertUCS2toASCII bufferStr(aColorSpec);
  const char* buffer = bufferStr.get();

  int nameLen = bufferStr.Length();
  if ((nameLen == 3) || (nameLen == 6)) {
    // Make sure the digits are legal
    for (int i = 0; i < nameLen; i++) {
      char ch = buffer[i];
      if (((ch >= '0') && (ch <= '9')) ||
          ((ch >= 'a') && (ch <= 'f')) ||
          ((ch >= 'A') && (ch <= 'F'))) {
        // Legal character
        continue;
      }
      // Whoops. Illegal character.
      return PR_FALSE;
    }

    // Convert the ascii to binary
    int dpc = ((3 == nameLen) ? 1 : 2);
    // Translate components from hex to binary
    int r = ComponentValue(buffer, nameLen, 0, dpc);
    int g = ComponentValue(buffer, nameLen, 1, dpc);
    int b = ComponentValue(buffer, nameLen, 2, dpc);
    if (dpc == 1) {
      // Scale single digit component to an 8 bit value. Replicate the
      // single digit to compute the new value.
      r = (r << 4) | r;
      g = (g << 4) | g;
      b = (b << 4) | b;
    }
    NS_ASSERTION((r >= 0) && (r <= 255), "bad r");
    NS_ASSERTION((g >= 0) && (g <= 255), "bad g");
    NS_ASSERTION((b >= 0) && (b <= 255), "bad b");
    if (nsnull != aResult) {
      *aResult = NS_RGB(r, g, b);
    }
    return PR_TRUE;
  }

  // Improperly formatted color value
  return PR_FALSE;
}

// compatible with legacy Nav behavior
extern "C" NS_GFX_(PRBool) NS_LooseHexToRGB(const nsString& aColorSpec, nscolor* aResult)
{
  // XXXldb nsStackString<30>
  NS_LossyConvertUCS2toASCII buffer(aColorSpec);

  int nameLen = buffer.Length();
  const char* colorSpec = buffer.get();
  if ('#' == colorSpec[0]) {
    ++colorSpec;
    --nameLen;
  }

  if (3 < nameLen) {
    // Convert the ascii to binary
    int dpc = (nameLen / 3) + (((nameLen % 3) != 0) ? 1 : 0);
    if (4 < dpc) {
      dpc = 4;
    }

    // Translate components from hex to binary
    int r = ComponentValue(colorSpec, nameLen, 0, dpc);
    int g = ComponentValue(colorSpec, nameLen, 1, dpc);
    int b = ComponentValue(colorSpec, nameLen, 2, dpc);
    NS_ASSERTION((r >= 0) && (r <= 255), "bad r");
    NS_ASSERTION((g >= 0) && (g <= 255), "bad g");
    NS_ASSERTION((b >= 0) && (b <= 255), "bad b");
    if (nsnull != aResult) {
      *aResult = NS_RGB(r, g, b);
    }
  }
  else {
    if (nsnull != aResult) {
      *aResult = NS_RGB(0, 0, 0);
    }
  }
  return PR_TRUE;
}

extern "C" NS_GFX_(PRBool) NS_ColorNameToRGB(const nsAString& aColorName, nscolor* aResult)
{
  nsColorName id = nsColorNames::LookupName(aColorName);
  if (eColorName_UNKNOWN < id) {
    NS_ASSERTION(id < eColorName_COUNT, "LookupName mess up");
    if (nsnull != aResult) {
      *aResult = nsColorNames::kColors[id];
    }
    return PR_TRUE;
  }
  return PR_FALSE;
}

extern "C" NS_GFX_(nscolor) NS_BrightenColor(nscolor inColor)
{
  PRIntn r, g, b, max, over;

  r = NS_GET_R(inColor);
  g = NS_GET_G(inColor);
  b = NS_GET_B(inColor);

  //10% of max color increase across the board
  r += 25;
  g += 25;
  b += 25;

  //figure out which color is largest
  if (r > g)
  {
    if (b > r)
      max = b;
    else
      max = r;
  }
  else
  {
    if (b > g)
      max = b;
    else
      max = g;
  }

  //if we overflowed on this max color, increase
  //other components by the overflow amount
  if (max > 255)
  {
    over = max - 255;

    if (max == r)
    {
      g += over;
      b += over;
    }
    else if (max == g)
    {
      r += over;
      b += over;
    }
    else
    {
      r += over;
      g += over;
    }
  }

  //clamp
  if (r > 255)
    r = 255;
  if (g > 255)
    g = 255;
  if (b > 255)
    b = 255;

  return NS_RGBA(r, g, b, NS_GET_A(inColor));
}

extern "C" NS_GFX_(nscolor) NS_DarkenColor(nscolor inColor)
{
  PRIntn r, g, b, max;

  r = NS_GET_R(inColor);
  g = NS_GET_G(inColor);
  b = NS_GET_B(inColor);

  //10% of max color decrease across the board
  r -= 25;
  g -= 25;
  b -= 25;

  //figure out which color is largest
  if (r > g)
  {
    if (b > r)
      max = b;
    else
      max = r;
  }
  else
  {
    if (b > g)
      max = b;
    else
      max = g;
  }

  //if we underflowed on this max color, decrease
  //other components by the underflow amount
  if (max < 0)
  {
    if (max == r)
    {
      g += max;
      b += max;
    }
    else if (max == g)
    {
      r += max;
      b += max;
    }
    else
    {
      r += max;
      g += max;
    }
  }

  //clamp
  if (r < 0)
    r = 0;
  if (g < 0)
    g = 0;
  if (b < 0)
    b = 0;

  return NS_RGBA(r, g, b, NS_GET_A(inColor));
}

// Function to convert RGB color space into the HSV colorspace
// Hue is the primary color defined from 0 to 359 degrees
// Saturation is defined from 0 to 255.  The higher the number.. the deeper the color
// Value is the brightness of the color. 0 is black, 255 is white.  
extern "C" NS_GFX_(void)
NS_RGB2HSV(nscolor aColor,PRUint16 &aHue,PRUint16 &aSat,PRUint16 &aValue)
{
PRUint8  r,g,b;
PRInt16  delta,min,max,r1,b1,g1;
float    hue;

  r = NS_GET_R(aColor);
  g = NS_GET_G(aColor);
  b = NS_GET_B(aColor);

  if (r>g) {
    max = r;
    min = g;
  } else {
    max = g;
    min = r;
  }

  if (b>max) {
    max = b;
  }
  if (b<min) {
    min = b;
  }

  // value or brightness will always be the max of all the colors(RGB)
  aValue = max;   
  delta = max-min;
  aSat = (max!=0)?((delta*255)/max):0;
  r1 = r;
  b1 = b;
  g1 = g;

  if (aSat==0) {
    hue = 1000;
  } else {
    if(r==max){
      hue=(float)(g1-b1)/(float)delta;
    } else if (g1==max) {
      hue= 2.0f+(float)(b1-r1)/(float)delta;
    } else { 
      hue = 4.0f+(float)(r1-g1)/(float)delta;
    }
  }

  if(hue<999) {
    hue*=60;
    if(hue<0){
      hue+=360;
    }
  } else {
    hue=0;
  }

  aHue = (PRUint16) hue;
}

// Function to convert HSV color space into the RGB colorspace
// Hue is the primary color defined from 0 to 359 degrees
// Saturation is defined from 0 to 255.  The higher the number.. the deeper the color
// Value is the brightness of the color. 0 is black, 255 is white.  
extern "C" NS_GFX_(void)
NS_HSV2RGB(nscolor &aColor,PRUint16 aHue,PRUint16 aSat,PRUint16 aValue)
{
PRUint16  r=0,g=0,b=0;
PRUint16  i,p,q,t;
double    h,f,percent;

  if ( aSat == 0 ) {
    // achromatic color, no hue is defined
    r = aValue;
    g = aValue;
    b = aValue;
  } else {
    // hue in in degrees around the color wheel defined from
    // 0 to 360 degrees.  
    if (aHue >= 360) {
      aHue = 0;
    }

    // we break the color wheel into 6 areas.. these
    // areas define how the saturation and value define the color.
    // reds behave differently than the blues
    h = (double)aHue / 60.0;
    i = (PRUint16) floor(h);
    f = h-(double)i;
    percent = ((double)aValue/255.0);   // this needs to be a value from 0 to 1, so a percentage
                                        // can be calculated of the saturation.
    p = (PRUint16)(percent*(255-aSat));
    q = (PRUint16)(percent*(255-(aSat*f)));
    t = (PRUint16)(percent*(255-(aSat*(1.0-f))));

    // i is guarenteed to never be larger than 5.
    switch(i){
      case 0: r = aValue; g = t; b = p; break;
      case 1: r = q; g = aValue; b = p; break;
      case 2: r = p; g = aValue; b = t; break;
      case 3: r = p; g = q; b = aValue; break;
      case 4: r = t; g = p; b = aValue; break;
      case 5: r = aValue; g = p; b = q; break;
    }
  }
  aColor = NS_RGB(r,g,b);
}

