/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is mozilla.org code, released
 * Jan 28, 2003.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2003 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *    Garrett Arch Blythe, 28-January-2003
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the MPL or the GPL.
 */

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>

/*
**  This file exists to perform a pathetic emulation of console
**      applications on the CE platform.
**
**  Much nicer implementations exist I am sure, which forward the
**      output to a window and read input from an entry field, but
**      here is something that works and gets the initial worries
**      out of the way.
*/


extern "C" int main(int inArgc, char**inArgv, char**inEnv);

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPTSTR    lpCmdLine,
                   int       nCmdShow)
{
    char *argv[100];
    int argc = 0;
    char commandLineA[0x1000];
    
    memset(argv, 0, sizeof(argv));
    
    //
    //  Retrieve the full command line.
    //  One passed in at least misses argv[0]....
    //
    LPTSTR commandLine = GetCommandLine();
    if(NULL != commandLine)
    {
        //
        //  Convert to multibyte.
        //  Multibyte limit sizeof(commandLineA).
        //
        int convertRes = WideCharToMultiByte(
            CP_ACP,
            0,
            commandLine,
            -1,
            commandLineA,
            sizeof(commandLineA),
            NULL,
            NULL
            );
        
        if(0 != convertRes)
        {
            int traverse = 0;
            
            //
            //  Whack up the multibyte version of the command line.
            //  Argument limit sizeof(argv) / sizeof(char*).
            //
            for(int loop = 0; loop < sizeof(argv) / sizeof(char*); loop++)
            {
                if('\0' != commandLineA[traverse] && traverse < sizeof(commandLineA))
                {
                    //
                    //  Quoting and other misfits are not handled.
                    //
                    //  Skip white space till we find an argument.
                    //  Record the position, find the end of the argument.
                    //  Truncate the argument, store in argv.
                    //

                    while(isspace(commandLineA[traverse]))
                    {
                        traverse++;
                    }

                    char* beginning = &commandLineA[traverse];

                    while('\0' != commandLineA[traverse] && 0 == isspace(commandLineA[traverse]))
                    {
                        traverse++;
                    }

                    if('\0' != commandLineA[traverse])
                    {
                        commandLineA[traverse] = '\0';
                        traverse++;
                    }

                    argv[loop] = beginning;
                }
                else
                {
                    //
                    //  End of string.
                    //
                    break;
                }
            }

            argc = loop;
        }
    }
    
    //
    // Rewrite stdin, stdout, stderr handles to some files.
    // Assuming the location valid.
    //
    FILE* redir_stdin = _wfreopen(_T("\\Temp\\stdin.txt"), _T("rb"), stdin);
    FILE* redir_stdout = _wfreopen(_T("\\Temp\\stdout.txt"), _T("wb"), stdout);
    FILE* redir_stderr = _wfreopen(_T("\\Temp\\stderr.txt"), _T("wb"), stderr);
    
    //
    // Invoke main.
    //
    int mainRetval = main(argc, argv, NULL);
    
    return 0;
}
