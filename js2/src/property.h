/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express oqr
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is the JavaScript 2 Prototype.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the NPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the NPL or the GPL.
 */


#ifndef property_h___
#define property_h___

#include <map>

namespace JavaScript {

    struct ExprList;
    typedef ExprList AttributeList;

namespace JS2Runtime {

    class JSFunction;
    class JSValue;
    class JSType;

    typedef enum { ValuePointer, FunctionPair, IndexPair, Slot, Method, Constructor } PropertyFlag;    
    
    class Property {
    public:
        Property() { }

        Property(uint32 g, uint32 s, JSType *type)
            : mType(type), mFlag(IndexPair) 
        { 
            mData.iPair.getterI = g;
            mData.iPair.setterI = s;
        }

        Property(JSType *type, JSFunction *g, JSFunction *s)    // XXX the type is the return
                                                                // type of the getter function.
            : mType(type), mFlag(FunctionPair) 
        { 
            mData.fPair.getterF = g;  
            mData.fPair.setterF = s;
        }

        Property(uint32 i, JSType *type, PropertyFlag flag) 
            : mType(type), mFlag(flag)
        { 
            mData.index = i;
        }

        Property(JSValue *p, JSType *type) 
            : mType(type), mFlag(ValuePointer) 
        { 
            mData.vp = p;
        }
        
        
        union {
            JSValue *vp;        // straightforward value
            struct {            // getter & setter functions
                JSFunction *getterF;
                JSFunction *setterF;
            } fPair;
            struct {            // getter & setter methods (in base->mMethods)
                uint32 getterI;
                uint32 setterI;
            } iPair;
            uint32 index;       // method (in base->mMethods) or 
                                // slot (in base->mInstanceValues)
        } mData;
        JSType *mType;
        PropertyFlag mFlag;
    };
    Formatter& operator<<(Formatter& f, const Property& prop);
   

    typedef std::pair<Property, AttributeList *> AttributedProperty;

    typedef std::multimap<String, AttributedProperty, std::less<const String> > PropertyMap;
    typedef PropertyMap::iterator PropertyIterator;


#define PROPERTY_KIND(it)           ((it)->second.first.mFlag)
#define PROPERTY(it)                ((it)->second.first)
#define ATTR_PROPERTY(it)           ((it)->second)
#define PROPERTY_ATTRLIST(it)       ((it)->second.second)
#define PROPERTY_VALUEPOINTER(it)   ((it)->second.first.mData.vp)
#define PROPERTY_INDEX(it)          ((it)->second.first.mData.index)
#define PROPERTY_NAME(it)           ((it)->first)
#define PROPERTY_GETTERF(it)        ((it)->second.first.mData.fPair.getterF)
#define PROPERTY_SETTERF(it)        ((it)->second.first.mData.fPair.setterF)
#define PROPERTY_GETTERI(it)        ((it)->second.first.mData.iPair.getterI)
#define PROPERTY_SETTERI(it)        ((it)->second.first.mData.iPair.setterI)


}
}


#endif