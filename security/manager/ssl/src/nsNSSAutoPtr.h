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
 * The Original Code is __________________________________________.
 *
 * The Initial Developer of the Original Code is
 * ____________________________________________.
 * Portions created by the Initial Developer are Copyright (C) 2___
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#ifndef _NSNSSAUTOPTR_H_
#define _NSNSSAUTOPTR_H_

template <class T>
class nsNSSAutoCPtr
{
public:

  typedef void (*destroyer)(T* t);

  nsNSSAutoCPtr<T>( T* t, destroyer d)
  :data(t), data_destroyer(d)
  {
  }
  
  ~nsNSSAutoCPtr<T>()
  {
    if (data && data_destroyer)
    {
      (*data_destroyer)(data);
    }
  }
  
  operator T*()
  {
    return data;
  }
  
  T& operator*()
  {
    return *data;
  }

  T* operator->()
  {
    return data;
  }
  
  PRBool operator!()
  {
    return !data;
  }
  
  void setPtr( T *t)
  {
    data = t;
  }
  
  T* ptr()
  {
    return data;
  }

private:

  T *data;
  destroyer data_destroyer;

};

#endif //_NSNSSAUTOPTR_H_
