/* -*- Mode: C; tab-width: 4; -*- */
/*******************************************************************************
 * npsimple.c
 ******************************************************************************
 * Simple LiveConnect Sample Plugin
 * Copyright (c) 1996 Netscape Communications. All rights reserved.
 ******************************************************************************
 * OVERVIEW
 * --------
 * Section 1 - Includes
 * Section 2 - Instance Structs
 * Section 3 - API Plugin Implementations
 * Section 4 - Java Native Method Implementations
 * Section 5 - Utility Method Implementations
 *******************************************************************************/


/*******************************************************************************
 * SECTION 1 - Includes
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "prtypes.h"
#include "npapi.h"
#include "npupp.h"

/*------------------------------------------------------------------------------
 * Windows Includes
 *----------------------------------------------------------------------------*/
#ifdef _WINDOWS /* Windows Includes */
#include <windows.h>
#endif /* _WINDOWS */
/*------------------------------------------------------------------------------
 * UNIX includes
 *----------------------------------------------------------------------------*/
#ifdef XP_UNIX
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#endif /* XP_UNIX */

FILE* log = NULL;

/*******************************************************************************
 * SECTION 2 - Instance Structs
 *******************************************************************************
 * Instance state information about the plugin.
 *
 * PLUGIN DEVELOPERS:
 *	Use this struct to hold per-instance information that you'll
 *	need in the various functions in this file.
 ******************************************************************************
 * First comes the PlatformInstance struct, which contains platform specific
 * information for each instance.
 *****************************************************************************/
 
/*------------------------------------------------------------------------------
 * Windows PlatformInstance
 *----------------------------------------------------------------------------*/
#ifdef XP_PC
typedef struct _PlatformInstance
{
	HWND			fhWnd;
	WNDPROC			fDefaultWindowProc;
} PlatformInstance;
#endif /* XP_PC */
/*------------------------------------------------------------------------------
 * UNIX PlatformInstance
 *----------------------------------------------------------------------------*/
#ifdef XP_UNIX
typedef struct _PlatformInstance
{
    Window 			window;
    Display *		display;
    uint32 			x, y;
    uint32 			width, height;
} PlatformInstance;
#endif /* XP_UNIX */
/*------------------------------------------------------------------------------
 * Macintosh PlatformInstance
 *----------------------------------------------------------------------------*/
#ifdef XP_MAC
typedef struct _PlatformInstance
{
	int				placeholder;
} PlatformInstance;
#endif /* macintosh */
/*------------------------------------------------------------------------------
 * Cross-Platform PluginInstance
 *----------------------------------------------------------------------------*/
typedef struct _PluginInstance
{
	NPWindow*			fWindow;
	uint16				fMode;
	PlatformInstance	fPlatform;
} PluginInstance;


/******************************************************************************
 * Method Declarations
 ******************************************************************************/
void		PlatformNew( PluginInstance* This );
NPError		PlatformDestroy( PluginInstance* This );
NPError		PlatformSetWindow( PluginInstance* This, NPWindow* window );
int16		PlatformHandleEvent( PluginInstance* This, void* event );

/*******************************************************************************
 * SECTION 3 - API Plugin Implementations
 ******************************************************************************/
 
/*------------------------------------------------------------------------------
 * UNIX-only API calls
 *----------------------------------------------------------------------------*/
#ifdef XP_UNIX
/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * NPP_GetMIMEDescription:
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
char* NPP_GetMIMEDescription(void)
{
	return("application/x-simple-plugin:smp:Simple LiveConnect Sample Plug-in");
}

#define PLUGIN_NAME			"Simple LiveConnect Sample Plug-in"
#define PLUGIN_DESCRIPTION	"Demonstrates a simple LiveConnected plug-in."
/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * NPP_GetValue:
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
NPError
NPP_GetValue(void *future, NPPVariable variable, void *value)
{
    NPError err = NPERR_NO_ERROR;
    if (variable == NPPVpluginNameString)
		*((char **)value) = PLUGIN_NAME;
    else if (variable == NPPVpluginDescriptionString)
		*((char **)value) = PLUGIN_DESCRIPTION;
    else
		err = NPERR_GENERIC_ERROR;

    return err;
}
#endif /* XP_UNIX */

/*------------------------------------------------------------------------------
 * Cross-Platform Plug-in API Calls
 *----------------------------------------------------------------------------*/
 
/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * NPP_Initialize:
 * Provides global initialization for a plug-in, and returns an error value. 
 *
 * This function is called once when a plug-in is loaded, before the first instance
 * is created. You should allocate any memory or resources shared by all
 * instances of your plug-in at this time. After the last instance has been deleted,
 * NPP_Shutdown will be called, where you can release any memory or
 * resources allocated by NPP_Initialize. 
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
NPError OSCALL
NP_Initialize(NPNetscapeFuncs* p)
{
    fprintf(log, "NP_Initialize()\n");
    return NPERR_NO_ERROR;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * NPP_Shutdown:
 * Provides global deinitialization for a plug-in. 
 * 
 * This function is called once after the last instance of your plug-in is destroyed.
 * Use this function to release any memory or resources shared across all
 * instances of your plug-in. You should be a good citizen and declare that
 * you're not using your java class any more. This allows java to unload
 * it, freeing up memory.
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
NPError OSCALL
NP_Shutdown(void)
{
    fprintf(log, "NP_Shutdown()\n");
    fclose(log);
    return NPERR_NO_ERROR;
}


NPError OSCALL
NP_GetEntryPoints(NPPluginFuncs* p)
{
    log = fopen("npsimp40.log", "w");
    fprintf(log, "NP_GetEntryPoints()\n");
    p->newp          = NPP_New;
    p->destroy       = NPP_Destroy;
    p->setwindow     = NPP_SetWindow;
    p->newstream     = NPP_NewStream;
    p->destroystream = NPP_DestroyStream;
    p->asfile        = NPP_StreamAsFile;
    p->writeready    = NPP_WriteReady;
    p->write         = NPP_Write;
    p->print         = NPP_Print;
    p->event         = NULL;
    p->urlnotify     = NPP_URLNotify;
    p->javaClass     = NULL;
    p->getvalue      = NULL;
    p->setvalue      = NULL;
    return NPERR_NO_ERROR;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * NPP_New:
 * Creates a new instance of a plug-in and returns an error value. 
 * 
 * NPP_New creates a new instance of your plug-in with MIME type specified
 * by pluginType. The parameter mode is NP_EMBED if the instance was created
 * by an EMBED tag, or NP_FULL if the instance was created by a separate file.
 * You can allocate any instance-specific private data in instance->pdata at this
 * time. The NPP pointer is valid until the instance is destroyed. 
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
NPError 
NPP_New(NPMIMEType pluginType,
	NPP instance,
	uint16 mode,
	int16 argc,
	char* argn[],
	char* argv[],
	NPSavedData* saved)
{
	NPError result = NPERR_NO_ERROR;
	PluginInstance* This;

    fprintf(log, "NPP_New()\n");

	if (instance == NULL) {
		return NPERR_INVALID_INSTANCE_ERROR;
	}
	instance->pdata = malloc(sizeof(PluginInstance));
	This = (PluginInstance*) instance->pdata;
	if (This == NULL) {
	    return NPERR_OUT_OF_MEMORY_ERROR;
	}
	/* mode is NP_EMBED, NP_FULL, or NP_BACKGROUND (see npapi.h) */
	This->fWindow = NULL;
	This->fMode = mode;
	PlatformNew( This ); 	/* Call Platform-specific initializations */

		/* PLUGIN DEVELOPERS:
		 *	Initialize fields of your plugin
		 *	instance data here.  If the NPSavedData is non-
		 *	NULL, you can use that data (returned by you from
		 *	NPP_Destroy to set up the new plugin instance).
		 */

	return result;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * NPP_Destroy:
 * Deletes a specific instance of a plug-in and returns an error value. 

 * NPP_Destroy is called when a plug-in instance is deleted, typically because the
 * user has left the page containing the instance, closed the window, or quit the
 * application. You should delete any private instance-specific information stored
 * in instance->pdata. If the instance being deleted is the last instance created
 * by your plug-in, NPP_Shutdown will subsequently be called, where you can
 * delete any data allocated in NPP_Initialize to be shared by all your plug-in's
 * instances. Note that you should not perform any graphics operations in
 * NPP_Destroy as the instance's window is no longer guaranteed to be valid. 
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
NPError 
NPP_Destroy(NPP instance, NPSavedData** save)
{
	PluginInstance* This;
    fprintf(log, "NPP_Destroy()\n");

	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	This = (PluginInstance*) instance->pdata;
	PlatformDestroy( This ); /* Perform platform specific cleanup */
	
	/* PLUGIN DEVELOPERS:
	 *	If desired, call NP_MemAlloc to create a
	 *	NPSavedDate structure containing any state information
	 *	that you want restored if this plugin instance is later
	 *	recreated.
	 */

	if (This != NULL) {
		free(instance->pdata);
		instance->pdata = NULL;
	}

	return NPERR_NO_ERROR;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * NPP_SetWindow:
 * Sets the window in which a plug-in draws, and returns an error value. 
 * 
 * NPP_SetWindow informs the plug-in instance specified by instance of the
 * the window denoted by window in which the instance draws. This NPWindow
 * pointer is valid for the life of the instance, or until NPP_SetWindow is called
 * again with a different value. Subsequent calls to NPP_SetWindow for a given
 * instance typically indicate that the window has been resized. If either window
 * or window->window are NULL, the plug-in must not perform any additional
 * graphics operations on the window and should free any resources associated
 * with the window. 
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
NPError 
NPP_SetWindow(NPP instance, NPWindow* window)
{
	NPError result = NPERR_NO_ERROR;
	PluginInstance* This;

    fprintf(log, "NPP_SetWindow()\n");

	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	This = (PluginInstance*) instance->pdata;

	/*
	 * PLUGIN DEVELOPERS:
	 *	Before setting window to point to the
	 *	new window, you may wish to compare the new window
	 *	info to the previous window (if any) to note window
	 *	size changes, etc.
	 */
	result = PlatformSetWindow( This, window );
	
	This->fWindow = window;
	return result;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * NPP_NewStream:
 * Notifies an instance of a new data stream and returns an error value. 
 * 
 * NPP_NewStream notifies the instance denoted by instance of the creation of
 * a new stream specifed by stream. The NPStream* pointer is valid until the
 * stream is destroyed. The MIME type of the stream is provided by the
 * parameter type. 
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
NPError 
NPP_NewStream(NPP instance,
	      NPMIMEType type,
	      NPStream *stream, 
	      NPBool seekable,
	      uint16 *stype)
{
	PluginInstance* This;

    fprintf(log, "NPP_NewStream()\n");

	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	This = (PluginInstance*) instance->pdata;

	return NPERR_NO_ERROR;
}


/* PLUGIN DEVELOPERS:
 *	These next 2 functions are directly relevant in a plug-in which
 *	handles the data in a streaming manner. If you want zero bytes
 *	because no buffer space is YET available, return 0. As long as
 *	the stream has not been written to the plugin, Navigator will
 *	continue trying to send bytes.  If the plugin doesn't want them,
 *	just return some large number from NPP_WriteReady(), and
 *	ignore them in NPP_Write().  For a NP_ASFILE stream, they are
 *	still called but can safely be ignored using this strategy.
 */

int32 STREAMBUFSIZE = 0X0FFFFFFF; /* If we are reading from a file in NPAsFile
								   * mode so we can take any size stream in our
								   * write call (since we ignore it) */

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * NPP_WriteReady:
 * Returns the maximum number of bytes that an instance is prepared to accept
 * from the stream. 
 * 
 * NPP_WriteReady determines the maximum number of bytes that the
 * instance will consume from the stream in a subsequent call NPP_Write. This
 * function allows Netscape to only send as much data to the instance as the
 * instance is capable of handling at a time, allowing more efficient use of
 * resources within both Netscape and the plug-in. 
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
int32 
NPP_WriteReady(NPP instance, NPStream *stream)
{
	PluginInstance* This;
    fprintf(log, "NPP_WriteReady()\n");

	if (instance != NULL)
		This = (PluginInstance*) instance->pdata;

	/* Number of bytes ready to accept in NPP_Write() */
	return STREAMBUFSIZE;
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * NPP_Write:
 * Delivers data from a stream and returns the number of bytes written. 
 * 
 * NPP_Write is called after a call to NPP_NewStream in which the plug-in
 * requested a normal-mode stream, in which the data in the stream is delivered
 * progressively over a series of calls to NPP_WriteReady and NPP_Write. The
 * function delivers a buffer buf of len bytes of data from the stream identified
 * by stream to the instance. The parameter offset is the logical position of
 * buf from the beginning of the data in the stream. 
 * 
 * The function returns the number of bytes written (consumed by the instance).
 * A negative return value causes an error on the stream, which will
 * subsequently be destroyed via a call to NPP_DestroyStream. 
 * 
 * Note that a plug-in must consume at least as many bytes as it indicated in the
 * preceeding NPP_WriteReady call. All data consumed must be either processed
 * immediately or copied to memory allocated by the plug-in: the buf parameter
 * is not persistent. 
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
int32 
NPP_Write(NPP instance, NPStream *stream, int32 offset, int32 len, void *buffer)
{
    fprintf(log, "NPP_Write()\n");
	if (instance != NULL) {
		PluginInstance* This = (PluginInstance*) instance->pdata;
	}
	return len;		/* The number of bytes accepted */
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * NPP_DestroyStream:
 * Indicates the closure and deletion of a stream, and returns an error value. 
 * 
 * The NPP_DestroyStream function is called when the stream identified by
 * stream for the plug-in instance denoted by instance will be destroyed. You
 * should delete any private data allocated in stream->pdata at this time. 
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
NPError 
NPP_DestroyStream(NPP instance, NPStream *stream, NPError reason)
{
	PluginInstance* This;

    fprintf(log, "NPP_DestroyStream()\n");
	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;
	This = (PluginInstance*) instance->pdata;

	return NPERR_NO_ERROR;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * NPP_StreamAsFile:
 * Provides a local file name for the data from a stream. 
 * 
 * NPP_StreamAsFile provides the instance with a full path to a local file,
 * identified by fname, for the stream specified by stream. NPP_StreamAsFile is
 * called as a result of the plug-in requesting mode NP_ASFILEONLY or
 * NP_ASFILE in a previous call to NPP_NewStream. If an error occurs while
 * retrieving the data or writing the file, fname may be NULL. 
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
void 
NPP_StreamAsFile(NPP instance, NPStream *stream, const char* fname)
{
	PluginInstance* This;
    fprintf(log, "NPP_StreamAsFile()\n");
	if (instance != NULL)
		This = (PluginInstance*) instance->pdata;

}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * NPP_Print:
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
void 
NPP_Print(NPP instance, NPPrint* printInfo)
{
    fprintf(log, "NPP_Print()\n");
	if(printInfo == NULL)
		return;

	if (instance != NULL) {
		PluginInstance* This = (PluginInstance*) instance->pdata;
	
		if (printInfo->mode == NP_FULL) {
		    /*
		     * PLUGIN DEVELOPERS:
		     *	If your plugin would like to take over
		     *	printing completely when it is in full-screen mode,
		     *	set printInfo->pluginPrinted to TRUE and print your
		     *	plugin as you see fit.  If your plugin wants Netscape
		     *	to handle printing in this case, set
		     *	printInfo->pluginPrinted to FALSE (the default) and
		     *	do nothing.  If you do want to handle printing
		     *	yourself, printOne is true if the print button
		     *	(as opposed to the print menu) was clicked.
		     *	On the Macintosh, platformPrint is a THPrint; on
		     *	Windows, platformPrint is a structure
		     *	(defined in npapi.h) containing the printer name, port,
		     *	etc.
		     */

			void* platformPrint =
				printInfo->print.fullPrint.platformPrint;
			NPBool printOne =
				printInfo->print.fullPrint.printOne;
			
			/* Do the default*/
			printInfo->print.fullPrint.pluginPrinted = FALSE;
		}
		else {	/* If not fullscreen, we must be embedded */
		    /*
		     * PLUGIN DEVELOPERS:
		     *	If your plugin is embedded, or is full-screen
		     *	but you returned false in pluginPrinted above, NPP_Print
		     *	will be called with mode == NP_EMBED.  The NPWindow
		     *	in the printInfo gives the location and dimensions of
		     *	the embedded plugin on the printed page.  On the
		     *	Macintosh, platformPrint is the printer port; on
		     *	Windows, platformPrint is the handle to the printing
		     *	device context.
		     */

			NPWindow* printWindow =
				&(printInfo->print.embedPrint.window);
			void* platformPrint =
				printInfo->print.embedPrint.platformPrint;
		}
	}
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * NPP_URLNotify:
 * Notifies the instance of the completion of a URL request. 
 * 
 * NPP_URLNotify is called when Netscape completes a NPN_GetURLNotify or
 * NPN_PostURLNotify request, to inform the plug-in that the request,
 * identified by url, has completed for the reason specified by reason. The most
 * common reason code is NPRES_DONE, indicating simply that the request
 * completed normally. Other possible reason codes are NPRES_USER_BREAK,
 * indicating that the request was halted due to a user action (for example,
 * clicking the "Stop" button), and NPRES_NETWORK_ERR, indicating that the
 * request could not be completed (for example, because the URL could not be
 * found). The complete list of reason codes is found in npapi.h. 
 * 
 * The parameter notifyData is the same plug-in-private value passed as an
 * argument to the corresponding NPN_GetURLNotify or NPN_PostURLNotify
 * call, and can be used by your plug-in to uniquely identify the request. 
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
void
NPP_URLNotify(NPP instance, const char* url, NPReason reason, void* notifyData)
{
	/* Not used in the Simple plugin. */
    fprintf(log, "NPP_URLNotify()\n");
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * NPP_HandleEvent:
 * Mac-only, but stub must be present for Windows
 * Delivers a platform-specific event to the instance. 
 * 
 * On the Macintosh, event is a pointer to a standard Macintosh EventRecord.
 * All standard event types are passed to the instance as appropriate. In general,
 * return TRUE if you handle the event and FALSE if you ignore the event. 
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
#ifndef XP_UNIX
int16
NPP_HandleEvent(NPP instance, void* event)
{
	PluginInstance* This;
	int16 eventHandled = FALSE;
	
    fprintf(log, "NPP_HandleEvent()\n");

	if (instance == NULL)
		return eventHandled;
	
	This = (PluginInstance*) instance->pdata;
	eventHandled = PlatformHandleEvent(This, event);
	return eventHandled;
}
#endif /* ndef XP_UNIX */
/*******************************************************************************/





/*******************************************************************************
 * SECTION 5 - Utility Method Implementations
 ******************************************************************************/

/*------------------------------------------------------------------------------
 * Platform-Specific Implemenations
 *------------------------------------------------------------------------------
 * UNIX Implementations
 *----------------------------------------------------------------------------*/
#ifdef XP_UNIX
void Redraw(Widget w, XtPointer closure, XEvent *event);

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * PlatformNew
 *
 * Initialize any Platform-Specific instance data.
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
void
PlatformNew( PluginInstance* This )
{
	This->fPlatform.window = 0;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * PlatformDestroy
 *
 * Destroy any Platform-Specific instance data.
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
NPError
PlatformDestroy( PluginInstance* This )
{
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * PlatformSetWindow
 *
 * Perform platform-specific window operations
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
NPError
PlatformSetWindow( PluginInstance* This, NPWindow* window )
{
	Widget netscape_widget;

	This->fPlatform.window = (Window) window->window;
	This->fPlatform.x = window->x;
	This->fPlatform.y = window->y;
	This->fPlatform.width = window->width;
	This->fPlatform.height = window->height;
	This->fPlatform.display = ((NPSetWindowCallbackStruct *)window->ws_info)->display;
	
    netscape_widget = XtWindowToWidget(This->fPlatform.display, This->fPlatform.window);
	XtAddEventHandler(netscape_widget, ExposureMask, FALSE, (XtEventHandler)Redraw, This);
	Redraw(netscape_widget, (XtPointer)This, NULL);
	return NPERR_NO_ERROR;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * PlatformHandleEvent
 *
 * Handle platform-specific events.
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
int16
PlatformHandleEvent( PluginInstance* This, void* event )
{
	/* UNIX Plugins do not use HandleEvent */
	return 0;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * Redraw
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
void
Redraw(Widget w, XtPointer closure, XEvent *event)
{
	PluginInstance* This = (PluginInstance*)closure;
	GC gc;
	XGCValues gcv;
	const char* text = "Hello World";

	XtVaGetValues(w, XtNbackground, &gcv.background,
				  XtNforeground, &gcv.foreground, 0);
	gc = XCreateGC(This->fPlatform.display, This->fPlatform.window, 
				   GCForeground|GCBackground, &gcv);
	XDrawRectangle(This->fPlatform.display, This->fPlatform.window, gc, 
				   0, 0, This->fPlatform.width-1, This->fPlatform.height-1);
	XDrawString(This->fPlatform.display, This->fPlatform.window, gc, 
				This->fPlatform.width/2 - 100, This->fPlatform.height/2,
				text, strlen(text));
}
#endif /* XP_UNIX */

/*------------------------------------------------------------------------------
 * Windows Implementations
 *----------------------------------------------------------------------------*/
#ifdef XP_PC
LRESULT CALLBACK PluginWindowProc( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
const char* gInstanceLookupString = "instance->pdata";

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * PlatformNew
 *
 * Initialize any Platform-Specific instance data.
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
void
PlatformNew( PluginInstance* This )
{
	This->fPlatform.fhWnd = NULL;
	This->fPlatform.fDefaultWindowProc = NULL;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * PlatformDestroy
 *
 * Destroy any Platform-Specific instance data.
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
NPError
PlatformDestroy( PluginInstance* This )
{

	if( This->fWindow != NULL ) { /* If we have a window, clean

								 * it up. */

		SetWindowLong( This->fPlatform.fhWnd, GWL_WNDPROC, (LONG)This->fPlatform.fDefaultWindowProc);

		This->fPlatform.fDefaultWindowProc = NULL;

		This->fPlatform.fhWnd = NULL;

	}



	return NPERR_NO_ERROR;

}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * PlatformSetWindow
 *
 * Perform platform-specific window operations
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
NPError
PlatformSetWindow( PluginInstance* This, NPWindow* window )
{
	if( This->fWindow != NULL ) /* If we already have a window, clean
								 * it up before trying to subclass
								 * the new window. */
	{
		if( (window == NULL) || ( window->window == NULL ) ) {
			/* There is now no window to use. get rid of the old
			 * one and exit. */
			SetWindowLong( This->fPlatform.fhWnd, GWL_WNDPROC, (LONG)This->fPlatform.fDefaultWindowProc);
			This->fPlatform.fDefaultWindowProc = NULL;
			This->fPlatform.fhWnd = NULL;
			return NPERR_NO_ERROR;
		}

		else if ( This->fPlatform.fhWnd == (HWND) window->window ) {
			/* The new window is the same as the old one. Exit now. */
			return NPERR_NO_ERROR;
		}
		else {
			/* Clean up the old window, so that we can subclass the new
			 * one later. */
			SetWindowLong( This->fPlatform.fhWnd, GWL_WNDPROC, (LONG)This->fPlatform.fDefaultWindowProc);
			This->fPlatform.fDefaultWindowProc = NULL;
			This->fPlatform.fhWnd = NULL;
		}
	}
	else if( (window == NULL) || ( window->window == NULL ) ) {
		/* We can just get out of here if there is no current
		 * window and there is no new window to use. */
		return NPERR_NO_ERROR;
	}

	/* At this point, we will subclass
	 * window->window so that we can begin drawing and
	 * receiving window messages. */
	This->fPlatform.fDefaultWindowProc = (WNDPROC)SetWindowLong( (HWND)window->window, GWL_WNDPROC, (LONG)PluginWindowProc);
	This->fPlatform.fhWnd = (HWND) window->window;
	SetProp( This->fPlatform.fhWnd, gInstanceLookupString, (HANDLE)This);

	InvalidateRect( This->fPlatform.fhWnd, NULL, TRUE );
	UpdateWindow( This->fPlatform.fhWnd );

        return NPERR_NO_ERROR;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * PlatformHandleEvent
 *
 * Handle platform-specific events.
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
int16
PlatformHandleEvent( PluginInstance* This, void* event )
{
	/* Windows Plugins use the Windows event call-back mechanism
	   for events. (See PluginWindowProc) */
	return 0;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * PluginWindowProc
 *
 * Handle the Windows window-event loop.
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
LRESULT CALLBACK PluginWindowProc( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	PluginInstance* This = (PluginInstance*) GetProp(hWnd, gInstanceLookupString);

	switch( Msg ) {
		case WM_PAINT: {
			PAINTSTRUCT paintStruct;
			HDC hdc;

			hdc = BeginPaint( hWnd, &paintStruct );
			TextOut(hdc, 0, 0, "Hello, World!", 15);

			EndPaint( hWnd, &paintStruct );
			break;
		}
		default: {
			This->fPlatform.fDefaultWindowProc( hWnd, Msg, wParam, lParam);
		}
	}
	return 0;
}
#endif /* XP_PC */




/*------------------------------------------------------------------------------
 * Macintosh Implementations
 *----------------------------------------------------------------------------*/
#ifdef macintosh
NPBool	StartDraw(NPWindow* window);
void 	EndDraw(NPWindow* window);
void 	DoDraw(PluginInstance* This);

CGrafPort 		gSavePort;
CGrafPtr		gOldPort;

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * PlatformNew
 *
 * Initialize any Platform-Specific instance data.
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
void
PlatformNew( PluginInstance* This )
{
	return;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * PlatformDestroy
 *
 * Destroy any Platform-Specific instance data.
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
NPError
PlatformDestroy( PluginInstance* This )
{
	return NPERR_NO_ERROR;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * PlatformSetWindow
 *
 * Perform platform-specific window operations
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
NPError
PlatformSetWindow( PluginInstance* This, NPWindow* window )
{
	This->fWindow = window;
	if( StartDraw( window ) ) {
		DoDraw(This);
		EndDraw( window );
	}
	return NPERR_NO_ERROR;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * PlatformHandleEvent
 *
 * Handle platform-specific events.
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
int16
PlatformHandleEvent( PluginInstance* This, void* event )
{
	int16 eventHandled = FALSE;
	
	EventRecord* ev = (EventRecord*) event;
	if (This != NULL && event != NULL)
	{
		switch (ev->what)
		{
			/*
			 * Draw ourselves on update events
			 */
			case updateEvt:
				if( StartDraw( This->fWindow ) ) {
					DoDraw(This);
					EndDraw( This->fWindow );
				}
				eventHandled = true;
				break;
			default:
				break;
		}
	}
	return eventHandled;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * StartDraw
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
NPBool
StartDraw(NPWindow* window)
{
	NP_Port* port;
	Rect clipRect;
	RGBColor  col;
	
	if (window == NULL)
		return FALSE;
	port = (NP_Port*) window->window;
	if (window->clipRect.left < window->clipRect.right)
	{
	/* Preserve the old port */
		GetPort((GrafPtr*)&gOldPort);
		SetPort((GrafPtr)port->port);
	/* Preserve the old drawing environment */
		gSavePort.portRect = port->port->portRect;
		gSavePort.txFont = port->port->txFont;
		gSavePort.txFace = port->port->txFace;
		gSavePort.txMode = port->port->txMode;
		gSavePort.rgbFgColor = port->port->rgbFgColor;
		gSavePort.rgbBkColor = port->port->rgbBkColor;
		GetClip(gSavePort.clipRgn);
	/* Setup our drawing environment */
		clipRect.top = window->clipRect.top + port->porty;
		clipRect.left = window->clipRect.left + port->portx;
		clipRect.bottom = window->clipRect.bottom + port->porty;
		clipRect.right = window->clipRect.right + port->portx;
		SetOrigin(port->portx,port->porty);
		ClipRect(&clipRect);
		clipRect.top = clipRect.left = 0;
		TextSize(12);
		TextFont(geneva);
		TextMode(srcCopy);
		col.red = col.green = col.blue = 0;
		RGBForeColor(&col);
		col.red = col.green = col.blue = 65000;
		RGBBackColor(&col);
		return TRUE;
	}
	else
		return FALSE;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * EndDraw
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
void
EndDraw(NPWindow* window)
{
	CGrafPtr myPort;
	NP_Port* port = (NP_Port*) window->window;
	SetOrigin(gSavePort.portRect.left, gSavePort.portRect.top);
	SetClip(gSavePort.clipRgn);
	GetPort((GrafPtr*)&myPort);
	myPort->txFont = gSavePort.txFont;
	myPort->txFace = gSavePort.txFace;
	myPort->txMode = gSavePort.txMode;
	RGBForeColor(&gSavePort.rgbFgColor);
	RGBBackColor(&gSavePort.rgbBkColor);
	SetPort((GrafPtr)gOldPort);
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * DoDraw
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
void
DoDraw(PluginInstance* This)
{
	Rect drawRect;
	drawRect.top = 0;
	drawRect.left = 0;
	drawRect.bottom = drawRect.top + This->fWindow->height;
	drawRect.right = drawRect.left + This->fWindow->width;
	EraseRect( &drawRect );
	MoveTo( 2, 12 );
	DrawString("\pHello, World!");
}
#endif /* macintosh */

/******************************************************************************/
