/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef MKFILE_H
#define MKFILE_H

#include "mkfsort.h"
#include "fileurl.h"

class nsFileTransfer;

/* states of the machine
 */
typedef enum {
	NET_CHECK_FILE_TYPE,
	NET_DELETE_FILE,
	NET_MOVE_FILE,
	NET_PUT_FILE,
	NET_MAKE_DIRECTORY,
	NET_FILE_SETUP_STREAM,
	NET_OPEN_FILE,
	NET_SETUP_FILE_STREAM,
	NET_OPEN_DIRECTORY,
	NET_READ_FILE_CHUNK,
	NET_READ_DIRECTORY_CHUNK,
	NET_BEGIN_PRINT_DIRECTORY,
	NET_PRINT_DIRECTORY,
	NET_FILE_DONE,
	NET_FILE_ERROR_DONE,
	NET_FILE_FREE
} net_FileStates;


typedef struct _FILEConData {
    XP_File           file_ptr;
    XP_Dir            dir_ptr;
    net_FileStates    next_state;
    NET_StreamClass * stream;
    char            * filename;
    Bool           is_dir;
    SortStruct      * sort_base;
    Bool           pause_for_read;
    Bool           is_cache_file;
	Bool		   calling_netlib_all_the_time;
    Bool destroy_graph_progress;  /* do we need to destroy graph progress? */
    int32   original_content_length; /* the content length at the time of
                                      * calling graph progress
                                      */
	char             * byterange_string; 
	int32              range_length;  /* the length of the current byte range */

#if defined(SMOOTH_PROGRESS)
    nsFileTransfer* transfer;
#endif
} FILEConData;

#endif /* MKFILE_H */
