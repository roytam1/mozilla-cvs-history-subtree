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

#ifndef lm_h___
#define lm_h___
/*
 * JS in the Navigator library-private interface.
 */
#include "xp.h"         /* for uint and PA_Block */
#include "prlong.h"	/* for int64 time type used below */
#include "libevent.h"	/* until its a stand-alone */
#include "libmocha.h"

/*
 * Shared string constants for common property names.
 */
extern char lm_argc_err_str[];          /* "incorrect number of arguments" */

extern char lm_unknown_origin_str[];    /* "[unknown origin]" */

extern char lm_onLoad_str[];            /* "onLoad" */
extern char lm_onUnload_str[];          /* "onUnload" */
extern char lm_onAbort_str[];           /* "onAbort" */
extern char lm_onError_str[];           /* "onError" */
extern char lm_onScroll_str[];          /* "onScroll" */
extern char lm_onFocus_str[];           /* "onFocus" */
extern char lm_onBlur_str[];            /* "onBlur" */
extern char lm_onSelect_str[];          /* "onSelect" */
extern char lm_onChange_str[];          /* "onChange" */
extern char lm_onReset_str[];           /* "onReset" */
extern char lm_onSubmit_str[];          /* "onSubmit" */
extern char lm_onClick_str[];           /* "onClick" */
extern char lm_onMouseDown_str[];       /* "onMouseDown" */
extern char lm_onMouseOver_str[];       /* "onMouseOver" */
extern char lm_onMouseOut_str[];        /* "onMouseOut" */
extern char lm_onMouseUp_str[];         /* "onMouseUp" */
extern char lm_onLocate_str[];          /* "onLocate" */
extern char lm_onHelp_str[];            /* "onHelp" [EA] */

extern char lm_focus_str[];             /* "focus" */
extern char lm_blur_str[];              /* "blur" */
extern char lm_select_str[];            /* "select" */
extern char lm_click_str[];             /* "click" */
extern char lm_scroll_str[];            /* "scroll" */
extern char lm_enable_str[];            /* "enable" */
extern char lm_disable_str[];           /* "disable" */

extern char lm_toString_str[];          /* "toString" */
extern char lm_length_str[];            /* "length" */
extern char lm_document_str[];          /* "document" */
extern char lm_forms_str[];             /* "forms" */
extern char lm_links_str[];             /* "links" */
extern char lm_anchors_str[];           /* "anchors" */
extern char lm_applets_str[];           /* "applets" */
extern char lm_embeds_str[];            /* "embeds" */
extern char lm_plugins_str[];           /* "plugins" */
extern char lm_images_str[];            /* "images" */
extern char lm_layers_str[];            /* "layers" */
extern char lm_location_str[];          /* "location" */
extern char lm_navigator_str[];         /* "navigator" */
extern char lm_netcaster_str[];         /* "netcaster" */
extern char lm_components_str[];        /* "components" */

extern char lm_parentLayer_str[];       /* "parentLayer" */
extern char lm_opener_str[];            /* "opener" */
extern char lm_closed_str[];            /* "closed" */
extern char lm_assign_str[];            /* "assign" */
extern char lm_reload_str[];            /* "reload" */
extern char lm_replace_str[];           /* "replace" */
extern char lm_event_str[];             /* "event" */
extern char lm_methodPrefix_str[];      /* "#method" */
extern char lm_methodArgc_str[];      /* "#method" */
extern char lm_methodArgv_str[];      /* "#method" */
extern char lm_getPrefix_str[];         /* "#get_" */
extern char lm_setPrefix_str[];         /* "#set_" */
extern char lm_typePrefix_str[];        /* "#type_" */
extern const char *lm_event_argv[];     /* {lm_event_str} */

extern PRThread		    *lm_InterpretThread;
extern PRThread		    *mozilla_thread;
extern PRThread		    *lm_js_lock_previous_owner;

extern JSContext *lm_writing_context;

/*
 * Timeout structure threaded on MochaDecoder.timeouts for cleanup.
 */
struct JSTimeout {
    int32               ref_count;      /* reference count to shared usage */
    char                *expr;          /* the JS expression to evaluate */
    JSObject            *funobj;        /* or function to call, if !expr */
    jsval               *argv;          /* function actual arguments */
    void                *toid;          /* Identifier, used internally only */
    uint32              public_id;      /* Returned as value of setTimeout() */
    uint16              argc;           /* and argument count */
    uint16              spare;          /* alignment padding */
    int32		doc_id;       	/* document this is for */
    int32               interval;       /* Non-zero if repetitive timeout */
    int64               when;           /* nominal time to run this timeout */
    JSVersion		version;	/* Version of JavaScript to execute */
    JSPrincipals        *principals;    /* principals with which to execute */
    char                *filename;      /* filename of setTimeout call */
    uint32              lineno;         /* line number of setTimeout call */
    JSTimeout           *next;          /* next timeout in list */
};

extern void             lm_ClearWindowTimeouts(MochaDecoder *decoder);

struct JSNestingUrl {
    JSNestingUrl 	*next;
    char                *str;
};

/*
 * Event queue stack madness to handle doc.write("<script>doc.write...").
 */
typedef struct QueueStackElement {
    PREventQueue		* queue;
    MWContext		        * context;
    int32			  doc_id;
    struct QueueStackElement	* up;
    struct QueueStackElement	* down;
    PRPackedBool		  done;
    PRPackedBool		  discarding;
    PRPackedBool		  inherit_parent;
    void                        * retval;
} QueueStackElement;

extern void
et_SubEventLoop(QueueStackElement * qse);

/*
 * Stack size per window context, plus one for the navigator.
 */
#define LM_STACK_SIZE   8192

extern JSRuntime        *lm_runtime;
extern JSClass		lm_window_class;
extern JSClass		lm_layer_class;
extern JSClass		lm_document_class;
extern JSClass		lm_event_class;

extern JSBool           lm_SaveParamString(JSContext *cx, PA_Block *bp,
                                           const char *str);
extern MochaDecoder     *lm_NewWindow(MWContext *context);
extern void             lm_DestroyWindow(MochaDecoder *decoder);

/*
 * Hold and drop the reference count for tree back-edges that go from object
 * private data to the containing decoder.  These refs do not keep the object
 * tree under decoder alive from the GC, but they do keep decoder from being
 * destroyed and some out of order finalizer tripping over its freed memory.
 */
#ifdef DEBUG
extern MochaDecoder     *lm_HoldBackCount(MochaDecoder *decoder);
extern void             lm_DropBackCount(MochaDecoder *decoder);

#define HOLD_BACK_COUNT(decoder) lm_HoldBackCount(decoder)
#define DROP_BACK_COUNT(decoder) lm_DropBackCount(decoder)
#else
#define HOLD_BACK_COUNT(decoder)                                             \
    (((decoder) ? (decoder)->back_count++ : 0), (decoder))
#define DROP_BACK_COUNT(decoder)                                             \
    (((decoder) && --(decoder)->back_count <= 0 && !(decoder)->forw_count)   \
     ? lm_DestroyWindow(decoder)					     \
     : (void)0)
#endif

extern JSBool           lm_InitWindowContent(MochaDecoder *decoder);
extern JSBool           lm_DefineWindowProps(JSContext *cx,
                                             MochaDecoder *decoder);
extern JSBool           lm_ResolveWindowProps(JSContext *cx,
                                              MochaDecoder *decoder,
                                              JSObject *obj,
                                              jsval id);
extern void             lm_FreeWindowContent(MochaDecoder *decoder,
					     JSBool fromDiscard);
extern JSBool           lm_SetInputStream(JSContext *cx,
					  MochaDecoder *decoder,
					  NET_StreamClass *stream,
					  URL_Struct *url_struct,
                                          JSBool free_stream_on_close);
extern JSObject         *lm_DefineDocument(MochaDecoder *decoder,
                                           int32 layer_id);
extern JSObject         *lm_DefineHistory(MochaDecoder *decoder);
extern JSObject         *lm_DefineLocation(MochaDecoder *decoder);
extern JSObject         *lm_DefineNavigator(MochaDecoder *decoder);
extern JSObject         *lm_DefineComponents(MochaDecoder *decoder);
extern JSObject         *lm_DefineCrypto(MochaDecoder *decoder);
extern JSObject         *lm_DefineScreen(MochaDecoder *decoder,
                                         JSObject *parent);
extern JSObject         *lm_DefineHardware(MochaDecoder *decoder,
                                         JSObject *parent);
extern JSBool           lm_DefinePluginClasses(MochaDecoder *decoder);
extern JSBool           lm_DefineBarClasses(MochaDecoder *decoder);
extern JSBool           lm_ResolveBar(JSContext *cx, MochaDecoder *decoder,
				      const char *name);
extern JSBool           lm_DefineTriggers(void);
extern JSObject         *lm_NewPluginList(JSContext *cx, JSObject *parent_obj);
extern JSObject         *lm_NewMIMETypeList(JSContext *cx);
extern JSObject         *lm_GetDocumentFromLayerId(MochaDecoder *decoder,
                                                   int32 layer_id);
extern JSObject         *lm_DefinePkcs11(MochaDecoder *decoder);
/*
 * Get (create if needed) document's form, link, and named anchor arrays.
 */
extern JSObject         *lm_GetFormArray(MochaDecoder *decoder,
                                         JSObject *document);
extern JSObject         *lm_GetLinkArray(MochaDecoder *decoder,
                                         JSObject *document);
extern JSObject         *lm_GetNameArray(MochaDecoder *decoder,
                                         JSObject *document);
extern JSObject         *lm_GetAppletArray(MochaDecoder *decoder,
                                           JSObject *document);
extern JSObject         *lm_GetEmbedArray(MochaDecoder *decoder,
                                          JSObject *document);
extern JSObject         *lm_GetImageArray(MochaDecoder *decoder,
                                          JSObject *document);
extern JSObject         *lm_GetDocumentLayerArray(MochaDecoder *decoder,
                                                  JSObject *document);


/*
 * dummy object for applets and embeds that can't be reflected
 */
extern JSObject *lm_DummyObject;
extern void lm_InitDummyObject(JSContext *cx);

/* bit vector for handlers */
typedef enum  {
	HANDLER_ONCLICK = 1 << 0,
	HANDLER_ONFOCUS = 1 << 1,
	HANDLER_ONBLUR = 1 << 2,
	HANDLER_ONCHANGE = 1 << 3,
	HANDLER_ONSELECT = 1 << 4,
	HANDLER_ONSCROLL = 1 << 5,
	HANDLER_ONMOUSEDOWN = 1 << 6,
	HANDLER_ONMOUSEUP = 1 << 7,
	HANDLER_ONKEYDOWN = 1 << 8,
	HANDLER_ONKEYUP = 1 << 9,
	HANDLER_ONKEYPRESS = 1 << 10,
	HANDLER_ONDBLCLICK = 1 << 11
} JSHandlersBitVector;

/*
 * Base class of all JS input private object data structures.
 */
typedef struct JSInputBase {
    MochaDecoder        *decoder;       /* this window's JS decoder */
    int32               type;           /* layout form element type */
    JSHandlersBitVector	handlers;	/* bit vector for handlers */
} JSInputBase;

/*
 * Base class of event-handling elements like layers and documents.
 */
typedef struct JSInputHandler {
    JSInputBase     base;           /* decoder and type */
    JSObject        *object;        /* this input handler's JS object */
    uint32          event_mask;     /* mask of events in progress */
} JSInputHandler;

/*
 * Base class of input event-capturing elements like layers and documents.
 */
typedef struct JSEventReceiver {
    JSObject        *object;        /* this event receiver's JS object */
    uint32          event_mask;     /* mask of events in progress */
} JSEventReceiver;

/*
 * Base class of input event-handling elements like anchors and form inputs.
 */
typedef struct JSEventCapturer {
    JSEventReceiver base;	    /* this event capturer's receiver base */
    uint32          event_bit;	    /* mask of events being captured */
} JSEventCapturer;

#define base_decoder    base.decoder
#define base_type       base.type
#define base_handlers	base.handlers

/*
 * JS URL object.
 *
 * Location is a special URL: when you set one of its properties, your client
 * window goes to the newly formed address.
 */
typedef struct JSURL {
    JSInputHandler  handler;
    int32           layer_id;
    uint32          index;
    JSString        *href;
    JSString        *target;
    JSString        *text;
} JSURL;

/* JS Document Object
 * Documents exist per-window and per-layer
 */
typedef struct JSDocument {
    JSEventCapturer	capturer;
    MochaDecoder        *decoder;
    JSObject            *object;
    int32               layer_id;     /* The containing layer's id */
    JSObject	        *forms;
    JSObject	        *links;
    JSObject	        *anchors;
    JSObject	        *applets;
    JSObject            *embeds;
    JSObject	        *images;
    JSObject	        *layers;
} JSDocument;

#define URL_NOT_INDEXED ((uint32)-1)

#define url_decoder     handler.base_decoder
#define url_type        handler.base_type
#define url_object      handler.object
#define url_event_mask  handler.event_mask

extern JSURL *
lm_NewURL(JSContext *cx, MochaDecoder *decoder,
          LO_AnchorData *anchor_data, JSObject *document);

extern void
lm_ReplaceURL(MWContext *context, URL_Struct *url_struct);

extern JSBool
lm_GetURL(JSContext *cx, MochaDecoder *decoder, URL_Struct *url_struct);

extern const char *
lm_CheckURL(JSContext *cx, const char *url_string, JSBool checkFile);

extern JSBool
lm_CheckWindowName(JSContext *cx, const char *window_name);

extern PRHashTable *
lm_GetIdToObjectMap(MochaDecoder *decoder);

extern JSBool PR_CALLBACK
lm_BranchCallback(JSContext *cx, JSScript *script);

extern void PR_CALLBACK
lm_ErrorReporter(JSContext *cx, const char *message,
                 JSErrorReport *report);

extern JSObject *
lm_GetFormObjectByID(MWContext *context, int32 layer_id, uint form_id);

extern LO_FormElementStruct *
lm_GetFormElementByIndex(JSContext * cx, JSObject *form_obj, int32 index);

extern JSObject *
lm_GetFormElementFromMapping(JSContext *cx, JSObject *form_obj, uint32 index);

extern JSBool
lm_AddFormElement(JSContext *cx, JSObject *form, JSObject *obj,
                  char *name, uint index);

extern JSBool
lm_ReflectRadioButtonArray(MWContext *context, int32 layer_id, intn form_id,
                           const char *name, PA_Tag * tag);

extern JSBool
lm_SendEvent(MWContext *context, JSObject *obj, JSEvent *event,
             jsval *result);

extern JSBool
lm_HandleEvent(JSContext *cx, JSObject *obj, JSObject *eventObj,
	       jsval funval, jsval *result);

extern JSBool
lm_FindEventHandler(MWContext *context, JSObject *obj, JSObject *eventObj,
		     jsval funval, jsval *result);

extern JSObject *
lm_NewEventObject(MochaDecoder * decoder, JSEvent * pEvent);

typedef struct JSEventNames {
    const char *lowerName;
    const char *mixedName;
} JSEventNames;

extern const char *
lm_EventName(uint32 event_bit);

extern JSEventNames *
lm_GetEventNames(uint32 event_bit);

/*
 * Compile the given attribute and attach it to the JSObject
 */
extern JSBool
lm_CompileEventHandler(MochaDecoder * decoder, PA_Block id, PA_Block data, 
		       int newline_(cx, sizeof *option);
		if (!option)
		    goto bad;

		option_obj =
		    JS_NewObject(cx, &lm_option_class,
				 input->input_decoder->option_prototype, obj);

		if (!option_obj || !JS_SetPrivate(cx, option_obj, option)) {
		    JS_free(cx, option);
		    goto bad;
		}
		option->decoder = HOLD_BACK_COUNT(input->input_decoder);
		option->object = option_obj;
		option->index = (uint32)slot;
		option->indexInForm = form_element->element_index;
		option->data = NULL;
		*vp = OBJECT_TO_JSVAL(option_obj);
		goto good;
	    }
	}
	break;

      case FORM_TYPE_RADIO:
      case FORM_TYPE_CHECKBOX:
	{
	    lo_FormElementToggleData *toggle;

	    toggle = &form_element->element_data->ele_toggle;
	    switch (input_slot) {
	      case INPUT_NAME:
		str = lm_LocalEncodingToStr(context, 
					    (char *)toggle->name);
		break;
	      case INPUT_VALUE:
		str = lm_LocalEncodingToStr(context, 
					    (char *)toggle->value);
		break;
	      case INPUT_STATUS:
		*vp = BOOLEAN_TO_JSVAL(toggle->toggled);
		goto good;
	      case INPUT_DEFAULT_STATUS:
		*vp = BOOLEAN_TO_JSVAL(toggle->default_toggle);
		goto good;

#if DISABLED_READONLY_SUPPORT
	      case INPUT_DISABLED:
	        *vp = BOOLEAN_TO_JSVAL(toggle->disabled);
		goto good;
	      case INPUT_READONLY:
		*vp = BOOLEAN_TO_JSVAL(FALSE);
		goto good;
#endif
	      default:
		/* Don't mess with a user-defined property. */
		goto good;
	    }
	}
	break;

      default:
	{
	    lo_FormElementMinimalData *minimal;

	    minimal = &form_element->element_data->ele_minimal;
	    switch (input_slot) {
	      case INPUT_NAME:
		str = lm_LocalEncodingToStr(context, 
					    (char *)minimal->name);
		break;
	      case INPUT_VALUE:
		str = lm_LocalEncodingToStr(context, 
					    (char *)minimal->value);
		break;
#if DISABLED_READONLY_SUPPORT
	      case INPUT_DISABLED:
	        *vp = BOOLEAN_TO_JSVAL(minimal->disabled);
		goto good;
	      case INPUT_READONLY:
		*vp = BOOLEAN_TO_JSVAL(FALSE); /* minimal elements don't have the readonly attribute. */
		goto good;
#endif
	      default:
		/* Don't mess with a user-defined property. */
		goto good;
	    }
	}
	break;
    }

    if (!str)
	goto bad;
    *vp = STRING_TO_JSVAL(str);

good:
    LO_UnlockLayout();
    return JS_TRUE;

bad:
    LO_UnlockLayout();
    return JS_FALSE;

}

char *
lm_FixNewlines(JSContext *cx, const char *value, JSBool formElement)
{
    size_t size;
    const char *cp;
    char *tp, *new_value;

#if defined XP_PC
    size = 1;
    for (cp = value; *cp != '\0'; cp++) {
	switch (*cp) {
	  case '\r':
	    if (cp[1] != '\n')
		size++;
	    break;
	  case '\n':
	    if (cp > value && cp[-1] != '\r')
		size++;
	    break;
	}
    }
    size += cp - value;
#else
    size = XP_STRLEN(value) + 1;
#endif
    new_value = JS_malloc(cx, size);
    if (!new_value)
	return NULL;
    for (cp = value, tp = new_value; *cp != '\0'; cp++) {
#if defined XP_MAC
	if (*cp == '\n') {
	    if (cp > value && cp[-1] != '\r')
		*tp++ = '\r';
	} else {
	    *tp++ = *cp;
	}
#elif defined XP_PC
	switch (*cp) {
	  case '\r':
	    *tp++ = '\r';
	    if (cp[1] != '\n' && formElement)
		*tp++ = '\n';
	    break;
	  case '\n':
	    if (cp > value && cp[-1] != '\r' && formElement)
		*tp++ = '\r';
	    *tp++ = '\n';
	    break;
	  default:
	    *tp++ = *cp;
	    break;
	}
#else /* XP_UNIX */
	if (*cp == '\r') {
	    if (cp[1] != '\n')
		*tp++ = '\n';
	} else {
	    *tp++ = *cp;
	}
#endif
    }
    *tp = '\0';
    return new_value;
}

PR_STATIC_CALLBACK(JSBool)
input_setProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    JSInput *input;
    enum input_slot input_slot;
    const char *prop_name;
    char *value = NULL;
    LO_FormElementStruct *form_element;
    MochaDecoder *decoder;
    MWContext *context;
    int32 intval;
    jsint slot;

    input = JS_GetInstancePrivate(cx, obj, &lm_input_class, NULL);
    if (!input)
	return JS_TRUE;

    /* If the property is seting a key handler we find out now so
     * that we can tell the front end to send the event. */
    if (JSVAL_IS_STRING(id)) {
	prop_name = JS_GetStringBytes(JSVAL_TO_STRING(id));
	/* XXX use lm_onKeyDown_str etc. initialized by PARAM_ONKEYDOWN */
	if (XP_STRCASECMP(prop_name, "onkeydown") == 0 ||
	    XP_STRCASECMP(prop_name, "onkeyup") == 0 ||
	    XP_STRCASECMP(prop_name, "onkeypress") == 0) {
	    form_element = lm_GetFormElementByIndex(cx, JS_GetParent(cx, obj),
						    input->index);
	    form_element->event_handler_present = TRUE;
	}
	return JS_TRUE;
    }

    XP_ASSERT(JSVAL_IS_INT(id));
    slot = JSVAL_TO_INT(id);

    decoder = input->input_decoder;
    context = decoder->window_context;
    input_slot = slot;
    switch (input_slot) {
      case INPUT_TYPE:
      case INPUT_FORM:
      case INPUT_OPTIONS:
	/* These are immutable. */
	break;
      case INPUT_NAME:
      case INPUT_VALUE:
      case INPUT_DEFAULT_VALUE:
	/* These are string-valued. */
	if (!JSVAL_IS_STRING(*vp) &&
	    !JS_ConvertValue(cx, *vp, JSTYPE_STRING, vp)) {
	    return JS_FALSE;
	}
	value = lm_StrToLocalEncoding(context, JSVAL_TO_STRING(*vp));
	break;
      case INPUT_STATUS:
      case INPUT_DEFAULT_STATUS:
#if DISABLED_READONLY_SUPPORT
      case INPUT_READONLY:
      case INPUT_DISABLED:
#endif
	/* These must be Booleans. */
	if (!JSVAL_IS_BOOLEAN(*vp) &&
	    !JS_ConvertValue(cx, *vp, JSTYPE_BOOLEAN, vp)) {
	    return JS_FALSE;
	}
	break;
      case INPUT_LENGTH:
      case INPUT_SELECTED_INDEX:
	/* These should be integers. */
	if (JSVAL_IS_INT(*vp))
	    intval = JSVAL_TO_INT(*vp);
	else if (!JS_ValueToInt32(cx, *vp, &intval)) {
	    return JS_FALSE;
	}
	break;
    }

    LO_LockLayout();

    form_element = lm_GetFormElementByIndex(cx, JS_GetParent(cx, obj),
					    input->index);
    if (!form_element)
	goto good;

    switch (form_element->element_data->type) {
      case FORM_TYPE_FILE:
	/* if we try to set a file upload widget we better be a signed script */
	if (!lm_CanAccessTarget(cx, JSTARGET_UNIVERSAL_FILE_READ))
	    break;
	/* else fall through... */

      case FORM_TYPE_TEXT:
      case FORM_TYPE_TEXTAREA:	/* XXX we ASSUME common struct prefixes */
      case FORM_TYPE_PASSWORD:
	{
	    lo_FormElementTextData *text;
	    JSBool ok;
	    char * fixed_string;

	    text = &form_element->element_data->ele_text;
	    switch (input_slot) {
	      case INPUT_NAME:
		if (!lm_SaveParamString(cx, &text->name, value))
		    goto bad;
		break;
	      case INPUT_VALUE:
	      case INPUT_DEFAULT_VALUE:
		fixed_string = lm_FixNewlines(cx, value, JS_TRUE);
		if (!fixed_string)
		    goto bad;
		ok = (input_slot == INPUT_VALUE)
		     ? lm_SaveParamString(cx, &text->current_text, fixed_string)
		     : lm_SaveParamString(cx, &text->default_text, fixed_string);

		JS_free(cx, (char *)fixed_string);
		if (!ok)
		    goto bad;
		if (input_slot == INPUT_VALUE && context) {
		    ET_PostManipulateForm(context, (LO_Element *)form_element,
                                  EVENT_CHANGE);
		}
		break;
#if DISABLED_READONLY_SUPPORT
	      case INPUT_DISABLED:
		text->disabled = JSVAL_TO_BOOLEAN(*vp);
		if (context) {
		  ET_PostManipulateForm(context, (LO_Element *)form_element,
					EVENT_CHANGE);
		}
		break;
	      case INPUT_READONLY:
		if (form_element->element_data->type == FORM_TYPE_FILE)
		  break;
		text->readonly = JSVAL_TO_BOOLEAN(*vp);
		if (context) {
		  ET_PostManipulateForm(context, (LO_Element *)form_element,
					EVENT_CHANGE);
		}
		break;
#endif
	      default:
		/* Don't mess with option or user-defined property. */
		goto good;
	    }
	}
	break;

      case FORM_TYPE_SELECT_ONE:
      case FORM_TYPE_SELECT_MULT:
	{
	    lo_FormElementSelectData *selectData;
	    lo_FormElementOptionData *optionData;
	    JSSelectOption *option;
	    int32 i, new_option_cnt, old_option_cnt;

	    selectData = &form_element->element_data->ele_select;
	    switch (slot) {
	      case INPUT_NAME:
		if (!lm_SaveParamString(cx, &selectData->name, value))
		    goto bad;
		break;

	      case INPUT_LENGTH:
		new_option_cnt = intval;
		old_option_cnt = selectData->option_cnt;
		optionData = (lo_FormElementOptionData *) selectData->options;

		/* Remove truncated slots, or clear extended element data. */
		if (new_ayer(MWContext *context, int32 wrap_width, int32 parent_layer_id);

extern void
lm_RestoreLayerState(MWContext *context, int32 layer_id, 
                     LO_BlockInitializeStruct *param);

extern PRHashNumber 
lm_KeyHash(const void *key);

extern JSBool
lm_jsval_to_rgb(JSContext *cx, jsval *vp, LO_Color **rgbp);

extern JSBool
layer_setBgColorProperty(JSContext *cx, JSObject *obj, jsval *vp);

extern JSObject *
lm_GetActiveContainer(MochaDecoder *decoder);

extern JSBool
lm_GetPrincipalsCompromise(JSContext *cx, JSObject *obj);

extern char *
lm_FixNewlines(JSContext *cx, const char *value, JSBool formElement);

extern JSBool PR_CALLBACK
win_open(JSContext *cx, JSObject *obj, uint argc, jsval *argv, jsval *rval);


/* defined in libmocha.h */
#ifdef JSDEBUGGER   

extern void
lm_InitJSDebug(JSRuntime *jsruntime);

extern void
lm_ExitJSDebug(JSRuntime *jsruntime);

#endif

#define IS_MESSAGE_WINDOW(context)                                           \
(((context)->type == MWContextMail)      ||                                  \
 ((context)->type == MWContextNews)      ||                                  \
 ((context)->type == MWContextMailMsg)   ||                                  \
 ((context)->type == MWContextNewsMsg))

/* INTL support */

extern char *
lm_StrToLocalEncoding(MWContext * context, JSString * str);

extern JSString *
lm_LocalEncodingToStr(MWContext * context, char * bytes);

/* end INTL support */

/* MLM */
typedef struct lm_lock_waiter {
    JSLockReleaseFunc       fn;
    void                  * data;
    struct lm_lock_waiter * next;
} lm_lock_waiter;

typedef struct ContextListStr ContextList;

typedef struct WindowGroup LMWindowGroup;

struct WindowGroup {
    LMWindowGroup     *next;
    LMWindowGroup     *prev;

    /* XXXMLM - this entry is currently unused; it should eventually hold
     *           a shared JSContext for the thread group. 
     */
    JSContext         *js_context;

    PRBool            interruptCurrentOp;

    PRMonitor         *owner_monitor;
    PRThread          *thread;
    PRThread          *owner;
    lm_lock_waiter    *waiting_list;
    int32             current_count;

    PRBool            mozWantsLock;
    PRBool            mozGotLock;
    
    PRBool            hasLock;
    JSContext         *lock_context;

    JSTimeout         **js_timeout_insertion_point;
    JSTimeout         *js_timeout_running;

    uint              inputRecurring;

    PREventQueue      *interpret_queue;
    QueueStackElement *queue_stack;
    uint              queue_depth;
    uint              queue_count;
    PRMonitor         *queue_monitor;
    ContextList       *mw_contexts;
    MWContext         *current_context;
    PRBool            done;
};

extern void lm_InitWindowGroups(void);
extern LMWindowGroup *lm_NewWindowGroup(void);
extern void lm_StartWindowGroup(LMWindowGroup *grp);
extern void lm_DestroyWindowGroup(LMWindowGroup *grp);
extern LMWindowGroup *LM_GetDefaultWindowGroup(MWContext *mwc);
extern LMWindowGroup *lm_MWContextToGroup(MWContext *mwc);
extern LMWindowGroup *lm_QueueStackToGroup(QueueStackElement *qse);
extern PREventQueue *LM_MWContextToQueue(MWContext *mwc);
extern PREventQueue *LM_WindowGroupToQueue(LMWindowGroup *lmg);
extern ContextList *lm_GetEntryForContext(LMWindowGroup *grp, MWContext *cx);
extern void LM_AddContextToGroup(LMWindowGroup *grp, MWContext *cx);
extern void LM_RemoveContextFromGroup(MWContext *cx);
extern PRBool LM_IsLocked(LMWindowGroup *grp);
extern void LM_BeginRequest(LMWindowGroup *grp, JSContext *jsc);
extern void LM_EndRequest(LMWindowGroup *grp, JSContext *jsc);

extern void LM_LockJSByGroup(LMWindowGroup *grp);
extern void LM_UnlockJSByGroup(LMWindowGroup *grp);

extern JSBool lm_inited(void);

extern JSContext *LM_GetCrippledContext(void);
extern MochaDecoder *LM_GetCrippledDecoder(void);
extern void LM_SetCrippledDecoder(MochaDecoder *md);
extern JSBool LM_ShouldRunGC(JSContext *cx, JSGCStatus status);

/* MLM */

#endif /* lm_h___ */
