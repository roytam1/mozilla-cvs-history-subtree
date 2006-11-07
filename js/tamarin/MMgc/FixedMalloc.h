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


#ifndef __Malloc__
#define __Malloc__


namespace MMgc
{
	/**
	 * A general purpose memory allocator using size classes 
	 */
	class FixedMalloc : public GCAllocObject
	{
		friend class GC;
	public:
		static void Init();
		static void Destroy();
		/**
		* return the number of bytes of memory issued
		*/
		size_t Allocated();
		static FixedMalloc *GetInstance() { return instance; }

		inline void* Alloc(size_t size)
		{
			void *item;
			GCAssert(size + 3 > size);
			// overflow detection
			if(size+3 < size)
				return NULL;
			size = (size+3)&~3;
			if (size <= (size_t)kLargestAlloc) {
				item = FindSizeClass(size)->Alloc(size);
			} else {
				item = LargeAlloc(size);
			}
			return item;
		}

		inline void Free(void *item)
		{
			if(item == 0)
				return;

			// small things are never allocated on the 4K boundary b/c the block
			// header structure is stored there, large things always are		
			if(IsLargeAlloc(item)) {
				LargeFree(item);
			} else {		
				FixedAllocSafe::GetFixedAllocSafe(item)->Free(item);
			}
		}

		static size_t Size(const void *item)
		{
			size_t size;
			if(IsLargeAlloc(item)) {
				size = LargeSize(item);
			} else {		
				size = FixedAlloc::Size(item);
			}
#ifdef MEMORY_INFO
			size -= DebugSize();
#endif
			return size;
		}

//	private:
		FixedMalloc();
		~FixedMalloc();
		static FixedMalloc *instance;
		const static int kLargestAlloc = 2032;	
		const static int kNumSizeClasses = 41;
		const static int kPageUsableSpace = GCHeap::kBlockSize - offsetof(MMgc::FixedAlloc::FixedBlock, items);

		static int16 kSizeClasses[kNumSizeClasses];
		static uint8 kSizeClassIndex[32];

		GCHeap *m_heap;
		FixedAllocSafe *m_allocs[kNumSizeClasses];	
		FixedAllocSafe *FindSizeClass(size_t size) const;

		static bool IsLargeAlloc(const void *item)
		{
			// space made in ctor
			item = GetRealPointer(item);
			return ((intptr) item & 0xFFF) == 0;
		}

		inline void *LargeAlloc(size_t size)
		{
			size += DebugSize();
			size_t blocksNeeded = ((size+0xfff)&~0xfff) >> 12;
			void *item = m_heap->Alloc(blocksNeeded, true, false);
			if(!item)
			{
				GCAssertMsg(item != NULL, "Large allocation of %d blocks failed!");
			}
			else
			{
#ifdef MEMORY_INFO
				item = DebugDecorate(item, size, 5);
				memset(item, 0xfb, size - DebugSize());
#endif
			}
			return item;
		}

	
		inline void LargeFree(void *item)
		{
#ifdef MEMORY_INFO
			item = DebugFree(item, 0xed, 5);
#endif
			m_heap->Free(item);
		}

		static size_t LargeSize(const void *item)
		{
			return GCHeap::GetGCHeap()->Size(item) * GCHeap::kBlockSize;
		}
	};
}
#endif /* __Malloc__ */
