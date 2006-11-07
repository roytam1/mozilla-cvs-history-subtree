/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1 
 *
 * The contents of this file are subject to the Mozilla Public License Version 1.1 (the 
 * "License"); you may not use this file except in compliance with the License. You may obtain 
 * a copy of the License at http://www.mozilla.org/MPL/ 
 * 
 * Software distributed under the License is distributed on an "AS IS" basis, WITHOUT 
 * WARRANTY OF ANY KIND, either express or implied. See the License for the specific 
 * language governing rights and limitations under the License. 
 * 
 * The Original Code is [Open Source Virtual Machine.] 
 * 
 * The Initial Developer of the Original Code is Adobe System Incorporated.  Portions created 
 * by the Initial Developer are Copyright (C)[ 2004-2006 ] Adobe Systems Incorporated. All Rights 
 * Reserved. 
 * 
 * Contributor(s): Adobe AS3 Team
 * 
 * Alternatively, the contents of this file may be used under the terms of either the GNU 
 * General Public License Version 2 or later (the "GPL"), or the GNU Lesser General Public 
 * License Version 2.1 or later (the "LGPL"), in which case the provisions of the GPL or the 
 * LGPL are applicable instead of those above. If you wish to allow use of your version of this 
 * file only under the terms of either the GPL or the LGPL, and not to allow others to use your 
 * version of this file under the terms of the MPL, indicate your decision by deleting provisions 
 * above and replace them with the notice and other provisions required by the GPL or the 
 * LGPL. If you do not delete the provisions above, a recipient may use your version of this file 
 * under the terms of any one of the MPL, the GPL or the LGPL. 
 * 
 ***** END LICENSE BLOCK ***** */



#include <stdlib.h>

#include "MMgc.h"
#include "GCDebug.h"

namespace MMgc
{	
	/* Returns the number of bits set in val. 
	 * For a derivation of this algorithm, see 
	 * "Algorithms and data structures with applications to  
	 *  graphics and geometry", by Jurg Nievergelt and Klaus Hinrichs, 
	 *  Prentice Hall, 1993. 
	 */ 
	int GCAllocBase::CountBits(uint32 value)
	{ 
		value -= (value & 0xaaaaaaaaL) >> 1; 
		value =  (value & 0x33333333L) + ((value >> 2) & 0x33333333L); 
		value =  (value + (value >> 4)) & 0x0f0f0f0fL; 
		value += value >> 8;      
		value += value >> 16;     
		return (int)value & 0xff; 
	}  

#ifdef MEMORY_INFO
	void GCAllocBase::WriteBackPointer(const void *item, const void *container, size_t itemSize)
	{
		GCAssert(container != NULL);
		int *p = (int*) item;
		size_t size = *p++;
		if(size && size <= itemSize) {
			// skip traceIndex + data + endMarker
			p += (2 + (size>>2));
			GCAssert(sizeof(int) == sizeof(void*));
			*p = (int) container;
		}
	}
#endif
}
