/* -*- Mode: C++; tab-width: 8; -*- */
/*******************************************************************************
 * npsimple.cpp
 ******************************************************************************
 * Simple Sample Plugin
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
#include "nsplugin.h"
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
    HWND		fhWnd;
    WNDPROC		fDefaultWindowProc;
} PlatformInstance;
#endif /* XP_PC */

/*------------------------------------------------------------------------------
 * UNIX PlatformInstance
 *----------------------------------------------------------------------------*/

#ifdef XP_UNIX
typedef struct _PlatformInstance
{
    Window 		window;
    Display *		display;
    uint32 		x, y;
    uint32 		width, height;
} PlatformInstance;
#endif /* XP_UNIX */

/*------------------------------------------------------------------------------
 * Macintosh PlatformInstance
 *----------------------------------------------------------------------------*/

#ifdef XP_MAC
typedef struct _PlatformInstance
{
    int			placeholder;
} PlatformInstance;
#endif /* macintosh */

////////////////////////////////////////////////////////////////////////////////
// Simple Plugin Classes
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// SimplePlugin represents the class of all simple plugins. One 
// instance of this class is kept around for as long as there are
// plugin instances outstanding.

class SimplePlugin : public NPIPlugin {
public:

    SimplePlugin(NPIPluginManager* mgr);
    virtual ~SimplePlugin(void);

    NS_DECL_ISUPPORTS
    
    ////////////////////////////////////////////////////////////////////////////
    // from NPIPlugin:

    virtual NPPluginError NP_LOADDS
    NewInstance(NPIPluginInstancePeer* peer, NPIPluginInstance* *result);

#ifdef XP_UNIX

    // (Corresponds to NPP_GetMIMEDescription.)
    virtual char* NP_LOADDS
    GetMIMEDescription(void);

#endif /* XP_UNIX */

    ////////////////////////////////////////////////////////////////////////////
    // SimplePlugin specific methods:

    NPIPluginManager* GetPluginManager(void) { return mgr; }

protected:
    NPIPluginManager* mgr;

};

////////////////////////////////////////////////////////////////////////////////
// SimplePluginInstance represents an instance of the SimplePlugin class.

class SimplePluginInstance : public NPIPluginInstance {
public:

    SimplePluginInstance(NPIPluginInstancePeer* peer, PRUint16 mode);
    virtual ~SimplePluginInstance(void);

    NS_DECL_ISUPPORTS

    ////////////////////////////////////////////////////////////////////////////
    // from NPIPluginInstance:

    // The Release method on NPIPluginInstance corresponds to NPP_Destroy.
    
    virtual NPIPluginInstancePeer* NP_LOADDS
    GetPeer(void);

    // See comment for NPIPlugin::NewInstance, above.
    virtual NPPluginError NP_LOADDS
    Start(void);

    // The old NPP_Destroy call has been factored into two plugin instance methods:
    //
    // Stop -- called when the plugin instance is to be stopped (e.g. by displaying
    // another browser window, causing the page containing the plugin to become
    // removed from the display).
    //
    // Release -- called once, before the plugin instance peer is to be destroyed.
    // This method is used to destroy the plugin instance.

    virtual NPPluginError NP_LOADDS
    Stop(void);

    // (Corresponds to NPP_SetWindow.)
    virtual NPPluginError NP_LOADDS
    SetWindow(NPPluginWindow* window);

    // XXX Should this return a void* or nsISupports*, and the caller be
    // required to QueryInterface for a NPIPluginStream*? That way we can
    // allow other stream implementations later.
    // (Corresponds to NPP_NewStream.)
    virtual NPPluginError NP_LOADDS
    NewStream(NPIPluginStreamPeer* peer, NPIPluginStream* *result);

    // (Corresponds to NPP_Print.)
    virtual void NP_LOADDS
    Print(NPPluginPrint* platformPrint);

    // (Corresponds to NPP_HandleEvent.)
    virtual PRInt16 NP_LOADDS
    HandleEvent(NPPluginEvent* event);

    // (Corresponds to NPP_URLNotify.)
    virtual void NP_LOADDS
    URLNotify(const char* url, const char* target,
              NPPluginReason reason, void* notifyData);

    // (Corresponds to NPP_GetValue.)
    virtual NPPluginError NP_LOADDS
    GetValue(NPPluginVariable variable, void *value);

    // (Corresponds to NPP_SetValue.)
    virtual NPPluginError NP_LOADDS
    SetValue(NPPluginManagerVariable variable, void *value);

    ////////////////////////////////////////////////////////////////////////////
    // SimplePluginInstance specific methods:

    void            PlatformNew(void);
    NPPluginError	PlatformDestroy(void);
    NPPluginError	PlatformSetWindow(NPPluginWindow* window);
    PRInt16         PlatformHandleEvent(NPPluginEvent* event);

    void SetMode(PRUint16 mode) { fMode = mode; }

#ifdef XP_PC
    static LRESULT CALLBACK 
    PluginWindowProc( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
#endif

protected:
    NPIPluginInstancePeer*      fPeer;
    NPPluginWindow*             fWindow;
    PRUint16                    fMode;
    PlatformInstance            fPlatform;

};

////////////////////////////////////////////////////////////////////////////////
// SimplePluginStream represents the stream used by SimplePluginInstances
// to receive data from the browser. 

class SimplePluginStream : public NPIPluginStream {
public:

    SimplePluginStream(NPIPluginStreamPeer* peer, SimplePluginInstance* inst);
    virtual ~SimplePluginStream(void);

    NS_DECL_ISUPPORTS

    ////////////////////////////////////////////////////////////////////////////
    // from NPIStream:

    // (Corresponds to NPP_WriteReady.)
    virtual PRInt32 NP_LOADDS
    WriteReady(void);

    // (Corresponds to NPP_Write and NPN_Write.)
    virtual PRInt32 NP_LOADDS
    Write(PRInt32 len, void* buffer);

    ////////////////////////////////////////////////////////////////////////////
    // from NPIPluginStream:

    // (Corresponds to NPP_StreamAsFile.)
    virtual void NP_LOADDS
    AsFile(const char* fname);

    virtual NPIPluginStreamPeer* NP_LOADDS
    GetPeer(void);

    // (Corresponds to NPP_NewStream's stype return parameter.)
    virtual NPStreamType NP_LOADDS
    GetStreamType(void);

protected:
    NPIPluginStreamPeer*        fPeer;
    SimplePluginInstance*       fInst;

};

// Interface IDs we'll need:
NS_DEFINE_IID(kPluginInstanceIID, NP_IPLUGININSTANCE_IID);
NS_DEFINE_IID(kPluginStreamIID, NP_IPLUGINSTREAM_IID);

/*******************************************************************************
 * SECTION 3 - API Plugin Implementations
 ******************************************************************************/

////////////////////////////////////////////////////////////////////////////////
// SimplePlugin Methods
////////////////////////////////////////////////////////////////////////////////

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * NP_CreatePlugin:
 * Provides global initialization for a plug-in, and returns an error value. 
 *
 * This function is called once when a plug-in is loaded, before the first instance
 * is created. You should allocate any memory or resources shared by all
 * instances of your plug-in at this time. After the last instance has been deleted,
 * NPP_Shutdown will be called, where you can release any memory or
 * resources allocated by NPP_Initialize. 
 +++++++++++++++++++++++++++++++++++++++++++++++++*/

NPPluginError
NP_CreatePlugin(NPIPluginManager* mgr, NPIPlugin* *result)
{
    SimplePlugin* inst = new SimplePlugin(mgr);
    if (inst == NULL) return NPPluginError_OutOfMemoryError;
    *result = inst;
    inst->AddRef();
    return NPPluginError_NoError;
}

SimplePlugin::SimplePlugin(NPIPluginManager* mgr) : mgr(mgr)
{
    NS_INIT_REFCNT();
}

SimplePlugin::~SimplePlugin()
{
}

nsresult SimplePlugin::QueryInterface(const nsIID& aIID, void** aInstancePtr) 
{
    if (NULL == aInstancePtr) {
        return NS_ERROR_NULL_POINTER; 
    } 
    static NS_DEFINE_IID(kPluginIID, NP_IPLUGIN_IID); 
    if (aIID.Equals(kPluginIID)) {
        *aInstancePtr = (void*) this; 
        AddRef(); 
        return NS_OK; 
    } 
    static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID); 
    if (aIID.Equals(kISupportsIID)) {
        *aInstancePtr = (void*) ((nsISupports*)this); 
        AddRef(); 
        return NS_OK; 
    } 
    return NS_NOINTERFACE; 
}

// These macros produce simple version of QueryInterface and AddRef.
// See the nsISupports.h header file for details.

NS_IMPL_ADDREF(SimplePlugin);
//NS_IMPL_RELEASE(SimplePlugin); 
nsrefcnt SimplePlugin::Release(void)
{
  if (--mRefCnt == 0) {
    delete this;
    return 0;
  }
  return mRefCnt;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * GetMIMEDescription:
 +++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifdef XP_UNIX

const char* 
SimplePlugin::GetMIMEDescription(void)
{
    return "application/x-simple-plugin:smp:Simple LiveConnect Sample Plug-in";
}

#endif /* XP_UNIX */

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * NewInstance:
 * Creates a new instance of a plug-in and returns an error value. 
 * 
 * NewInstance creates a new instance of your plug-in with MIME type specified
 * by pluginType. The parameter mode is NP_EMBED if the instance was created
 * by an EMBED tag, or NP_FULL if the instance was created by a separate file.
 * You can allocate any instance-specific private data in instance->pdata at this
 * time. The NPP pointer is valid until the instance is destroyed. 
 +++++++++++++++++++++++++++++++++++++++++++++++++*/

NPPluginError 
SimplePlugin::NewInstance(NPIPluginInstancePeer* peer, NPIPluginInstance* *result)
{
    nsMIMEType pluginType = peer->GetMIMEType();
    PRUint16 mode = peer->GetMode();
    PRInt16 argc = peer->GetArgCount();
    const char** argn = peer->GetArgNames();
    const char** argv = peer->GetArgValues();

    // mode is NPPluginType_Embedded, NPPluginType_Full, or NP_BACKGROUND
    SimplePluginInstance* inst = new SimplePluginInstance(peer, mode);
    if (inst == NULL) return NPPluginError_OutOfMemoryError;

    inst->PlatformNew(); 	/* Call Platform-specific initializations */

    *result = inst;
    inst->AddRef();

    return NPPluginError_NoError;
}

////////////////////////////////////////////////////////////////////////////////
// SimplePluginInstance Methods
////////////////////////////////////////////////////////////////////////////////

SimplePluginInstance::SimplePluginInstance(NPIPluginInstancePeer* peer, PRUint16 mode)
    : fPeer(peer), fWindow(NULL), fMode(mode)
{
    NS_INIT_REFCNT();
}

SimplePluginInstance::~SimplePluginInstance()
{
    PlatformDestroy(); // Perform platform specific cleanup
}

// These macros produce simple version of QueryInterface and AddRef.
// See the nsISupports.h header file for details.

NS_IMPL_QUERY_INTERFACE(SimplePluginInstance, kPluginInstanceIID);
NS_IMPL_ADDREF(SimplePluginInstance);
NS_IMPL_RELEASE(SimplePluginInstance);

NPIPluginInstancePeer*
SimplePluginInstance::GetPeer(void)
{
    return fPeer;
}

NPPluginError
SimplePluginInstance::Start(void)
{
    return NPPluginError_NoError;
}

NPPluginError
SimplePluginInstance::Stop(void)
{
    return NPPluginError_NoError;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * NPP_SetWindow:
 * Sets the window in which a plug-in draws, and returns an error value. 
 * 
 * NPP_SetWindow informs the plug-in instance specified by instance of the
 * the window denoted by window in which the instance draws. This NPPluginWindow
 * pointer is valid for the life of the instance, or until NPP_SetWindow is called
 * again with a different value. Subsequent calls to NPP_SetWindow for a given
 * instance typically indicate that the window has been resized. If either window
 * or window->window are NULL, the plug-in must not perform any additional
 * graphics operations on the window and should free any resources associated
 * with the window. 
 +++++++++++++++++++++++++++++++++++++++++++++++++*/

NPPluginError
SimplePluginInstance::SetWindow(NPPluginWindow* window)
{
    NPPluginError result;

    /*
     * PLUGIN DEVELOPERS:
     *	Before setting window to point to the
     *	new window, you may wish to compare the new window
     *	info to the previous window (if any) to note window
     *	size changes, etc.
     */
    result = PlatformSetWindow(window);
    fWindow = window;
    return result;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * NewStream:
 * Notifies an instance of a new data stream and returns an error value. 
 * 
 * NewStream notifies the instance denoted by instance of the creation of
 * a new stream specifed by stream. The NPStream* pointer is valid until the
 * stream is destroyed. The MIME type of the stream is provided by the
 * parameter type. 
 +++++++++++++++++++++++++++++++++++++++++++++++++*/

NPPluginError
SimplePluginInstance::NewStream(NPIPluginStreamPeer* peer, NPIPluginStream* *result)
{
    *result = new SimplePluginStream(peer, this);
    return NPPluginError_NoError;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * NPP_Print:
 +++++++++++++++++++++++++++++++++++++++++++++++++*/

void
SimplePluginInstance::Print(NPPluginPrint* printInfo)
{
    if (printInfo == NULL)
        return;

    if (printInfo->mode == NPPluginType_Full) {
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
        PRBool printOne =
            printInfo->print.fullPrint.printOne;
			
        /* Do the default*/
        printInfo->print.fullPrint.pluginPrinted = PR_FALSE;
    }
    else {	/* If not fullscreen, we must be embedded */
        /*
         * PLUGIN DEVELOPERS:
         *	If your plugin is embedded, or is full-screen
         *	but you returned false in pluginPrinted above, NPP_Print
         *	will be called with mode == NPPluginType_Embedded.  The NPPluginWindow
         *	in the printInfo gives the location and dimensions of
         *	the embedded plugin on the printed page.  On the
         *	Macintosh, platformPrint is the printer port; on
         *	Windows, platformPrint is the handle to the printing
         *	device context.
         */

        NPPluginWindow* printWindow =
            &(printInfo->print.embedPrint.window);
        void* platformPrint =
            printInfo->print.embedPrint.platformPrint;
    }
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

PRInt16
SimplePluginInstance::HandleEvent(NPPluginEvent* event)
{
    PRInt16 eventHandled = PlatformHandleEvent(event);
    return eventHandled;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * URLNotify:
 * Notifies the instance of the completion of a URL request. 
 * 
 * URLNotify is called when Netscape completes a GetURLNotify or
 * PostURLNotify request, to inform the plug-in that the request,
 * identified by url, has completed for the reason specified by reason. The most
 * common reason code is NPRES_DONE, indicating simply that the request
 * completed normally. Other possible reason codes are NPRES_USER_BREAK,
 * indicating that the request was halted due to a user action (for example,
 * clicking the "Stop" button), and NPRES_NETWORK_ERR, indicating that the
 * request could not be completed (for example, because the URL could not be
 * found). The complete list of reason codes is found in npapi.h. 
 * 
 * The parameter notifyData is the same plug-in-private value passed as an
 * argument to the corresponding GetURLNotify or PostURLNotify
 * call, and can be used by your plug-in to uniquely identify the request. 
 +++++++++++++++++++++++++++++++++++++++++++++++++*/
void
SimplePluginInstance::URLNotify(const char* url, const char* target,
                                NPPluginReason reason, void* notifyData)
{
    // Not used in the Simple plugin
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * NPP_GetValue:
 +++++++++++++++++++++++++++++++++++++++++++++++++*/

#define PLUGIN_NAME             "Simple LiveConnect Sample Plug-in"
#define PLUGIN_DESCRIPTION      "Demonstrates a simple LiveConnected plug-in."

NPPluginError
SimplePluginInstance::GetValue(NPPluginVariable variable, void *value)
{
    NPPluginError err = NPPluginError_NoError;
    if (variable == NPPluginVariable_NameString)
        *((char **)value) = PLUGIN_NAME;
    else if (variable == NPPluginVariable_DescriptionString)
        *((char **)value) = PLUGIN_DESCRIPTION;
    else
        err = NPPluginError_GenericError;

    return err;
}

NPPluginError
SimplePluginInstance::SetValue(NPPluginManagerVariable variable, void *value)
{
    return NPPluginError_NoError;
}

////////////////////////////////////////////////////////////////////////////////
// SimplePluginStream Methods
////////////////////////////////////////////////////////////////////////////////

// These macros produce simple version of QueryInterface and AddRef.
// See the nsISupports.h header file for details.

SimplePluginStream::SimplePluginStream(NPIPluginStreamPeer* peer, SimplePluginInstance* inst)
    : fPeer(peer), fInst(inst)
{
    NS_INIT_REFCNT();
}

SimplePluginStream::~SimplePluginStream()
{
}

NS_IMPL_QUERY_INTERFACE(SimplePluginStream, kPluginStreamIID);
NS_IMPL_ADDREF(SimplePluginStream);
NS_IMPL_RELEASE(SimplePluginStream);

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

PRInt32 STREAMBUFSIZE = 0X0FFFFFFF; /* If we are reading from a file in NPAsFile
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

PRInt32 
SimplePluginStream::WriteReady(void)
{
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

PRInt32 
SimplePluginStream::Write(PRInt32 len, void *buffer)
{
    return len;		/* The number of bytes accepted */
}

/*******************************************************************************/

NPIPluginStreamPeer*
SimplePluginStream::GetPeer(void)
{
    return fPeer;
}

NPStreamType
SimplePluginStream::GetStreamType(void)
{
    // XXX these should become subclasses
    return NPStreamType_Normal;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * AsFile:
 * Provides a local file name for the data from a stream. 
 * 
 * AsFile provides the instance with a full path to a local file,
 * identified by fname, for the stream specified by stream. NPP_StreamAsFile is
 * called as a result of the plug-in requesting mode NP_ASFILEONLY or
 * NP_ASFILE in a previous call to NPP_NewStream. If an error occurs while
 * retrieving the data or writing the file, fname may be NULL. 
 +++++++++++++++++++++++++++++++++++++++++++++++++*/

void 
SimplePluginStream::AsFile(const char* fname)
{
}




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
SimplePluginInstance::PlatformNew(void)
{
    This->fPlatform.window = 0;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * PlatformDestroy
 *
 * Destroy any Platform-Specific instance data.
 +++++++++++++++++++++++++++++++++++++++++++++++++*/

NPPluginError
SimplePluginInstance::PlatformDestroy(void)
{
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * PlatformSetWindow
 *
 * Perform platform-specific window operations
 +++++++++++++++++++++++++++++++++++++++++++++++++*/

NPPluginError
SimplePluginInstance::PlatformSetWindow(NPPluginWindow* window)
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
    return NPPluginError_NoError;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * PlatformHandleEvent
 *
 * Handle platform-specific events.
 +++++++++++++++++++++++++++++++++++++++++++++++++*/

int16
SimplePluginInstance::PlatformHandleEvent(NPPluginEvent* event)
{
    /* UNIX Plugins do not use HandleEvent */
    return 0;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * Redraw
 +++++++++++++++++++++++++++++++++++++++++++++++++*/

void
SimplePluginInstance::Redraw(Widget w, XtPointer closure, XEvent *event)
{
    GC gc;
    XGCValues gcv;
    const char* text = "Hello World";

    XtVaGetValues(w, XtNbackground, &gcv.background,
                  XtNforeground, &gcv.foreground, 0);
    gc = XCreateGC(fPlatform.display, fPlatform.window, 
                   GCForeground|GCBackground, &gcv);
    XDrawRectangle(fPlatform.display, fPlatform.window, gc, 
                   0, 0, fPlatform.width-1, fPlatform.height-1);
    XDrawString(fPlatform.display, fPlatform.window, gc, 
                fPlatform.width/2 - 100, fPlatform.height/2,
                text, strlen(text));
}
#endif /* XP_UNIX */

/*------------------------------------------------------------------------------
 * Windows Implementations
 *----------------------------------------------------------------------------*/

#ifdef XP_PC
const char* gInstanceLookupString = "instance->pdata";

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * PlatformNew
 *
 * Initialize any Platform-Specific instance data.
 +++++++++++++++++++++++++++++++++++++++++++++++++*/

void
SimplePluginInstance::PlatformNew(void)
{
    fPlatform.fhWnd = NULL;
    fPlatform.fDefaultWindowProc = NULL;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * PlatformDestroy
 *
 * Destroy any Platform-Specific instance data.
 +++++++++++++++++++++++++++++++++++++++++++++++++*/

NPPluginError
SimplePluginInstance::PlatformDestroy(void)
{
    if( fWindow != NULL ) { /* If we have a window, clean
                                   * it up. */
        SetWindowLong( fPlatform.fhWnd, GWL_WNDPROC, (LONG)fPlatform.fDefaultWindowProc);
        fPlatform.fDefaultWindowProc = NULL;
        fPlatform.fhWnd = NULL;
    }

    return NPPluginError_NoError;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * PlatformSetWindow
 *
 * Perform platform-specific window operations
 +++++++++++++++++++++++++++++++++++++++++++++++++*/

NPPluginError
SimplePluginInstance::PlatformSetWindow(NPPluginWindow* window)
{
    if( fWindow != NULL ) /* If we already have a window, clean
                           * it up before trying to subclass
                           * the new window. */
    {
        if( (window == NULL) || ( window->window == NULL ) ) {
            /* There is now no window to use. get rid of the old
             * one and exit. */
            SetWindowLong( fPlatform.fhWnd, GWL_WNDPROC, (LONG)fPlatform.fDefaultWindowProc);
            fPlatform.fDefaultWindowProc = NULL;
            fPlatform.fhWnd = NULL;
            return NPPluginError_NoError;
        }

        else if ( fPlatform.fhWnd == (HWND) window->window ) {
            /* The new window is the same as the old one. Exit now. */
            return NPPluginError_NoError;
        }
        else {
            /* Clean up the old window, so that we can subclass the new
             * one later. */
            SetWindowLong( fPlatform.fhWnd, GWL_WNDPROC, (LONG)fPlatform.fDefaultWindowProc);
            fPlatform.fDefaultWindowProc = NULL;
            fPlatform.fhWnd = NULL;
        }
    }
    else if( (window == NULL) || ( window->window == NULL ) ) {
        /* We can just get out of here if there is no current
         * window and there is no new window to use. */
        return NPPluginError_NoError;
    }

    /* At this point, we will subclass
     * window->window so that we can begin drawing and
     * receiving window messages. */
    fPlatform.fDefaultWindowProc =
        (WNDPROC)SetWindowLong( (HWND)window->window,
                                GWL_WNDPROC, (LONG)SimplePluginInstance::PluginWindowProc);
    fPlatform.fhWnd = (HWND) window->window;
    SetProp(fPlatform.fhWnd, gInstanceLookupString, (HANDLE)this);

    InvalidateRect( fPlatform.fhWnd, NULL, TRUE );
    UpdateWindow( fPlatform.fhWnd );
    return NPPluginError_NoError;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * PlatformHandleEvent
 *
 * Handle platform-specific events.
 +++++++++++++++++++++++++++++++++++++++++++++++++*/

PRInt16
SimplePluginInstance::PlatformHandleEvent(NPPluginEvent* event)
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

LRESULT CALLBACK 
SimplePluginInstance::PluginWindowProc( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    SimplePluginInstance* inst = (SimplePluginInstance*) GetProp(hWnd, gInstanceLookupString);

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
          inst->fPlatform.fDefaultWindowProc(hWnd, Msg, wParam, lParam);
      }
    }
    return 0;
}
#endif /* XP_PC */



/*------------------------------------------------------------------------------
 * Macintosh Implementations
 *----------------------------------------------------------------------------*/

#ifdef macintosh

PRBool	StartDraw(NPPluginWindow* window);
void 	EndDraw(NPPluginWindow* window);
void 	DoDraw(SimplePluginInstance* This);

CGrafPort 		gSavePort;
CGrafPtr		gOldPort;

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * PlatformNew
 *
 * Initialize any Platform-Specific instance data.
 +++++++++++++++++++++++++++++++++++++++++++++++++*/

void
SimplePluginInstance::PlatformNew(void)
{
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * PlatformDestroy
 *
 * Destroy any Platform-Specific instance data.
 +++++++++++++++++++++++++++++++++++++++++++++++++*/

NPPluginError
SimplePluginInstance::PlatformDestroy(void)
{
    return NPPluginError_NoError;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * PlatformSetWindow
 *
 * Perform platform-specific window operations
 +++++++++++++++++++++++++++++++++++++++++++++++++*/

NPPluginError
SimplePluginInstance::PlatformSetWindow(NPPluginWindow* window)
{
    fWindow = window;
    if( StartDraw( window ) ) {
        DoDraw(This);
        EndDraw( window );
    }
    return NPPluginError_NoError;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++
 * PlatformHandleEvent
 *
 * Handle platform-specific events.
 +++++++++++++++++++++++++++++++++++++++++++++++++*/

int16
SimplePluginInstance::PlatformHandleEvent(NPPluginEvent* event)
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
            if( StartDraw( fWindow ) ) {
                DoDraw(This);
                EndDraw( fWindow );
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

PRBool
SimplePluginInstance::StartDraw(NPPluginWindow* window)
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
SimplePluginInstance::EndDraw(NPPluginWindow* window)
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
SimplePluginInstance::DoDraw(void)
{
    Rect drawRect;
    drawRect.top = 0;
    drawRect.left = 0;
    drawRect.bottom = drawRect.top + fWindow->height;
    drawRect.right = drawRect.left + fWindow->width;
    EraseRect( &drawRect );
    MoveTo( 2, 12 );
    DrawString("\pHello, World!");
}

#endif /* macintosh */

/******************************************************************************/
