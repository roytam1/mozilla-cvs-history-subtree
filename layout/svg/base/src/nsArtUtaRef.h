/* ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is Mozilla SVG Project.
 *
 * The Initial Developer of the Original Code is
 * Crocodile Clips Ltd.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Original Author: James Turner (james.turner@crocodile-clips.com)
 *
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
 *
 * ***** END LICENSE BLOCK ***** */


#ifndef __NSSVG_ART_REF_H__
#define __NSSVG_ART_REF_H__

#include "libart-incs.h"

class nsArtUtaRef
{
public:
  typedef ArtUta* value_ptr;
  
  // constructors / assignment operators
  nsArtUtaRef(value_ptr x) :
    mCounter(x ? new refCounter(x) : nsnull)
  {
    if (mCounter)
      mCounter->addRef();
  }
  
  nsArtUtaRef(const nsArtUtaRef &copy) :
    mCounter(copy.mCounter)
  {
    if (mCounter)
      mCounter->addRef();
  }
  
  nsArtUtaRef& operator=(const nsArtUtaRef& x)
  {
    if (mCounter)
      mCounter->decRef();
    mCounter = x.mCounter;
    
    if(mCounter)
      mCounter->addRef();
    return *this;
  }
    
  ~nsArtUtaRef() {
    if (mCounter)
    mCounter->decRef();
  }

  // 'looks-like-a-pointer' methods
  ArtUta& operator*()
  {
    NS_ASSERTION(mCounter, "de-referencing NULL nsArtUtaRef");
    return *(mCounter->mInner);
  }
  
  const ArtUta& operator*() const
  {
    NS_ASSERTION(mCounter, "de-referencing NULL nsArtUtaRef");
    return *(mCounter->mInner);
  }
  
  value_ptr operator->()
  {
    NS_ASSERTION(mCounter, "de-referencing NULL nsArtUtaRef");
    return mCounter->mInner;
  } 
  
  operator const void* () const { return mCounter ? this : nsnull; } 
  
  // hack
  value_ptr get() const
  {
    NS_ASSERTION(mCounter, "de-referencing NULL nsArtUtaRef");
    return mCounter->mInner;
  }
  
  // comparisons
  // I think defaults are okay
  
private:
  /* the inner class holds the ref-count, and owns the C pointer,
  primitive thing that it is.  This is pretty crude, but should get
  the job done. Note we always allocate these things on the heap, so
  calling 'delete this' is safe. Trust me.*/
  struct refCounter
  {
    explicit refCounter(value_ptr x) : 
      mRefCount(0), // init to zero, not one : the constructing outer instance will addRef
      mInner(x)
    {
      NS_ASSERTION(x, "NULL instance pointer passed to nsArtUtaRef");
    }
  
    ~refCounter()
    {
      NS_ASSERTION(mRefCount == 0, "nsArtUtaRef count non-zero in dtor");
      art_uta_free(mInner);
    }
  
    void addRef() { mRefCount++;}
    void decRef() { if (--mRefCount <= 0) delete this; }
    
    int mRefCount;
    value_ptr mInner;
  };
  
  /// the shared inner counter object for this reference
  refCounter* mCounter;
};

#endif // __NSSVG_ART_REF_H__

