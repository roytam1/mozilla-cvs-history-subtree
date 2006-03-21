// license block 

// membuf embed headers
#include "BrowserContainerImpl.h"

// mozilla headers
#include "plgetopt.h"

// platform headers
#include <stdio.h>

#define DEFAULT_FILENAME  "membuf-browser-out.tga"
#define DEFAULT_URI       "http://www.mozilla.com/"
#define DEFAULT_WIDTH     800
#define DEFAULT_HEIGHT    600

int main(int argc, char** argv)
{
    char* url = DEFAULT_URI;
    char* filename = DEFAULT_FILENAME;
    int width = DEFAULT_WIDTH;
    int height = DEFAULT_HEIGHT;
    nsresult rv = NS_ERROR_FAILURE;

    PLOptState* pl = PL_CreateOptState(argc, argv, "uo:w:h:");
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
        case 'u':
            printf("um, output");
            url = 0;
            break;
        }
    }

    if (url) {
  
        BrowserContainerImpl *container = new BrowserContainerImpl();
        NS_ENSURE_ARG_POINTER(container);  // just checks for non-null

        // initialize the container
        container->Init(width, height);

        // create the browser
        container->CreateBrowser();

        // tell the container what file to dump to
        container->SetOutputFile(filename);

        // set the size of the browser window
        //container->SetDimensions( width, height );

        // load the passed in url
        container->LoadURI(url);

        // have the browser load and display the page
        container->Run();

    } else {
        printf("Membuf Browser\n");
        printf("Use Mozilla membuf extension to render HTML page to memory.\n");
        printf("(c) 2006 John Gaunt <mozilla@jgaunt.com>\n");
        printf("(c) 2003 Peter Amstutz <tetron@interreality.org>\n");
        printf("         Stuart Parmenter <pavlov@netscape.com>\n");
        printf("         Joe Hewitt <hewitt@netscape.com>\n");
        printf("This is free software available under the Mozilla Public License (MPL)\n");
        printf("\n");
        printf("Usage: \n");
        printf("memBrowEmbed [-u] [-o filename] [-w int] [-h int] <url>\n");
        printf("  Options:\n");
        printf("    -u Usage: display this message\n");
        printf("    -o Filename to save image to, default '%s'\n", DEFAULT_FILENAME);
        printf("    -w Preferred page width, in pixels, default %i\n", DEFAULT_WIDTH);
        printf("    -h Preferred page height, in pixels, default %i\n", DEFAULT_HEIGHT);
        printf("Note: output will be a 32-bit Targa (.tga) file.\n");
    }

    if (NS_SUCCEEDED(rv))
      printf("XXXjgaunt - We're DONE!!!\n");

    return 1;
}

//END_OF_MAIN()
