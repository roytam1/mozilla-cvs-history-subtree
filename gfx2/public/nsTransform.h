/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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
 * The Original Code is mozilla.org code.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 2000 Netscape Communications Corporation. All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *   Stuart Parmenter <pavlov@netscape.com>
 */

#include "nsMatrix.h"


/**
 * @file nsTransform.h
 */

/**
 * nsTransform class
 * @class nsTransform
 */
class nsTransform {
 public:
  nsTransform();
  virtual ~nsTransform();

  /**
   * Transform types
   */
  //@{

  /** 
   * The unit type is not one of predefined types. It is invalid to attempt to define a
   * new value of this type or to attempt to switch an existing value to this type.
   */
  const unsigned short NS_TRANSFORM_UNKNOWN   = 0;

  /**
   * A "matrix(...)" transformation.
   */
  const unsigned short NS_TRANSFORM_MATRIX    = 1;

  /**
   * A "translate(...)" transformation.
   */
  const unsigned short NS_TRANSFORM_TRANSLATE = 2;

  /**
   * A "scale(...)" transformation.
   */
  const unsigned short NS_TRANSFORM_SCALE     = 3;

  /**
   * A "rotate(...)" transformation.
   */
  const unsigned short NS_TRANSFORM_ROTATE    = 4;

  /**
   * A "skewX(...)" transformation.
   */
  const unsigned short NS_TRANSFORM_SKEWX     = 5;

  /**
   * A "skewY(...)" transformation.
   */
  const unsigned short NS_TRANSFORM_SKEWY     = 6;

  //@}

  unsigned short getType();
  nsMatrix getMatrix();
  float getAngle;

  /**
   * Sets the transform type to NS_TRANSFORM_MATRIX, with parameter matrix defining the new transformation.
   *
   * @param matrix The new matrix for the transformation.
   */
  void setMatrix(nsMatrix matrix);

  /**
   * Sets the transform type to NS_TRANSFORM_TRANSLATE, with parameters tx and ty defining the translation amounts
   *
   * @param tx The translation amount in X.
   * @param ty The translation amount in Y.
   */
  void setTranslate(float tx, float ty);

  /**
   * Sets the transform type to NS_TRANSFORM_SCALE, with parameters sx and sy defining the scale amounts.
   *
   * @param sx The scale factor in X.
   * @param sy The scale factor in Y.
   */
  void setScale(float sx, float sy);

  /**
   * Sets the transform type to NS_TRANSFORM_ROTATE, with parameter angle defining the rotation angle and
   * parameters cx and cy defining the optional centre of rotation.
   *
   * @param angle The rotation angle.
   * @param cx The x coordinate of centre of rotation.
   * @param cy The y coordinate of centre of rotation.
   */
  void setRotate(float angle, float cx, float cy);

  /**
   * Sets the transform type to NS_TRANSFORM_SKEWX, with parameter angle defining the amount of skew.
   *
   * @param angle The skew angle.
   */
  void setSkewX(float angle);

  /**
   * Sets the transform type to NS_TRANSFORM_SKEWY, with parameter angle defining the amount of skew.
   *
   * @param angle The skew angle.
   */
  void setSkewY(float angle);
};
