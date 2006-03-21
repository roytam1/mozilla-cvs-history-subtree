/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Membuf server code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by the Initial Developer are
 * Copyright (C) 2003 the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Ben Goodger <ben@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 */

#ifndef _membufImageDownSample_h
#define _membufImageDownSample_h

// Allegro defines non-ISO C++ compliant ZERO_SIZED_ARRAY
#include "membufAllegroDefines.h"

#include "nspr/prtypes.h"

// Down-Samples the given image for display on the given device.
/*void DownSampleImage(struct Image* aImage, PRUint8 aDevice);


void DownSampleCleanup();

// Converter Functions
void DownSample16(struct Image* aImage, PRUint8* aColorCube,
                  int aR, int aG, int aB);
void DownSample16D(struct Image* aImage, PRUint8* aColorCube,
                   int aR, int aG, int aB);
void DownSample8D(struct Image* aImage, PRUint8* aColorCube,
                  int aR, int aG, int aB);
void DownSample4D(struct Image* aImage, PRUint8* aColorCube,
             int aR, int aG, int aB);
void DownSample4GrayD(struct Image* aImage, PRUint8* aColorCube,
                 int aR, int aG, int aB);
void DownSample1(struct Image* aImage, PRUint8* aColorCube,
            int aR, int aG, int aB);
*/

// Color Cube Creation Functions
uint8* MakeDitherCube(unsigned long* aPixels, int aR, int aG, int aB);
uint8* MakeColorCubeInternal(unsigned long* aPixels, int aR, int aG, int aB);
uint8* MakeColorCube222();
uint8* MakeColorCube(int aR, int aG, int aB);

uint8* MakeDownSamplePalette(int aR, int aG, int aB);

#endif // _membufImageDownSample_h
