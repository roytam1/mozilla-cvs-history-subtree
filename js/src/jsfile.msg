/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

/*
        Error messages for jsfile.c. See js.msg for format specification.
*/

MSG_DEF(JSFILEMSG_NOT_AN_ERROR,                                    0, 0, JSEXN_NONE, "<Error #0 is reserved>")
MSG_DEF(JSFILEMSG_FILE_CONSTRUCTOR_UNDEFINED_ERROR,                1, 0, JSEXN_NONE, "File constructor is undefined")
MSG_DEF(JSFILEMSG_FILE_CURRENTDIR_UNDEFINED_ERROR,                 2, 0, JSEXN_NONE, "File.currentDir is undefined")
MSG_DEF(JSFILEMSG_FIRST_ARGUMENT_OPEN_NOT_STRING_ERROR,            3, 1, JSEXN_NONE, "The first argument {0} to file.open must be a string")
MSG_DEF(JSFILEMSG_SECOND_ARGUMENT_OPEN_NOT_STRING_ERROR,           4, 1, JSEXN_NONE, "The second argument {0} to file.open must be a string")
MSG_DEF(JSFILEMSG_CANNOT_COPY_FILE_OPEN_FOR_WRITING_ERROR,         5, 1, JSEXN_NONE, "Cannot copy file {0} open for writing")
MSG_DEF(JSFILEMSG_INIT_FAILED,                                     6, 0, JSEXN_NONE, "JS_InitClass failed")
MSG_DEF(JSFILEMSG_COPY_READ_ERROR,                                 7, 1, JSEXN_NONE, "An error occured while attempting to read a file {0} to copy")
MSG_DEF(JSFILEMSG_COPY_WRITE_ERROR,                                8, 1, JSEXN_NONE, "An error occured while attempting to copy into file {0}")
MSG_DEF(JSFILEMSG_EXPECTS_ONE_ARG_ERROR,                           9, 2, JSEXN_NONE, "Operation {0} expects one argument, not {1}")
MSG_DEF(JSFILEMSG_CANNOT_OPEN_WRITING_ERROR,                      10, 1, JSEXN_NONE, "Cannot open file {0} for writing")
MSG_DEF(JSFILEMSG_WRITEALL_EXPECTS_ONE_ARG_ERROR,                 11, 0, JSEXN_NONE, "writeAll expects one argument")
MSG_DEF(JSFILEMSG_FIRST_ARGUMENT_WRITEALL_NOT_ARRAY_ERROR,        12, 0, JSEXN_NONE, "writeAll expects an array as an argument")
MSG_DEF(JSFILEMSG_FIRST_ARGUMENT_CONSTRUCTOR_NOT_STRING_ERROR,    13, 1, JSEXN_NONE, "The argument to the File constructor {0} must be a string")
MSG_DEF(JSFILEMSG_BIDIRECTIONAL_PIPE_NOT_SUPPORTED,               14, 0, JSEXN_NONE, "Bidirectional pipes are not supported")
MSG_DEF(JSFILEMSG_OPEN_MODE_NOT_SUPPORTED_WITH_PIPES,             15, 2, JSEXN_NONE, "The opening mode you have chosen {0} is not supported by the pipe you are trying to open: {1}")
MSG_DEF(JSFILEMSG_OP_FAILED,                                      16, 2, JSEXN_NONE, "\"{0}\" operation failed on file {1}")
MSG_DEF(JSFILEMSG_CANNOT_ACCESS_FILE_STATUS,                      17, 1, JSEXN_NONE, "Cannot access file status for {0}")
MSG_DEF(JSFILEMSG_RENAME_FAILED,                                  18, 2, JSEXN_NONE, "Cannot rename {0} to {1}")
MSG_DEF(JSFILEMSG_FIRST_ARGUMENT_MUST_BE_A_FUNCTION_OR_REGEX,     19, 1, JSEXN_NONE, "The first argument to file.list must be a function or a regex")
MSG_DEF(JSFILEMSG_CANNOT_DO_LIST_ON_A_FILE,                       20, 1, JSEXN_NONE, "{0} must be a directory, cannot do list")
MSG_DEF(JSFILEMSG_CANNOT_SET_PRIVATE_FILE,                        21, 1, JSEXN_NONE, "Cannot set private data for file {0}")
MSG_DEF(JSFILEMSG_FIRST_ARGUMENT_MUST_BE_A_NUMBER,                22, 2, JSEXN_NONE, "First argument to {0} must be a number, not {1}")
MSG_DEF(JSFILEMSG_CANNOT_WRITE,                                   23, 1, JSEXN_NONE, "Cannot write to {0}, file mode is different")
MSG_DEF(JSFILEMSG_CANNOT_READ,                                    24, 1, JSEXN_NONE, "Cannot read from {0}, file mode is different")
MSG_DEF(JSFILEMSG_OBJECT_CREATION_FAILED,                         25, 1, JSEXN_NONE, "Failed to create a new object in {0}")
MSG_DEF(JSFILEMSG_UNDEF_ARRAY_LENGTH,                             26, 1, JSEXN_NONE, "Array length cannot be determined")
MSG_DEF(JSFILEMSG_CANNOT_OPEN_DIR,                                27, 1, JSEXN_NONE, "Cannot open directory {0}")
MSG_DEF(JSFILEMSG_NO_RANDOM_ACCESS,                               28, 1, JSEXN_NONE, "File {0} doesn't support random access")
MSG_DEF(JSFILEMSG_CANNOT_REPORT_POSITION,                         29, 1, JSEXN_NONE, "Cannot report the position in file {0}, seek operation failed")
MSG_DEF(JSFILEMSG_CANNOT_SET_POSITION,                            30, 1, JSEXN_NONE, "Cannot set the position in file {0}, seek operation failed")

