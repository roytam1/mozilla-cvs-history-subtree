
#ifndef nsSpillableBuffer_h__
#define nsSpillableBuffer_h__

/*
  nsSpillableBuffer
  
  This class provides a stack buffer that spills into the heap when
  necessary.
  
  After constructing one of these, you should *always* check that
  the buffer allocation succeeded by calling GetCapacity().
*/


template <class T>
class nsSpillableBuffer
{
public:
  typedef T unit_type;
  typedef PRInt32   TUnitCount;
  
protected:

  enum {
      kStackBufferUnitCount		= 256
  };

public:

  nsSpillableBuffer(TUnitCount inBufferUnitCount = 0)
  :	mBufferPtr(mBuffer)
  , mCurCapacity(kStackBufferUnitCount)
  {
    if (inBufferUnitCount > kStackBufferUnitCount)
      EnsureCapacity(inBufferUnitCount);
  }

  ~nsSpillableBuffer()
  {
    DeleteBuffer();
  }

  PRBool EnsureCapacity(TUnitCount inUnitCapacity)
  {
    // we currently don't free up memory if the buffer shrinks, since we expect
    // instances of this class to be short-lived
    if (inUnitCapacity < mCurCapacity)
      return PR_TRUE;
    
    if (inUnitCapacity > kStackBufferUnitCount)
    {
      DeleteBuffer();

      // could do chunking here
      unit_type*  newBuffer = (unit_type*)nsMemory::Alloc(inUnitCapacity * sizeof(unit_type));
      if (!newBuffer) return PR_FALSE;
      
      mBufferPtr    = newBuffer;
      mCurCapacity  = inUnitCapacity;
      return PR_TRUE;
    }
    
    mCurCapacity = kStackBufferUnitCount;
    return PR_TRUE;
  }
                
  unit_type*  GetBuffer()     { return mBufferPtr;    }
  PRInt32     GetCapacity()   { return mCurCapacity;  }

protected:

  void DeleteBuffer()
  {
    if (mBufferPtr != mBuffer)
    {
      nsMemory::Free(mBufferPtr);
      mBufferPtr = mBuffer;
      mCurCapacity = kStackBufferUnitCount;
    }                
  }
  
protected:

  unit_type	    *mBufferPtr;
  unit_type	    mBuffer[kStackBufferUnitCount * sizeof(unit_type)];
  TUnitCount    mCurCapacity;

};

#endif // nsSpillableBuffer_h__
