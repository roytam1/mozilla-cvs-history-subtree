/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "MPL"); you may not use this file except in
 * compliance with the MPL.  You may obtain a copy of the MPL at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the MPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the MPL
 * for the specific language governing rights and limitations under the
 * MPL.
 *
 * The Initial Developers of this code under the MPL are Owen Taylor
 * <otaylor@redhat.com> and Christopher Blizzard <blizzard@redhat.com>.
 * Portions created by the Initial Developers are Copyright (C) 1999
 * Owen Taylor and Christopher Blizzard.  All Rights Reserved.
 */

#include "gdksuperwin.h"

static void gdk_superwin_class_init(GdkSuperWinClass *klass);
static void gdk_superwin_init(GdkSuperWin *superwin);
static Bool gdk_superwin_expose_predicate(Display *display, 
                                          XEvent  *xevent, 
                                          XPointer arg);

typedef struct _GdkSuperWinTranslate GdkSuperWinTranslate;

struct _GdkSuperWinTranslate
{
  unsigned long serial;
  gint dx;
  gint dy;
};

GtkType
gdk_superwin_get_type(void)
{
  static GtkType superwin_type = 0;
  
  if (!superwin_type)
    {
      static const GtkTypeInfo superwin_info =
      {
        "GtkSuperWin",
          sizeof(GdkSuperWin),
          sizeof(GdkSuperWinClass),
          (GtkClassInitFunc) gdk_superwin_class_init,
          (GtkObjectInitFunc) gdk_superwin_init,
          /* reserved_1 */ NULL,
          /* reserved_2 */ NULL,
          (GtkClassInitFunc) NULL
      };
      
      superwin_type = gtk_type_unique (gtk_object_get_type(), &superwin_info);
    }
  return superwin_type;
}

static void
gdk_superwin_class_init(GdkSuperWinClass *klass)
{
  GtkObjectClass *object_class;

  object_class = GTK_OBJECT_CLASS(klass);

  /* XXX do we need a finalize in here to destroy the
     window? */
}

static void
gdk_superwin_init(GdkSuperWin *superwin)
{
  
}

static GdkFilterReturn  gdk_superwin_bin_filter   (GdkXEvent *gdk_xevent,
                                                   GdkEvent  *event,
                                                   gpointer   data);
static GdkFilterReturn  gdk_superwin_shell_filter (GdkXEvent *gdk_xevent,
                                                   GdkEvent  *event,
                                                   gpointer   data);
static void             gdk_superwin_expose_area  (GdkSuperWin *superwin,
                                                   gint         x,
                                                   gint         y,
                                                   gint         width,
                                                   gint         height);
static gboolean gravity_works;

GdkSuperWin *
gdk_superwin_new (GdkWindow *parent_window,
                  guint      x,
                  guint      y,
                  guint      width,
                  guint      height)
{
  GdkWindowAttr attributes;
  gint attributes_mask;
  GdkSuperWin *superwin = gtk_type_new(GDK_TYPE_SUPERWIN);

  superwin->translate_queue = NULL;

  superwin->event_func = NULL;
  superwin->func_data = NULL;
  superwin->notify = NULL;

  /* Create the shell (clipping) window */
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = x;
  attributes.y = y;
  attributes.width = width;
  attributes.height = height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = GDK_VISIBILITY_NOTIFY_MASK;

  attributes_mask = GDK_WA_X | GDK_WA_Y;

  superwin->shell_window = gdk_window_new (parent_window,
					   &attributes, attributes_mask);

  /* Create the bin window for drawing */

  attributes.x = 0;
  attributes.y = 0;
  attributes.event_mask = GDK_EXPOSURE_MASK;

  superwin->bin_window = gdk_window_new (superwin->shell_window,
					 &attributes, attributes_mask);
  gdk_window_show (superwin->bin_window);

  gdk_window_add_filter (superwin->shell_window, gdk_superwin_shell_filter, superwin);
  gdk_window_add_filter (superwin->bin_window, gdk_superwin_bin_filter, superwin);

  gravity_works = gdk_window_set_static_gravities (superwin->bin_window, TRUE);

  return superwin;
}

/* XXX this should really be part of the object... */
/* XXX and it should chain up to the object's destructor */

void gdk_superwin_destroy(GdkSuperWin *superwin)
{
  gdk_window_remove_filter(superwin->shell_window,
                           gdk_superwin_shell_filter,
                           superwin);
  gdk_window_remove_filter(superwin->bin_window,
                           gdk_superwin_bin_filter,
                           superwin);
  gdk_window_destroy(superwin->bin_window);
  gdk_window_destroy(superwin->shell_window);
  /* XXX this needs to free itself */
}

void         
gdk_superwin_scroll (GdkSuperWin *superwin,
		     gint dx,
		     gint dy)
{
  GdkSuperWinTranslate *translate;
  gint width, height;

  gdk_window_get_size (superwin->shell_window, &width, &height);

  g_print("scrolling x %d y %d\n", dx, dy);

  XFlush(GDK_DISPLAY());

  if (dx > 0 || dy > 0) 
    {
      translate = g_new (GdkSuperWinTranslate, 1);
      translate->dx = MAX (0, dx);
      translate->dy = MAX (0, dy);
      translate->serial = NextRequest (GDK_DISPLAY());
      g_print("adding serial %lx with vals %d %d to translate queue\n",
              translate->serial, translate->dx, translate->dy);
      superwin->translate_queue = g_list_append (superwin->translate_queue, translate);
    }

  g_print("move_resizing window to %d, %d %d, %d\n",
          MIN (0, -dx), MIN (0, -dy),
          width + ABS(dx), height + ABS(dy));


  gdk_window_move_resize (superwin->bin_window,
			  MIN (0, -dx), MIN (0, -dy),
			  width + ABS(dx), height + ABS(dy));

  XFlush(GDK_DISPLAY());

  g_print("move window to %d, %d\n",
          MIN (0, -dx) + dx, MIN (0, -dy) + dy);

  gdk_window_move (superwin->bin_window,
		   MIN (0, -dx) + dx, MIN (0, -dy) + dy);  

  XFlush(GDK_DISPLAY());

  if (dx < 0 || dy < 0) 
    {
      translate = g_new (GdkSuperWinTranslate, 1);
      translate->dx = MIN (0, dx);
      translate->dy = MIN (0, dy);
      translate->serial = NextRequest (GDK_DISPLAY());
      g_print("adding serial %lx with vals %d %d to translate queue\n",
              translate->serial, translate->dx, translate->dy);
      superwin->translate_queue = g_list_append (superwin->translate_queue, translate);
    }

  g_print("move_resizing window to %d,%d %d,%d\n",
          0, 0, width, height);

  gdk_window_move_resize (superwin->bin_window,
			  0, 0, width, height);

  XFlush(GDK_DISPLAY());

  if (dx < 0)
    gdk_superwin_expose_area (superwin,
			      MAX (width + dx, 0), 0,
			      MIN (-dx, width),    height);
  else
    gdk_superwin_expose_area (superwin,
			      0,                   0,
			      MIN (dx, width),     height);

  if (dy < 0)
    gdk_superwin_expose_area (superwin, 
			      MIN (dx, 0),              MAX (height + dy, 0),
			      MIN (width - ABS(dx), 0), MIN (-dy, height));
  else
    gdk_superwin_expose_area (superwin, 
			      MIN (dx, 0),              0,
			      MIN (width - ABS(dx), 0), MIN (dy, height));
  
}

void         
gdk_superwin_set_event_func (GdkSuperWin    *superwin,
                             GdkSuperWinFunc event_func,
                             gpointer        func_data,
                             GDestroyNotify  notify)
{
  if (superwin->notify && superwin->func_data)
    superwin->notify (superwin->func_data);
  
  superwin->event_func = event_func;
  superwin->func_data = func_data;
  superwin->notify = notify;

  /* g_print("setting func to %p, arg to %p in win %p\n",
     superwin->event_func, superwin->func_data, superwin); */

}

void gdk_superwin_resize (GdkSuperWin *superwin,
			  gint         width,
			  gint         height)
{
  gdk_window_resize (superwin->bin_window, width, height);
  gdk_window_resize (superwin->shell_window, width, height);
}

static void
gdk_superwin_expose_area  (GdkSuperWin *superwin,
                           gint         x,
                           gint         y,
                           gint         width,
                           gint         height)
{
  
}

static GdkFilterReturn 
gdk_superwin_bin_filter (GdkXEvent *gdk_xevent,
			 GdkEvent  *event,
			 gpointer   data)
{
  XEvent *xevent = (XEvent *)gdk_xevent;
  GdkSuperWin *superwin = data;
  GdkSuperWinTranslate *translate;
  GList *tmp_list;
  
  g_print("filter for serial %lx\n",
          xevent->xany.serial);

  /* XXX implement this hell

     Ok, this is ugly.  Because of some race conditions in the layout
     engine in mozilla, we need to go out and make sure that we get
     the last ConfigureNotify event for this window.  If you don't
     then the layout engine and the front end will get into a
     deathgrip from which you can never return.  However, the fact
     that gaffaw scrolling is in effect with translated expose events
     makes life interesting.

     If we go out and get the extra ConfigureNotify events we have to
     get the Expose events that are also part of that logical request.
     We do this by taking the old serial number from the
     ConfigureNotify event and the new ConfigureNotify event and use
     those as the basis for a predicate.  The predicate will never
     return true but it will find all of the Expose events that fall
     into that range of serials, create an entry in a the expose
     translation list for them ( by serial since we already know the
     expose serial number in the predicate ) and then process as
     normal.  The expose handler will be processed as part of the
     event loop as normal.  Every time it is called it will walk the
     list and make any adjustments to its x/y that are required.

  */

  switch (xevent->xany.type) {
  case Expose:
    tmp_list = superwin->translate_queue;
    g_print("got an expose event at %d,%d %d,%d\n",
          xevent->xexpose.x, xevent->xexpose.y,
          xevent->xexpose.width, xevent->xexpose.height);
    while (tmp_list) {
      translate = tmp_list->data;
      
      xevent->xexpose.x += translate->dx;
      xevent->xexpose.y += translate->dy;
      g_print("translated event to %d,%d %d,%d\n",
              xevent->xexpose.x, xevent->xexpose.y,
              xevent->xexpose.width, xevent->xexpose.height);
      tmp_list = tmp_list->next;
    }
    break;

  case ConfigureNotify:
    g_print("got a configurenotify with serial %lx\n",
            xevent->xany.serial);
    gdk_superwin_clear_translate_queue(superwin, xevent->xany.serial);
    break;
  }

  if (superwin->event_func) {
    /* g_print("calling func %p with arg %p in win %p\n",
       superwin->event_func, superwin->func_data, superwin); */
    superwin->event_func (superwin, xevent, superwin->func_data);
  }

  return GDK_FILTER_REMOVE;
}

static GdkFilterReturn 
gdk_superwin_shell_filter (GdkXEvent *gdk_xevent,
			  GdkEvent  *event,
			  gpointer   data)
{
  XEvent *xevent = (XEvent *)gdk_xevent;
  GdkSuperWin *superwin = data;
  
  if (xevent->type == VisibilityNotify)
    {
      switch (xevent->xvisibility.state)
        {
        case VisibilityFullyObscured:
          superwin->visibility = GDK_VISIBILITY_FULLY_OBSCURED;
          break;
          
        case VisibilityPartiallyObscured:
          superwin->visibility = GDK_VISIBILITY_PARTIAL;
          break;
          
        case VisibilityUnobscured:
          superwin->visibility = GDK_VISIBILITY_UNOBSCURED;
          break;
        }
      
      return GDK_FILTER_REMOVE;
    }
  
  return GDK_FILTER_CONTINUE;
}

void
gdk_superwin_clear_translate_queue(GdkSuperWin *superwin, unsigned long serial)
{
  GdkSuperWinTranslate *translate;
  GList *tmp_list;
  GList *link_to_remove = NULL;

  if (superwin->translate_queue) {
    tmp_list = superwin->translate_queue;
    while (tmp_list) {
      translate = tmp_list->data;
      if (serial == translate->serial) {
        g_print("removing serial %lx\n",
                serial);
        g_free (tmp_list->data);
        superwin->translate_queue = g_list_remove_link (superwin->translate_queue, tmp_list);
        link_to_remove = tmp_list;
      }
      tmp_list = tmp_list->next;
      if (link_to_remove) {
        g_list_free_1(link_to_remove);
        link_to_remove = NULL;
      }
    }
  }
}

Bool gdk_superwin_expose_predicate(Display *display, XEvent  *xevent, XPointer arg)
{
  
}
