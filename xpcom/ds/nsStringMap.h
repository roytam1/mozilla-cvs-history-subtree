#ifndef nsStringMap_h__
#define nsStringMap_h__

/* 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is Mozilla Communicator client code.
 * 
 * The Initial Developer of the Original Code is James L. Nance
 * Portions created by James L. Nance are Copyright (C) 2001
 * James L. Nance.  All  Rights Reserved.
 * 
 * Contributor(s): Patricia Jewell Nance, Jesse Jacob Nance
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

#if !defined(TEST_PATRICIA)
#   include "nscore.h"
#   include "prtypes.h"
#   include "plarena.h"
#endif

typedef PRBool (*PR_CALLBACK nsStringMapEnumFunc) (
    const char *aKey, void *aData, void *aClosure);

class nsStringMap
{
public:
    nsStringMap();
    ~nsStringMap();
    PRBool Put(const char *str, void *obj, PRBool copy=PR_FALSE);
    void*  Get(const char *str);
    void   Reset();
    void   Reset(nsStringMapEnumFunc destroyFunc, void *aClosure = 0);
    void   Enumerate(nsStringMapEnumFunc aEnumFunc, void *aClosure = 0);

    struct Patricia {
	Patricia   *l, *r;
	PRUint32    bit;    // Bit position for l/r comp
	const char *key;
	void       *obj;
    };

    // The BitTester class is used to test a particular bit position in a 
    // 0 terminated string of unknown length.  Bits after the end of the
    // string are treated as zero
    class BitTester {
	PRUint32    slen;
	const char *cstr;
    public:
	BitTester(const char *s) : slen(nsCRT::strlen(s)), cstr(s) {}
	BitTester(const char *s, PRUint32 l) : slen(l), cstr(s) {}

	// We dont know how long ostr is, but by including the terminating
	// 0 character in the comparison we cover the case where str is a
	// substring of ostr.
	PRInt32 strcmp(const char *ostr) const {
	    return nsCRT::memcmp((void*)cstr, ostr, slen+1);
	}

	PRUint32 strlen() const {return slen;}

	static PRBool isset_checked(const char *str, PRUint32 idx) {
	    return (str[idx/8] & (1<<(idx & 7))) != 0;
	}

	static PRBool
	bitsequal(const char *str1, const char*str2, PRUint32 idx) {
	    return (str1[idx/8] & (1<<(idx&7)))==(str2[idx/8] & (1<<(idx&7)));
	}

	PRBool isset(PRUint32 idx) {
	    const PRUint32 base = idx/8;
	    if(base>=slen) return 0;
	    return (cstr[base] & (1<<(idx & 7))) != 0;
	}
    };

private:
    PLArenaPool mPool;
    Patricia *newNode();
    Patricia *searchDown(BitTester&);
    void enumerate_recurse(nsStringMapEnumFunc, void*, Patricia*);
    Patricia head; // Sentinal node
    PRInt32  numEntries;
    static const char zero_str[];
};

#endif
