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


#ifndef __GCAllocBase__
#define __GCAllocBase__


namespace MMgc
{
	class GC;
	
	/**
	 * This is a base class for the GC allocators.
	 */
	class GCAllocBase : public GCAllocObject
	{
	public:
		GCAllocBase(GC *gc) { m_gc = gc; }
		virtual ~GCAllocBase() {}

		
		#ifdef MMGC_IA32
		static inline uint32 FindOneBit(uint32 value)
		{
#ifndef __GNUC__
			_asm
			{
				bsr eax,[value];
			}
#else
			// DBC - This gets rid of a compiler warning and matchs PPC results where value = 0
			register int	result = ~0;
			
			if (value)
			{
				asm (
					"bsr %1, %0"
					: "=r" (result)
					: "m"(value)
					);
			}
			return result;
#endif
		}
		#endif

		#ifdef MMGC_PPC
		static inline int FindOneBit(uint32 value)
		{
			register int index;
			#ifdef DARWIN
			asm ("cntlzw %0,%1" : "=r" (index) : "r" (value));
			#else
			register uint32 in = value;
			asm { cntlzw index, in; }
			#endif
			return 31-index;
		}
		#endif
		
		static int CountBits(uint32 value);

#ifdef MEMORY_INFO
		// debugging routine that records who marked who, can be used to
		// answer the question, how did I get marked?  also could be used to
		// find false positives by verifying the back pointer chain gets back
		// to a GC root
		static void WriteBackPointer(const void *item, const void *container, size_t itemSize);
#endif

	protected:
		GC *m_gc;
	};
}

#endif /* __GCAllocBase__ */
