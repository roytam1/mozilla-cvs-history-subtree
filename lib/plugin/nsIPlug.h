/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

////////////////////////////////////////////////////////////////////////////////
// INTERFACE TO NETSCAPE COMMUNICATOR PLUGINS (NEW C++ API).
//
// This superscedes the old plugin API (npapi.h, npupp.h), and 
// eliminates the need for glue files: npunix.c, npwin.cpp and npmac.cpp. 
// Correspondences to the old API are shown throughout the file.
////////////////////////////////////////////////////////////////////////////////

// XXX THIS HEADER IS A BETA VERSION OF THE NEW PLUGIN INTERFACE.
// USE ONLY FOR EXPERIMENTAL PURPOSES!

#ifndef nsIPlug_h___
#define nsIPlug_h___

#ifdef __OS2__
#pragma pack(1)
#endif

#ifdef XP_MAC
	#include <Quickdraw.h>
	#include <Events.h>
#endif

#ifdef XP_UNIX
	#include <X11/Xlib.h>
	#include <X11/Xutil.h>
        #include <stdio.h> /* FILE */
#endif

#ifdef XP_PC
	#include <windef.h>
#endif

#include "nsIFactory.h"
#include "nsRepository.h"       // for NSGetFactory
#include "nsIOutputStream.h"

////////////////////////////////////////////////////////////////////////////////

/* The OS/2 version of Netscape uses RC_DATA to define the
   mime types, file extentions, etc that are required.
   Use a vertical bar to seperate types, end types with \0.
   FileVersion and ProductVersion are 32bit ints, all other
   entries are strings the MUST be terminated wwith a \0.

AN EXAMPLE:

RCDATA NS_INFO_ProductVersion { 1,0,0,1,}

RCDATA NS_INFO_MIMEType    { "video/x-video|",
                             "video/x-flick\0" }
RCDATA NS_INFO_FileExtents { "avi|",
                             "flc\0" }
RCDATA NS_INFO_FileOpenName{ "MMOS2 video player(*.avi)|",
                             "MMOS2 Flc/Fli player(*.flc)\0" }

RCDATA NS_INFO_FileVersion       { 1,0,0,1 }
RCDATA NS_INFO_CompanyName       { "Netscape Communications\0" }
RCDATA NS_INFO_FileDescription   { "NPAVI32 Extension DLL\0"
RCDATA NS_INFO_InternalName      { "NPAVI32\0" )
RCDATA NS_INFO_LegalCopyright    { "Copyright Netscape Communications \251 1996\0"
RCDATA NS_INFO_OriginalFilename  { "NVAPI32.DLL" }
RCDATA NS_INFO_ProductName       { "NPAVI32 Dynamic Link Library\0" }

*/


/* RC_DATA types for version info - required */
#define NS_INFO_ProductVersion      1
#define NS_INFO_MIMEType            2
#define NS_INFO_FileOpenName        3
#define NS_INFO_FileExtents         4

/* RC_DATA types for version info - used if found */
#define NS_INFO_FileDescription     5
#define NS_INFO_ProductName         6

/* RC_DATA types for version info - optional */
#define NS_INFO_CompanyName         7
#define NS_INFO_FileVersion         8
#define NS_INFO_InternalName        9
#define NS_INFO_LegalCopyright      10
#define NS_INFO_OriginalFilename    11

#ifndef RC_INVOKED

////////////////////////////////////////////////////////////////////////////////
// Structures and definitions

#ifdef XP_MAC
#pragma options align=mac68k
#endif

typedef const char*     nsMIMEType;

struct nsByteRange {
    PRInt32             offset; 	/* negative offset means from the end */
    PRUint32            length;
    struct nsByteRange* next;
};

struct nsRect {
    PRUint16            top;
    PRUint16            left;
    PRUint16            bottom;
    PRUint16            right;
};

////////////////////////////////////////////////////////////////////////////////
// Unix specific structures and definitions

#ifdef XP_UNIX

#include <stdio.h>

/*
 * Callback Structures.
 *
 * These are used to pass additional platform specific information.
 */
enum nsPluginCallbackType {
    nsPluginCallbackType_SetWindow = 1,
    nsPluginCallbackType_Print
};

struct nsPluginAnyCallbackStruct {
    PRInt32     type;
};

struct nsPluginSetWindowCallbackStruct {
    PRInt32     type;
    Display*    display;
    Visual*     visual;
    Colormap    colormap;
    PRUint32    depth;
};

struct nsPluginPrintCallbackStruct {
    PRInt32     type;
    FILE*       fp;
};

#endif /* XP_UNIX */

////////////////////////////////////////////////////////////////////////////////

// List of variable names for which NPP_GetValue shall be implemented
enum nsPluginVariable {
    nsPluginVariable_NameString = 1,
    nsPluginVariable_DescriptionString,
    nsPluginVariable_WindowBool,        // XXX go away
    nsPluginVariable_TransparentBool,   // XXX go away?
    nsPluginVariable_JavaClass,         // XXX go away
    nsPluginVariable_WindowSize,
    nsPluginVariable_TimerInterval
    // XXX add MIMEDescription (for unix) (but GetValue is on the instance, not the class)
};

// List of variable names for which NPN_GetValue is implemented by Mozilla
enum nsPluginManagerVariable {
    nsPluginManagerVariable_XDisplay = 1,
    nsPluginManagerVariable_XtAppContext,
    nsPluginManagerVariable_NetscapeWindow,
    nsPluginManagerVariable_JavascriptEnabledBool,      // XXX prefs accessor api
    nsPluginManagerVariable_ASDEnabledBool,             // XXX prefs accessor api
    nsPluginManagerVariable_IsOfflineBool               // XXX prefs accessor api
};

////////////////////////////////////////////////////////////////////////////////

enum nsPluginType {
    nsPluginType_Embedded = 1,
    nsPluginType_Full
};

// XXX this can go away now
enum NPStreamType {
    NPStreamType_Normal = 1,
    NPStreamType_Seek,
    NPStreamType_AsFile,
    NPStreamType_AsFileOnly
};

#define NP_STREAM_MAXREADY	(((unsigned)(~0)<<1)>>1)

/*
 * The type of a NPWindow - it specifies the type of the data structure
 * returned in the window field.
 */
enum nsPluginWindowType {
    nsPluginWindowType_Window = 1,
    nsPluginWindowType_Drawable
};

struct nsPluginWindow {
    void*       window;         /* Platform specific window handle */
                                /* OS/2: x - Position of bottom left corner  */
                                /* OS/2: y - relative to visible netscape window */
    PRUint32    x;              /* Position of top left corner relative */
    PRUint32    y;              /*	to a netscape page.					*/
    PRUint32    width;          /* Maximum window size */
    PRUint32    height;
    nsRect      clipRect;       /* Clipping rectangle in port coordinates */
                                /* Used by MAC only.			  */
#ifdef XP_UNIX
    void*       ws_info;        /* Platform-dependent additonal data */
#endif /* XP_UNIX */
    nsPluginWindowType type;    /* Is this a window or a drawable? */
};

struct nsPluginFullPrint {
    PRBool      pluginPrinted;	/* Set TRUE if plugin handled fullscreen */
                                /*	printing							 */
    PRBool      printOne;       /* TRUE if plugin should print one copy  */
                                /*	to default printer					 */
    void*       platformPrint;  /* Platform-specific printing info */
};

struct nsPluginEmbedPrint {
    nsPluginWindow    window;
    void*       platformPrint;	/* Platform-specific printing info */
};

struct nsPluginPrint {
    nsPluginType      mode;     /* NP_FULL or nsPluginType_Embedded */
    union
    {
        nsPluginFullPrint     fullPrint;	/* if mode is NP_FULL */
        nsPluginEmbedPrint    embedPrint;	/* if mode is nsPluginType_Embedded */
    } print;
};

struct nsPluginEvent {

#if defined(XP_MAC)
    EventRecord* event;
    void*       window;

#elif defined(XP_PC)
    uint16      event;
    uint32      wParam;
    uint32      lParam;

#elif defined(XP_OS2)
    uint32      event;
    uint32      wParam;
    uint32      lParam;

#elif defined(XP_UNIX)
    XEvent      event;

#endif
};

#ifdef XP_MAC
typedef RgnHandle nsRegion;
#elif defined(XP_PC)
typedef HRGN nsRegion;
#elif defined(XP_UNIX)
typedef Region nsRegion;
#else
typedef void *nsRegion;
#endif

////////////////////////////////////////////////////////////////////////////////
// Mac-specific structures and definitions.

#ifdef XP_MAC

struct NPPort {
    CGrafPtr    port;   /* Grafport */
    PRInt32     portx;  /* position inside the topmost window */
    PRInt32     porty;
};

/*
 *  Non-standard event types that can be passed to HandleEvent
 */
#define getFocusEvent           (osEvt + 16)
#define loseFocusEvent          (osEvt + 17)
#define adjustCursorEvent       (osEvt + 18)
#define menuCommandEvent		(osEvt + 19)

#endif /* XP_MAC */

////////////////////////////////////////////////////////////////////////////////
// Error and Reason Code definitions

enum nsPluginError {
    nsPluginError_Base = 0,
    nsPluginError_NoError = 0,
    nsPluginError_GenericError,
    nsPluginError_InvalidInstanceError,
    nsPluginError_InvalidFunctableError,
    nsPluginError_ModuleLoadFailedError,
    nsPluginError_OutOfMemoryError,
    nsPluginError_InvalidPluginError,
    nsPluginError_InvalidPluginDirError,
    nsPluginError_IncompatibleVersionError,
    nsPluginError_InvalidParam,
    nsPluginError_InvalidUrl,
    nsPluginError_FileNotFound,
    nsPluginError_NoData,
    nsPluginError_StreamNotSeekable
};

#define NPCallFailed( code ) ((code) != nsPluginError_NoError)

enum nsPluginReason {
    nsPluginReason_Base = 0,
    nsPluginReason_Done = 0,
    nsPluginReason_NetworkErr,
    nsPluginReason_UserBreak,
    nsPluginReason_NoReason
};

////////////////////////////////////////////////////////////////////////////////
// Classes
////////////////////////////////////////////////////////////////////////////////

// Classes that must be implemented by the plugin DLL:
struct nsIPlugin;                       // plugin class (MIME-type handler)
class nsILiveConnectPlugin;             // subclass of nsIPlugin (see nsILCPlg.h)
class nsIPluginInstance;                // plugin instance
class nsIPluginStream;                  // stream to receive data from the browser

// Classes that are implemented by the browser:
class nsIPluginManager;                 // minimum browser requirements
class nsIFileUtilities;                 // file utilities (accessible from nsIPluginManager)
class nsIPluginInstancePeer;            // parts of nsIPluginInstance implemented by the browser
class nsIWindowlessPluginInstancePeer;  // subclass of nsIPluginInstancePeer for windowless plugins
class nsIPluginTagInfo;                 // describes html tag (accessible from nsIPluginInstancePeer)
class nsILiveConnectPluginInstancePeer; // subclass of nsIPluginInstancePeer (see nsILCPlg.h)
class nsIPluginStreamPeer;              // parts of nsIPluginStream implemented by the browser
class nsISeekablePluginStreamPeer;      // seekable subclass of nsIPluginStreamPeer

//       Plugin DLL Side                Browser Side
//
//         
//       +-----------+                 +-----------------------+
//       | Plugin /  |                 | Plugin Manager        |
//       | LC Plugin |                 |                       |
//       +-----------+                 +-----------------------+
//            ^                               ^
//            |                               |
//            |                        +-----------------------+
//            |                        | Plugin Manager Stream |
//            |                        +-----------------------+
//            |
//            |
//       +-----------------+   peer    +-----------------------+
//       | Plugin Instance |---------->| Plugin Instance Peer  |
//       |                 |           | / LC Plugin Inst Peer |
//       +-----------------+           +-----------------------+
//
//       +-----------------+   peer    +-----------------------+
//       | Plugin Stream   |---------->| Plugin Stream Peer /  |
//       |                 |           | Seekable P Stream Peer|
//       +-----------------+           +-----------------------+

////////////////////////////////////////////////////////////////////////////////
// This is the main entry point to the plugin's DLL. The plugin manager finds
// this symbol and calls it to create the plugin class. Once the plugin object
// is returned to the plugin manager, instances on the page are created by 
// calling nsIPlugin::CreateInstance.

// (Declared in nsRepository.h)
//extern "C" NS_EXPORT nsresult NSGetFactory(const nsCID &aClass,
//                                           nsIFactory **aFactory);

////////////////////////////////////////////////////////////////////////////////
// THINGS THAT MUST BE IMPLEMENTED BY THE PLUGIN...
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Plugin Interface
// This is the minimum interface plugin developers need to support in order to
// implement a plugin. The plugin manager may QueryInterface for more specific 
// plugin types, e.g. nsILiveConnectPlugin.

struct nsIPlugin : public nsIFactory {
public:

    // This call initializes the plugin and will be called before any new
    // instances are created. It is passed browserInterfaces on which QueryInterface
    // may be used to obtain an nsIPluginManager, and other interfaces.
    NS_IMETHOD_(nsPluginError)
    Initialize(nsISupports* browserInterfaces) = 0;

    // (Corresponds to NPP_Shutdown.)
    // Called when the browser is done with the plugin factory, or when
    // the plugin is disabled by the user.
    NS_IMETHOD_(nsPluginError)
    Shutdown(void) = 0;

    // (Corresponds to NPP_GetMIMEDescription.)
    NS_IMETHOD_(const char*)
    GetMIMEDescription(void) = 0;

    // The old NPP_New call has been factored into two plugin instance methods:
    //
    // CreateInstance -- called once, after the plugin instance is created. This 
    // method is used to initialize the new plugin instance (although the actual
    // plugin instance object will be created by the plugin manager).
    //
    // nsIPluginInstance::Start -- called when the plugin instance is to be
    // started. This happens in two circumstances: (1) after the plugin instance
    // is first initialized, and (2) after a plugin instance is returned to
    // (e.g. by going back in the window history) after previously being stopped
    // by the Stop method. 

};

#define NS_IPLUGIN_IID                               \
{ /* df773070-0199-11d2-815b-006008119d7a */         \
    0xdf773070,                                      \
    0x0199,                                          \
    0x11d2,                                          \
    {0x81, 0x5b, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

////////////////////////////////////////////////////////////////////////////////
// Plugin Instance Interface

// (Corresponds to NPP object.)
class nsIPluginInstance : public nsISupports {
public:

    NS_IMETHOD_(nsPluginError)
    Initialize(nsIPluginInstancePeer* peer) = 0;

    // Required backpointer to the peer.
    NS_IMETHOD_(nsIPluginInstancePeer*)
    GetPeer(void) = 0;

    // See comment for nsIPlugin::CreateInstance, above.
    NS_IMETHOD_(nsPluginError)
    Start(void) = 0;

    // The old NPP_Destroy call has been factored into two plugin instance 
    // methods:
    //
    // Stop -- called when the plugin instance is to be stopped (e.g. by 
    // displaying another plugin manager window, causing the page containing 
    // the plugin to become removed from the display).
    //
    // Release -- called once, before the plugin instance peer is to be 
    // destroyed. This method is used to destroy the plugin instance.

    NS_IMETHOD_(nsPluginError)
    Stop(void) = 0;

    NS_IMETHOD_(nsPluginError)
    Destroy(void) = 0;

    // (Corresponds to NPP_SetWindow.)
    NS_IMETHOD_(nsPluginError)
    SetWindow(nsPluginWindow* window) = 0;

    // (Corresponds to NPP_NewStream.)
    NS_IMETHOD_(nsPluginError)
    NewStream(nsIPluginStreamPeer* peer, nsIPluginStream* *result) = 0;

    // (Corresponds to NPP_Print.)
    NS_IMETHOD_(void)
    Print(nsPluginPrint* platformPrint) = 0;

    // (Corresponds to NPP_HandleEvent.)
    // Note that for Unix and Mac the nsPluginEvent structure is different
    // from the old NPEvent structure -- it's no longer the native event
    // record, but is instead a struct. This was done for future extensibility,
    // and so that the Mac could receive the window argument too. For Windows
    // and OS2, it's always been a struct, so there's no change for them.
    NS_IMETHOD_(PRInt16)
    HandleEvent(nsPluginEvent* event) = 0;

    // (Corresponds to NPP_URLNotify.)
    NS_IMETHOD_(void)
    URLNotify(const char* url, const char* target,
              nsPluginReason reason, void* notifyData) = 0;

    // (Corresponds to NPP_GetValue.)
    NS_IMETHOD_(nsPluginError)
    GetValue(nsPluginVariable variable, void *value) = 0;

    // (Corresponds to NPP_SetValue.)
    NS_IMETHOD_(nsPluginError)
    SetValue(nsPluginManagerVariable variable, void *value) = 0;
};

#define NS_IPLUGININSTANCE_IID                       \
{ /* ebe00f40-0199-11d2-815b-006008119d7a */         \
    0xebe00f40,                                      \
    0x0199,                                          \
    0x11d2,                                          \
    {0x81, 0x5b, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

////////////////////////////////////////////////////////////////////////////////
// Plugin Stream Interface

class nsIPluginStream : public nsIOutputStream {
public:

    // (Corresponds to NPP_NewStream's stype return parameter.)
    NS_IMETHOD_(NPStreamType)
    GetStreamType(void) = 0;

    // (Corresponds to NPP_StreamAsFile.)
    NS_IMETHOD_(void)
    AsFile(const char* fname) = 0;

};

#define NS_IPLUGINSTREAM_IID                         \
{ /* f287dd50-0199-11d2-815b-006008119d7a */         \
    0xf287dd50,                                      \
    0x0199,                                          \
    0x11d2,                                          \
    {0x81, 0x5b, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

////////////////////////////////////////////////////////////////////////////////
// THINGS IMPLEMENTED BY THE BROWSER...
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Plugin Manager Interface
// This interface defines the minimum set of functionality that a plugin
// manager will support if it implements plugins. Plugin implementations can
// QueryInterface to determine if a plugin manager implements more specific 
// APIs for the plugin to use.

class nsIPluginManager : public nsISupports {
public:

    // (Corresponds to NPN_ReloadPlugins.)
    NS_IMETHOD_(void)
    ReloadPlugins(PRBool reloadPages) = 0;

    // (Corresponds to NPN_MemAlloc.)
    NS_IMETHOD_(void*)
    MemAlloc(PRUint32 size) = 0;

    // (Corresponds to NPN_MemFree.)
    NS_IMETHOD_(void)
    MemFree(void* ptr) = 0;

    // (Corresponds to NPN_MemFlush.)
    NS_IMETHOD_(PRUint32)
    MemFlush(PRUint32 size) = 0;

    // (Corresponds to NPN_UserAgent.)
    NS_IMETHOD_(const char*)
    UserAgent(void) = 0;

    // (Corresponds to NPN_GetURL and NPN_GetURLNotify.)
    //   notifyData: When present, URLNotify is called passing the notifyData back
    //          to the client. When NULL, this call behaves like NPN_GetURL.
    // New arguments:
    //   peer:  A plugin instance peer. The peer's window will be used to display
    //          progress information. If NULL, the load happens in the background.
    //   altHost: An IP-address string that will be used instead of the host
    //          specified in the URL. This is used to prevent DNS-spoofing attacks.
    //          Can be defaulted to NULL meaning use the host in the URL.
    //   referrer: 
    //   forceJSEnabled: Forces JavaScript to be enabled for 'javascript:' URLs,
    //          even if the user currently has JavaScript disabled. 
    NS_IMETHOD_(nsPluginError)
    GetURL(nsISupports* peer, const char* url, const char* target, void* notifyData = NULL,
           const char* altHost = NULL, const char* referrer = NULL,
           PRBool forceJSEnabled = PR_FALSE) = 0;

    // (Corresponds to NPN_PostURL and NPN_PostURLNotify.)
    //   notifyData: When present, URLNotify is called passing the notifyData back
    //          to the client. When NULL, this call behaves like NPN_GetURL.
    // New arguments:
    //   peer:  A plugin instance peer. The peer's window will be used to display
    //          progress information. If NULL, the load happens in the background.
    //   altHost: An IP-address string that will be used instead of the host
    //          specified in the URL. This is used to prevent DNS-spoofing attacks.
    //          Can be defaulted to NULL meaning use the host in the URL.
    //   referrer: 
    //   forceJSEnabled: Forces JavaScript to be enabled for 'javascript:' URLs,
    //          even if the user currently has JavaScript disabled. 
    //   postHeaders: A string containing post headers.
    //   postHeadersLength: The length of the post headers string.
    NS_IMETHOD_(nsPluginError)
    PostURL(nsISupports* peer, const char* url, const char* target, PRUint32 bufLen, 
            const char* buf, PRBool file, void* notifyData = NULL,
            const char* altHost = NULL, const char* referrer = NULL,
            PRBool forceJSEnabled = PR_FALSE,
            PRUint32 postHeadersLength = 0, const char* postHeaders = NULL) = 0;

};

#define NS_IPLUGINMANAGER_IID                        \
{ /* f10b9600-a1bc-11d1-85b1-00805f0e4dfe */         \
    0xf10b9600,                                      \
    0xa1bc,                                          \
    0x11d1,                                          \
    {0x85, 0xb1, 0x00, 0x80, 0x5f, 0x0e, 0x4d, 0xfe} \
}

////////////////////////////////////////////////////////////////////////////////
// Plugin Manager 2 Interface
// These extensions to nsIPluginManager are only available in Communicator 5.0.

class nsIPluginManager2 : public nsIPluginManager {
public:

    NS_IMETHOD_(void)
    BeginWaitCursor(void) = 0;

    NS_IMETHOD_(void)
    EndWaitCursor(void) = 0;

    NS_IMETHOD_(PRBool)
    SupportsURLProtocol(const char* protocol) = 0;

};

#define NS_IPLUGINMANAGER2_IID                       \
{ /* 29c4ae70-019a-11d2-815b-006008119d7a */         \
    0x29c4ae70,                                      \
    0x019a,                                          \
    0x11d2,                                          \
    {0x81, 0x5b, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

////////////////////////////////////////////////////////////////////////////////
// File Utilities Interface
// This interface reflects operations only available in Communicator 5.0.

class nsIFileUtilities : public nsISupports {
public:

    // QueryInterface on nsIPluginManager to get this.
    
    NS_IMETHOD_(const char*)
    GetProgramPath(void) = 0;

    NS_IMETHOD_(const char*)
    GetTempDirPath(void) = 0;

    enum FileNameType { SIGNED_APPLET_DBNAME, TEMP_FILENAME };

    NS_IMETHOD_(nsresult)
    GetFileName(const char* fn, FileNameType type,
                char* resultBuf, PRUint32 bufLen) = 0;

    NS_IMETHOD_(nsresult)
    NewTempFileName(const char* prefix, char* resultBuf, PRUint32 bufLen) = 0;

};

#define NS_IFILEUTILITIES_IID                        \
{ /* 89a31ce0-019a-11d2-815b-006008119d7a */         \
    0x89a31ce0,                                      \
    0x019a,                                          \
    0x11d2,                                          \
    {0x81, 0x5b, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

////////////////////////////////////////////////////////////////////////////////
// Plugin Instance Peer Interface

enum nsPluginTagType {
    nsPluginTagType_Unknown,
    nsPluginTagType_Embed,
    nsPluginTagType_Object,
    nsPluginTagType_Applet
};

class nsIPluginInstancePeer : public nsISupports {
public:

    // (Corresponds to NPP_New's MIMEType argument.)
    NS_IMETHOD_(nsMIMEType)
    GetMIMEType(void) = 0;

    // (Corresponds to NPP_New's mode argument.)
    NS_IMETHOD_(nsPluginType)
    GetMode(void) = 0;

    // (Corresponds to NPN_NewStream.)
    NS_IMETHOD_(nsPluginError)
    NewStream(nsMIMEType type, const char* target, nsIOutputStream* *result) = 0;

    // (Corresponds to NPN_Status.)
    NS_IMETHOD_(void)
    ShowStatus(const char* message) = 0;

    // (Corresponds to NPN_GetValue.)
    NS_IMETHOD_(nsPluginError)
    GetValue(nsPluginManagerVariable variable, void *value) = 0;

    // (Corresponds to NPN_SetValue.)
    NS_IMETHOD_(nsPluginError)
    SetValue(nsPluginVariable variable, void *value) = 0;

};

#define NS_IPLUGININSTANCEPEER_IID                   \
{ /* 4b7cea20-019b-11d2-815b-006008119d7a */         \
    0x4b7cea20,                                      \
    0x019b,                                          \
    0x11d2,                                          \
    {0x81, 0x5b, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

////////////////////////////////////////////////////////////////////////////////
// Plugin Instance Peer 2 Interface
// These extensions to nsIPluginManager are only available in Communicator 5.0.

class nsIPluginInstancePeer2 : public nsIPluginInstancePeer {
public:
    
    ////////////////////////////////////////////////////////////////////////////
    // New top-level window handling calls for Mac:
    
    NS_IMETHOD_(void)
    RegisterWindow(void* window) = 0;
    
    NS_IMETHOD_(void)
    UnregisterWindow(void* window) = 0;

	// Menu ID allocation calls for Mac:
    NS_IMETHOD_(PRInt16)
	AllocateMenuID(PRBool isSubmenu) = 0;

	// On the mac (and most likely win16), network activity can
    // only occur on the main thread. Therefore, we provide a hook
    // here for the case that the main thread needs to tickle itself.
    // In this case, we make sure that we give up the monitor so that
    // the tickle code can notify it without freezing.
    NS_IMETHOD_(PRBool)
    Tickle(void) = 0;

};

#define NS_IPLUGININSTANCEPEER2_IID                  \
{ /* 51b52b80-019b-11d2-815b-006008119d7a */         \
    0x51b52b80,                                      \
    0x019b,                                          \
    0x11d2,                                          \
    {0x81, 0x5b, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

////////////////////////////////////////////////////////////////////////////////
// Windowless Plugin Instance Peer Interface

class nsIWindowlessPluginInstancePeer : public nsISupports {
public:

    // (Corresponds to NPN_InvalidateRect.)
    NS_IMETHOD_(void)
    InvalidateRect(nsRect *invalidRect) = 0;

    // (Corresponds to NPN_InvalidateRegion.)
    NS_IMETHOD_(void)
    InvalidateRegion(nsRegion invalidRegion) = 0;

    // (Corresponds to NPN_ForceRedraw.)
    NS_IMETHOD_(void)
    ForceRedraw(void) = 0;

};

#define NS_IWINDOWLESSPLUGININSTANCEPEER_IID         \
{ /* 57b4e2f0-019b-11d2-815b-006008119d7a */         \
    0x57b4e2f0,                                      \
    0x019b,                                          \
    0x11d2,                                          \
    {0x81, 0x5b, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

////////////////////////////////////////////////////////////////////////////////
// Plugin Tag Info Interface
// This interface provides information about the HTML tag on the page.
// Some day this might get superseded by a DOM API.

class nsIPluginTagInfo : public nsISupports {
public:

    // QueryInterface on nsIPluginInstancePeer to get this.

    // (Corresponds to NPP_New's argc, argn, and argv arguments.)
    // Get a ptr to the paired list of attribute names and values,
    // returns the length of the array.
    //
    // Each name or value is a null-terminated string.
    NS_IMETHOD_(nsPluginError)
    GetAttributes(PRUint16& n, const char*const*& names, const char*const*& values) = 0;

    // Get the value for the named attribute.  Returns NULL
    // if the attribute was not set.
    NS_IMETHOD_(const char*)
    GetAttribute(const char* name) = 0;

};

#define NS_IPLUGINTAGINFO_IID                        \
{ /* 5f1ec1d0-019b-11d2-815b-006008119d7a */         \
    0x5f1ec1d0,                                      \
    0x019b,                                          \
    0x11d2,                                          \
    {0x81, 0x5b, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

////////////////////////////////////////////////////////////////////////////////
// Plugin Tag Info Interface
// These extensions to nsIPluginTagInfo are only available in Communicator 5.0.

class nsIPluginTagInfo2 : public nsIPluginTagInfo {
public:

    // QueryInterface on nsIPluginInstancePeer to get this.

    // Get the type of the HTML tag that was used ot instantiate this
    // plugin.  Currently supported tags are EMBED, OBJECT and APPLET.
    NS_IMETHOD_(nsPluginTagType) 
    GetTagType(void) = 0;

    // Get the complete text of the HTML tag that was
    // used to instantiate this plugin
    NS_IMETHOD_(const char *)
    GetTagText(void) = 0;

    // Get a ptr to the paired list of parameter names and values,
    // returns the length of the array.
    //
    // Each name or value is a null-terminated string.
    NS_IMETHOD_(nsPluginError)
    GetParameters(PRUint16& n, const char*const*& names, const char*const*& values) = 0;

    // Get the value for the named parameter.  Returns null
    // if the parameter was not set.
    NS_IMETHOD_(const char*)
    GetParameter(const char* name) = 0;
    
    NS_IMETHOD_(const char*)
    GetDocumentBase(void) = 0;
    
    // Return an encoding whose name is specified in:
    // http://java.sun.com/products/jdk/1.1/docs/guide/intl/intl.doc.html#25303
    NS_IMETHOD_(const char*)
    GetDocumentEncoding(void) = 0;
    
    NS_IMETHOD_(const char*)
    GetAlignment(void) = 0;
    
    NS_IMETHOD_(PRUint32)
    GetWidth(void) = 0;
    
    NS_IMETHOD_(PRUint32)
    GetHeight(void) = 0;
    
    NS_IMETHOD_(PRUint32)
    GetBorderVertSpace(void) = 0;
    
    NS_IMETHOD_(PRUint32)
    GetBorderHorizSpace(void) = 0;

    // Returns a unique id for the current document on which the
    // plugin is displayed.
    NS_IMETHOD_(PRUint32)
    GetUniqueID(void) = 0;

};

#define NS_IPLUGINTAGINFO2_IID                       \
{ /* 6a49c9a0-019b-11d2-815b-006008119d7a */         \
    0x6a49c9a0,                                      \
    0x019b,                                          \
    0x11d2,                                          \
    {0x81, 0x5b, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

////////////////////////////////////////////////////////////////////////////////
// Plugin Stream Peer Interface
// A plugin stream peer is passed to a plugin instance's NewStream call in 
// order to indicate that a new stream is to be created and be read by the
// plugin instance.

class nsIPluginStreamPeer : public nsISupports {
public:

    // (Corresponds to NPStream's url field.)
    NS_IMETHOD_(const char*)
    GetURL(void) = 0;

    // (Corresponds to NPStream's end field.)
    NS_IMETHOD_(PRUint32)
    GetEnd(void) = 0;

    // (Corresponds to NPStream's lastmodified field.)
    NS_IMETHOD_(PRUint32)
    GetLastModified(void) = 0;

    // (Corresponds to NPStream's notifyData field.)
    NS_IMETHOD_(void*)
    GetNotifyData(void) = 0;

    // (Corresponds to NPP_DestroyStream's reason argument.)
    NS_IMETHOD_(nsPluginReason)
    GetReason(void) = 0;

    // (Corresponds to NPP_NewStream's MIMEType argument.)
    NS_IMETHOD_(nsMIMEType)
    GetMIMEType(void) = 0;

};

#define NS_IPLUGINSTREAMPEER_IID                     \
{ /* 717b1e90-019b-11d2-815b-006008119d7a */         \
    0x717b1e90,                                      \
    0x019b,                                          \
    0x11d2,                                          \
    {0x81, 0x5b, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

////////////////////////////////////////////////////////////////////////////////
// Plugin Stream Peer Interface
// These extensions to nsIPluginStreamPeer are only available in Communicator 5.0.

class nsIPluginStreamPeer2 : public nsIPluginStreamPeer {
public:

    NS_IMETHOD_(PRUint32)
    GetContentLength(void) = 0;

    NS_IMETHOD_(PRUint32)
    GetHeaderFieldCount(void) = 0;

    NS_IMETHOD_(const char*)
    GetHeaderFieldKey(PRUint32 index) = 0;

    NS_IMETHOD_(const char*)
    GetHeaderField(PRUint32 index) = 0;

};

#define NS_IPLUGINSTREAMPEER2_IID                    \
{ /* 77083af0-019b-11d2-815b-006008119d7a */         \
    0x77083af0,                                      \
    0x019b,                                          \
    0x11d2,                                          \
    {0x81, 0x5b, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

////////////////////////////////////////////////////////////////////////////////
// Seekable Plugin Stream Peer Interface
// The browser implements this subclass of plugin stream peer if a stream
// is seekable. Plugins can query interface for this type, and call the 
// RequestRead method to seek to a particular position in the stream.

class nsISeekablePluginStreamPeer : public nsISupports {
public:

    // QueryInterface for this class corresponds to NPP_NewStream's 
    // seekable argument.

    // (Corresponds to NPN_RequestRead.)
    NS_IMETHOD_(nsPluginError)
    RequestRead(nsByteRange* rangeList) = 0;

};

#define NS_ISEEKABLEPLUGINSTREAMPEER_IID             \
{ /* 7e028d20-019b-11d2-815b-006008119d7a */         \
    0x7e028d20,                                      \
    0x019b,                                          \
    0x11d2,                                          \
    {0x81, 0x5b, 0x00, 0x60, 0x08, 0x11, 0x9d, 0x7a} \
}

////////////////////////////////////////////////////////////////////////////////

#ifdef XP_MAC
#pragma options align=reset
#endif

#endif /* RC_INVOKED */
#ifdef __OS2__
#pragma pack()
#endif

#endif /* nsIPlug_h___ */
