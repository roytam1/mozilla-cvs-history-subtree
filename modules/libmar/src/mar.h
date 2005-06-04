/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
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
 * The Original Code is Mozilla Archive code.
 *
 * The Initial Developer of the Original Code is Google Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Darin Fisher <darin@meer.net>
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

#ifndef MAR_H__
#define MAR_H__

#include "prtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MarItem_ {
  struct MarItem_ *next;
  PRUint32 offset;  /* offset into archive */
  PRUint32 length;  /* length of data in bytes */
  PRUint32 flags;   /* contains file mode bits */
  char name[1];
} MarItem;

typedef struct MarFile_ MarFile;

typedef int (* MarItemCallback)(MarFile *mar, const MarItem *item, void *data);


/******************************************************************************
 * The APIs below require mar_read.c
 */

/* open a mar archive for reading */
MarFile *mar_open(const char *path);

/* close a mar archive that was opened using mar_open */
void mar_close(MarFile *mar);

/* find a mar item by name */
const MarItem *mar_find_item(MarFile *mar, const char *item);

/* enumerate all mar items via callback function */
int mar_enum_items(MarFile *mar, MarItemCallback callback, void *data);

/* read from mar item at given offset up to bufsize bytes */
int mar_read(MarFile *mar, const MarItem *item, int offset, char *buf,
             int bufsize);


/******************************************************************************
 * The APIs below require additional source modules
 */

/* create the archive from a set of files (needs mar_create.c) */
int mar_create(const char *dest, int num_files, char **files);

/* test mar file (needs mar_test.c) */
int mar_test(const char *path);

/* extract mar file to working directory (needs mar_extract.c) */
int mar_extract(const char *path);

#ifdef __cplusplus
}
#endif

#endif  /* MAR_H__ */
