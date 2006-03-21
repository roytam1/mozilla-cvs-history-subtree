/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is Anya server code
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by the Initial Developer are
 * Copyright (C) 2003 the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Stuart Parmenter <pavlov@netscape.com>
 *    Joe Hewitt <hewitt@netscape.com>
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
 */

#include "membufAllegroDefines.h"

#define MEMBUF_VERSION 3
#define MEMBUF_PORT 6666

/* float to fixed 16.16 conversions */
/* XXX these suck. i'm sure theres a far better way to do this */
#define ftof9(x) (uint16)((x)*512.0f)
#define f9tof(x) ((float)(x)/512.0f)

#define MAX(x,y) ((x)>(y)?(x):(y))

#define ALLEGRO_NO_MAGIC_MAIN
#define USE_CONSOLE
#include <allegro.h>

#include "membufBrowser.h"

#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "prinrval.h"
#include "plgetopt.h"


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#ifdef WIN32
#include <winsock2.h>
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#include <sys/io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>

#define NCHAN_LIMIT     20

#define DEFAULT_FILENAME "mozURLimg-out.tga"

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 600

int main(int argc, char** argv)
{
#ifdef WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,0), &wsaData) != 0) {
        return 0;
    }
#endif

    char* url = 0;
    char* filename = DEFAULT_FILENAME;
    int width = DEFAULT_WIDTH;
    int height = DEFAULT_HEIGHT;
    nsresult rv = NS_ERROR_FAILURE;

    PLOptState* pl = PL_CreateOptState(argc, argv, "o:w:");
    while(PL_GetNextOpt(pl) == PL_OPT_OK) {
        switch(pl->option) {
        case 0:
            url = strdup(pl->value);
            break;
        case 'o':
            filename = strdup(pl->value);
            break;
        case 'w':
            width = atoi(pl->value);
            break;
        case 'h':
            height = atoi(pl->value);
            break;
        }
    }

    if(url) {
        /* Initialize the embedded browser */
        static membufBrowser* browser = new membufBrowser();
        NS_ADDREF(browser);
        if (browser)
          printf("XXXjgaunt - we have a browser\n");

        rv = browser->Init();
        if (NS_SUCCEEDED(rv))
          printf("XXXjgaunt - passed Init()\n");


        NS_ENSURE_SUCCESS(rv, 0);

        printf("Load page at %s\n", url);

        /* Resize the embedded browser to match the device */
	rv = browser->SetViewportSize(width, height);
        if (NS_SUCCEEDED(rv))
          printf("XXXjgaunt - successfully resized\n");


        /* Load the URI synchronously */
        rv = browser->LoadURI(url, filename);
        if (NS_SUCCEEDED(rv))
          printf("XXXjgaunt - loaded the page!!!\n");

        if (NS_FAILED(rv)) {
            printf("Failed loading the page\n");
        }

        //nscoord w = 0, h = 0;
        //gBrowser->GetCanvasSize(&w, &h);
        NS_RELEASE(browser);
    } else {
        printf("Mozilla URL to Image\n");
        printf("Use Mozilla membuf extension to render HTML page to an image file.\n");
        printf("(c) 2003 Peter Amstutz <tetron@interreality.org>\n");
        printf("         Stuart Parmenter <pavlov@netscape.com>\n");
        printf("         Joe Hewitt <hewitt@netscape.com>\n");
        printf("This is free software available under the Mozilla Public License (MPL)\n");
        printf("\n");
        printf("Usage: \n");
        printf("mozURLimg [-o filename] [-w int] [-h int] <url>\n");
        printf("  Options:\n");
        printf("    -o Filename to save image to, default '%s'\n", DEFAULT_FILENAME);
        printf("    -w Preferred page width, in pixels, default %i\n", DEFAULT_WIDTH);
        printf("    -h Preferred page height, in pixels, default %i\n", DEFAULT_HEIGHT);
        printf("Note: output will be a 32-bit Targa (.tga) file.\n");
    }

    if (NS_SUCCEEDED(rv))
      printf("XXXjgaunt - We're DONE!!!\n");

#ifdef WIN32
    WSACleanup();
#endif

    return 1;
}

END_OF_MAIN()
