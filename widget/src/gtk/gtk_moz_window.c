/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "MPL"); you may not use this file except in
 * compliance with the MPL.  You may obtain a copy of the MPL at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the MPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the MPL
 * for the specific language governing rights and limitations under the
 * MPL.
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Library General Public License (the "LGPL"), in
 * which case the provisions of the LGPL are applicable instead of
 * those above.  If you wish to allow use of your version of this file
 * only under the terms of the LGPL and not to allow others to use
 * your version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice and
 * other provisions required by the LGPL.  If you do not delete the
 * provisions above, a recipient may use your version of this file
 * under either the MPL or the LGPL.
 */

/* Copyright (C) 1999 Christopher Blizzard, All Rights Reserved.
 */

#include <gtk/gtksignal.h>
#include <gtk/gtkprivate.h>
#include <gdk/gdkx.h>
#include "gtk_moz_window.h"

typedef struct _GtkMozWindowAdjData GtkMozWindowAdjData;
typedef struct _GtkMozWindowChild   GtkMozWindowChild;

struct _GtkMozWindowAdjData {
  gint dx;
  gint dy;
};

struct _GtkMozWindowChild {
  GtkWidget *widget;
  gint x;
  gint y;
};

#define IS_ONSCREEN(x,y) ((x >= G_MINSHORT) && (x <= G_MAXSHORT) && \
                          (y >= G_MINSHORT) && (y <= G_MAXSHORT))

static void gtk_moz_window_class_init (GtkMozWindowClass *class);
static void gtk_moz_window_init (GtkMozWindow *moz_window);
static void gtk_moz_window_set_adjustments(GtkMozWindow *moz_window,
                                           GtkAdjustment *hadjustment,
                                           GtkAdjustment *vadjustment);
static void gtk_moz_window_adjustment_changed(GtkAdjustment *adjustment,
                                              GtkMozWindow *moz_window);
static void gtk_moz_window_realize(GtkWidget *widget);
static void gtk_moz_window_unrealize(GtkWidget *widget);
static void gtk_moz_window_map(GtkWidget *widget);
static void gtk_moz_window_size_request(GtkWidget *widget,
                                        GtkRequisition *requisition);
static void gtk_moz_window_remove(GtkContainer *container,
                                  GtkWidget *widget);
static gint gtk_moz_window_expose             (GtkWidget      *widget, 
                                               GdkEventExpose *event);
static void gtk_moz_window_forall(GtkContainer *container,
                                  gboolean include_internals,
                                  GtkCallback callback,
                                  gpointer callback_data);
static void gtk_moz_window_size_allocate(GtkWidget *widget,
                                         GtkAllocation *allocation);
static void gtk_moz_window_position_child(GtkMozWindow *moz_window,
                                          GtkMozWindowChild *child);
static void gtk_moz_window_allocate_child(GtkMozWindow *moz_window,
                                          GtkMozWindowChild *child);
static void gtk_moz_window_position_children (GtkMozWindow *moz_window);
static void gtk_moz_window_adjust_allocations_recurse (GtkWidget *widget,
                                                       gpointer   cb_data);
static void gtk_moz_window_adjust_allocations (GtkMozWindow *moz_window,
                                               gint       dx,
                                               gint       dy);

static void gtk_moz_window_expose_area   (GtkMozWindow      *moz_window,
                                          gint            x, 
                                          gint            y, 
                                          gint            width, 
                                          gint            height);
static void gtk_moz_window_draw (GtkWidget *widget, GdkRectangle *area);

static GdkFilterReturn gtk_moz_window_filter      (GdkXEvent      *gdk_xevent,
                                                   GdkEvent       *event,
                                                   gpointer        data);
static GdkFilterReturn gtk_moz_window_main_filter (GdkXEvent      *gdk_xevent,
                                                   GdkEvent       *event,
                                                   gpointer        data);


static GtkWidgetClass *parent_class = NULL;

GtkType
gtk_moz_window_get_type (void)
{
  static GtkType moz_window_type = 0;

  if (!moz_window_type)
    {
      static const GtkTypeInfo moz_window_info =
      {
        "GtkMozWindow",
        sizeof(GtkMozWindow),
        sizeof(GtkMozWindowClass),
        (GtkClassInitFunc) gtk_moz_window_class_init,
        (GtkObjectInitFunc) gtk_moz_window_init,
        (GtkArgSetFunc) NULL,
        (GtkArgGetFunc) NULL,
      };

      moz_window_type = gtk_type_unique (GTK_TYPE_CONTAINER, &moz_window_info);
    }
  return moz_window_type;
}

static void
gtk_moz_window_class_init(GtkMozWindowClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;

  object_class = (GtkObjectClass *)class;
  widget_class = (GtkWidgetClass *)class;
  container_class = (GtkContainerClass *)class;

  parent_class = gtk_type_class(GTK_TYPE_CONTAINER);

  widget_class->realize = gtk_moz_window_realize;
  widget_class->unrealize = gtk_moz_window_unrealize;
  widget_class->map = gtk_moz_window_map;
  widget_class->size_request = gtk_moz_window_size_request;
  widget_class->size_allocate = gtk_moz_window_size_allocate;
  widget_class->draw = gtk_moz_window_draw;
  widget_class->expose_event = gtk_moz_window_expose;

  widget_class->set_scroll_adjustments_signal =
    gtk_signal_new("set_scroll_adjustments",
                   GTK_RUN_LAST,
                   object_class->type,
                   GTK_SIGNAL_OFFSET(GtkMozWindowClass, set_scroll_adjustments),
                   gtk_marshal_NONE__POINTER_POINTER,
                   GTK_TYPE_NONE, 2, GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);

  container_class->remove = gtk_moz_window_remove;
  container_class->forall = gtk_moz_window_forall;

  class->set_scroll_adjustments = gtk_moz_window_set_adjustments;
}

void
gtk_moz_window_init(GtkMozWindow *moz_window)
{
  g_return_if_fail(moz_window != NULL);

  moz_window->children = NULL;
  moz_window->width = 100;
  moz_window->height = 100;
  moz_window->hadjustment = NULL;
  moz_window->vadjustment = NULL;
  moz_window->bin_window = NULL;
  moz_window->scroll_x = 0;
  moz_window->scroll_y = 0;
  moz_window->visibility = GDK_VISIBILITY_PARTIAL;
  moz_window->configure_serial = 0;
}

GtkWidget
*gtk_moz_window_new (GtkAdjustment *hadjustment,
                     GtkAdjustment *vadjustment)
{
  GtkMozWindow *moz_window;
  moz_window = gtk_type_new(GTK_TYPE_MOZ_WINDOW);
  gtk_moz_window_set_adjustments(moz_window, hadjustment, vadjustment);
  return GTK_WIDGET(moz_window);
}

void
gtk_moz_window_put (GtkMozWindow *moz_window,
                    GtkWidget *child_widget,
                    gint x, gint y)
{
  GtkMozWindowChild *child;
  g_return_if_fail(moz_window != NULL);
  g_return_if_fail(GTK_IS_MOZ_WINDOW(moz_window));
  g_return_if_fail(child_widget != NULL);
  g_return_if_fail(GTK_IS_WIDGET(child_widget));

  child = g_new(GtkMozWindowChild, 1);
  child->widget = child_widget;
  child->x = x;
  child->y = y;

  moz_window->children = g_list_append(moz_window->children, child);
  
  gtk_widget_set_parent(child_widget, GTK_WIDGET(moz_window));
  if (GTK_WIDGET_REALIZED(moz_window))
    gtk_widget_set_parent_window(child->widget, moz_window->bin_window);
  
  if (!IS_ONSCREEN(x, y))
    GTK_PRIVATE_SET_FLAG(child_widget, GTK_IS_OFFSCREEN);
  
  if (GTK_WIDGET_VISIBLE(moz_window))
    {
      if(GTK_WIDGET_REALIZED(moz_window) &&
         !GTK_WIDGET_REALIZED(child_widget))
        gtk_widget_realize(child_widget);
      /* XXX not sure why I have to check if it's visible, too */ 
      if (GTK_WIDGET_MAPPED(moz_window) &&
          GTK_WIDGET_VISIBLE(child_widget) &&
          !GTK_WIDGET_MAPPED(child_widget))
        gtk_widget_map(child_widget);
    }

  /* XXX uhh...this needs to die. */
  if (GTK_WIDGET_VISIBLE(child_widget) && GTK_WIDGET_VISIBLE(moz_window))
    gtk_widget_queue_resize(child_widget);
}

void
gtk_moz_window_move (GtkMozWindow *moz_window,
                     GtkWidget *child_widget,
                     gint x, gint y)
{
  GList *tmp_list;
  GtkMozWindowChild *child;

  g_return_if_fail(moz_window != NULL);
  g_return_if_fail(GTK_IS_MOZ_WINDOW(moz_window));

  tmp_list = moz_window->children;
  while(tmp_list) {
    child = tmp_list->data;
    tmp_list = tmp_list->next;
    if (child->widget == child_widget)
      {
        child->x = x;
        child->y = y;
        /* XXX this must die */
        if (GTK_WIDGET_VISIBLE(child_widget) && GTK_WIDGET_VISIBLE(moz_window))
          gtk_widget_queue_resize(child_widget);
        return;
      }
  }
}

void
gtk_moz_window_set_size (GtkMozWindow *moz_window,
                         guint width,  guint height)
{
  g_return_if_fail(moz_window != NULL);
  g_return_if_fail(GTK_IS_MOZ_WINDOW(moz_window));

  moz_window->width = width;
  moz_window->height = height;

  moz_window->hadjustment->upper = moz_window->width;
  gtk_signal_emit_by_name(GTK_OBJECT(moz_window->hadjustment), "changed");
  
  moz_window->vadjustment->upper = moz_window->height;
  gtk_signal_emit_by_name(GTK_OBJECT(moz_window->vadjustment), "changed");
}

GtkAdjustment
*gtk_moz_window_get_hadjustment (GtkMozWindow *moz_window)
{
  g_return_val_if_fail(moz_window != NULL, NULL);
  g_return_val_if_fail(GTK_IS_MOZ_WINDOW(moz_window), NULL);
  return moz_window->hadjustment;
}

GtkAdjustment
*gtk_moz_window_get_vadjustment (GtkMozWindow *moz_window)
{
  g_return_val_if_fail(moz_window != NULL, NULL);
  g_return_val_if_fail(GTK_IS_MOZ_WINDOW(moz_window), NULL);
  return moz_window->vadjustment;
}

void
gtk_moz_window_set_hadjustment (GtkMozWindow *moz_window,
                                GtkAdjustment *adjustment)
{
  g_return_if_fail(moz_window != NULL);
  g_return_if_fail(GTK_IS_MOZ_WINDOW(moz_window));
  gtk_moz_window_set_adjustments(moz_window, adjustment, moz_window->vadjustment);
}

void            gtk_moz_window_set_vadjustment (GtkMozWindow *moz_window,
                                                GtkAdjustment *adjustment)
{
  g_return_if_fail(moz_window != NULL);
  g_return_if_fail(GTK_IS_MOZ_WINDOW(moz_window));
  gtk_moz_window_set_adjustments(moz_window, moz_window->hadjustment, adjustment);
}

void
gtk_moz_window_set_adjustments(GtkMozWindow *moz_window,
                               GtkAdjustment *hadjustment,
                               GtkAdjustment *vadjustment)
{
  gboolean need_adjust = FALSE;

  g_return_if_fail(moz_window != NULL);
  g_return_if_fail(GTK_IS_MOZ_WINDOW(moz_window));

  if (hadjustment)
    g_return_if_fail(GTK_IS_ADJUSTMENT(hadjustment));
  else
    hadjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));

  if (vadjustment)
    g_return_if_fail(GTK_IS_ADJUSTMENT(vadjustment));
  else
    vadjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
  
  if (moz_window->hadjustment && (moz_window->hadjustment != hadjustment))
    {
      gtk_signal_disconnect_by_data(GTK_OBJECT(moz_window->hadjustment), moz_window);
      gtk_object_unref(GTK_OBJECT(moz_window->hadjustment));
    }
  
  if (moz_window->vadjustment && (moz_window->vadjustment != vadjustment))
    {
      gtk_signal_disconnect_by_data(GTK_OBJECT(moz_window->vadjustment), moz_window);
      gtk_object_unref(GTK_OBJECT(moz_window->vadjustment));
    }

  if (moz_window->hadjustment != hadjustment)
    {
      moz_window->hadjustment = hadjustment;
      gtk_object_ref(GTK_OBJECT(moz_window->hadjustment));
      gtk_object_sink(GTK_OBJECT(moz_window->hadjustment));
      gtk_signal_connect(GTK_OBJECT(moz_window->hadjustment), "value_changed",
                         (GtkSignalFunc) gtk_moz_window_adjustment_changed,
                         moz_window);
      need_adjust = TRUE;
    }

  if (moz_window->vadjustment != vadjustment)
    {
      moz_window->vadjustment = vadjustment;
      gtk_object_ref(GTK_OBJECT(moz_window->vadjustment));
      gtk_object_sink(GTK_OBJECT(moz_window->vadjustment));

      gtk_signal_connect(GTK_OBJECT(moz_window->vadjustment), "value_changed",
                         (GtkSignalFunc) gtk_moz_window_adjustment_changed,
                         moz_window);
      need_adjust = TRUE;
    }
  
  if (need_adjust)
    gtk_moz_window_adjustment_changed(NULL, moz_window);
}

/* This function is used to find events to process while scrolling
 */

static Bool 
gtk_moz_window_expose_predicate (Display *display, 
                                 XEvent  *xevent, 
                                 XPointer arg)
{
  if ((xevent->type == Expose) || 
      ((xevent->xany.window == *(Window *)arg) &&
       (xevent->type == ConfigureNotify)))
    return True;
  else
    return False;
}


static void gtk_moz_window_adjustment_changed(GtkAdjustment *adjustment,
                                              GtkMozWindow *moz_window)
{
  GtkWidget *widget;
  XEvent xevent;
  gint dx, dy;

  widget = GTK_WIDGET(moz_window);

  dx = (gint)moz_window->hadjustment->value - moz_window->xoffset;
  dy = (gint)moz_window->vadjustment->value - moz_window->yoffset;

  moz_window->xoffset = (gint)moz_window->hadjustment->value;
  moz_window->yoffset = (gint)moz_window->vadjustment->value;
  
  if (!GTK_WIDGET_MAPPED(moz_window))
    {
      gtk_moz_window_position_children(moz_window);
      return;
    }

  gtk_moz_window_adjust_allocations(moz_window, -dx, -dy);

  if (dx > 0) {
    gdk_window_resize(moz_window->bin_window,
                      widget->allocation.width + dx,
                      widget->allocation.height);
    gdk_window_move(moz_window->bin_window, -dx, 0);
    gdk_window_move_resize(moz_window->bin_window,
                           0, 0,
                           widget->allocation.width,
                           widget->allocation.height);
    gtk_moz_window_expose_area(moz_window,
                               MAX((gint)widget->allocation.width - dx, 0),
                               0,
                               MIN(dx, widget->allocation.width),
                               widget->allocation.height);
  }
  else if (dx < 0) {
    gdk_window_move_resize(moz_window->bin_window,
                           dx, 0,
                           widget->allocation.width - dx,
                           widget->allocation.height);
    gdk_window_move(moz_window->bin_window, 0, 0);
    gdk_window_resize(moz_window->bin_window,
                      widget->allocation.width,
                      widget->allocation.height);
    gtk_moz_window_expose_area(moz_window,
                               0, 0,
                               MIN(-dx, widget->allocation.width),
                               widget->allocation.height);
  }
  
  if (dy > 0) {
    gdk_window_resize(moz_window->bin_window,
                      widget->allocation.width,
                      widget->allocation.height + dy);
    gdk_window_move (moz_window->bin_window, 0, -dy);
    gdk_window_move_resize(moz_window->bin_window,
                           0, 0,
                           widget->allocation.width,
                           widget->allocation.height);
    gtk_moz_window_expose_area(moz_window,
                               0, 
                               MAX((gint)widget->allocation.height - dy, 0),
                               widget->allocation.width,
                               MIN(dy, widget->allocation.height));
  }
  else if (dy < 0) {
    gdk_window_move_resize(moz_window->bin_window,
                           0, dy,
                           widget->allocation.width,
                           widget->allocation.height - dy);
    gdk_window_move(moz_window->bin_window, 0, 0);
    gdk_window_resize(moz_window->bin_window,
                      widget->allocation.width,
                      widget->allocation.height);
    gtk_moz_window_expose_area(moz_window,
                               0, 0,
                               widget->allocation.width,
                               MIN(-dy, (gint)widget->allocation.height));
  }
  gtk_moz_window_position_children(moz_window);

  /* We have to make sure that all exposes from this scroll get
   * processed before we scroll again, or the expose events will
   * have invalid coordinates.
   *
   * We also do expose events for other windows, since otherwise
   * their updating will fall behind the scrolling 
   *
   * This also avoids a problem in pre-1.0 GTK where filters don't
   * have access to configure events that were compressed.
   */

  gdk_flush();

  while (XCheckIfEvent(GDK_WINDOW_XDISPLAY (moz_window->bin_window),
                       &xevent,
                       gtk_moz_window_expose_predicate,
                       (XPointer)&GDK_WINDOW_XWINDOW (moz_window->bin_window)))
    {
      GdkEvent event;
      GtkWidget *event_widget;

      if ((xevent.xany.window == GDK_WINDOW_XWINDOW (moz_window->bin_window)) &&
          (gtk_moz_window_filter (&xevent, &event, moz_window) == GDK_FILTER_REMOVE))
        continue;
      
      if (xevent.type == Expose)
        {
          event.expose.window = gdk_window_lookup (xevent.xany.window);
          gdk_window_get_user_data (event.expose.window, 
                                    (gpointer *)&event_widget);

          if (event_widget)
            {
              event.expose.type = GDK_EXPOSE;
              event.expose.area.x = xevent.xexpose.x;
              event.expose.area.y = xevent.xexpose.y;
              event.expose.area.width = xevent.xexpose.width;
              event.expose.area.height = xevent.xexpose.height;
              event.expose.count = xevent.xexpose.count;
              
              gdk_window_ref (event.expose.window);
              gtk_widget_event (event_widget, &event);
              gdk_window_unref (event.expose.window);
            }
        }
    }
}

static void gtk_moz_window_realize(GtkWidget *widget)
{
  GList *tmp_list;
  GtkMozWindow *moz_window;
  GdkWindowAttr attributes;
  gint attributes_mask;

  g_return_if_fail(widget != NULL);
  g_return_if_fail(GTK_IS_MOZ_WINDOW(widget));

  moz_window = GTK_MOZ_WINDOW(widget);
  GTK_WIDGET_SET_FLAGS(moz_window, GTK_REALIZED);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes.event_mask = GDK_VISIBILITY_NOTIFY_MASK;

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  
  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
                                   &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, widget);

  attributes.x = 0;
  attributes.y = 0;
  attributes.event_mask = GDK_EXPOSURE_MASK | 
                          gtk_widget_get_events (widget);

  moz_window->bin_window = gdk_window_new(widget->window,
                                          &attributes, attributes_mask);
  gdk_window_set_user_data(moz_window->bin_window, widget);
  widget->style = gtk_style_attach (widget->style, widget->window);
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
  gtk_style_set_background (widget->style, moz_window->bin_window, GTK_STATE_NORMAL);

  gdk_window_add_filter (widget->window, gtk_moz_window_main_filter, moz_window);
  gdk_window_add_filter (moz_window->bin_window, gtk_moz_window_filter, moz_window);

  gdk_window_set_static_gravities (moz_window->bin_window, TRUE);

  tmp_list = moz_window->children;
  while (tmp_list)
    {
      GtkMozWindowChild *child = tmp_list->data;
      tmp_list = tmp_list->next;

      gtk_widget_set_parent_window(child->widget, moz_window->bin_window);
    }
  
}
static void gtk_moz_window_unrealize(GtkWidget *widget)
{

  GtkMozWindow *moz_window;
  g_return_if_fail(widget != NULL);
  g_return_if_fail(GTK_IS_MOZ_WINDOW(widget));
  
  moz_window = GTK_MOZ_WINDOW(widget);
  gdk_window_set_user_data(moz_window->bin_window, NULL);
  gdk_window_destroy(moz_window->bin_window);
  moz_window->bin_window = NULL;

  if (GTK_WIDGET_CLASS(parent_class)->unrealize)
    (*GTK_WIDGET_CLASS(parent_class)->unrealize)(widget);
      
}

static void gtk_moz_window_map(GtkWidget *widget)
{
  GList *tmp_list;
  GtkMozWindow *moz_window;

  g_return_if_fail(widget != NULL);
  g_return_if_fail(GTK_IS_MOZ_WINDOW(widget));

  moz_window = GTK_MOZ_WINDOW(widget);
  GTK_WIDGET_SET_FLAGS(widget, GTK_MAPPED);

  tmp_list = moz_window->children;
  while(tmp_list)
    {
      GtkMozWindowChild *child = tmp_list->data;
      tmp_list = tmp_list->next;

      if (GTK_WIDGET_VISIBLE(child->widget))
        {
          if (!GTK_WIDGET_MAPPED(child->widget) &&
              !GTK_WIDGET_IS_OFFSCREEN(child->widget))
            gtk_widget_map(child->widget);
        }
    }
  gdk_window_show(moz_window->bin_window);
  gdk_window_show(widget->window);
}

static void gtk_moz_window_size_request(GtkWidget *widget,
                                        GtkRequisition *requisition)
{
  GList *tmp_list;
  GtkMozWindow *moz_window;
  
  g_return_if_fail(widget != NULL);
  g_return_if_fail(GTK_IS_MOZ_WINDOW(widget));

  moz_window = GTK_MOZ_WINDOW(widget);

  requisition->width = 0;
  requisition->height = 0;

  tmp_list = moz_window->children;

  while(tmp_list)
    {
      GtkMozWindowChild *child = tmp_list->data;
      GtkRequisition child_requisition;
      tmp_list = tmp_list->next;
      gtk_widget_size_request(child->widget, &child_requisition);
    }
}

static void gtk_moz_window_remove(GtkContainer *container,
                                  GtkWidget *widget)
{
  GList *tmp_list;
  GtkMozWindow *moz_window;
  GtkMozWindowChild *child = NULL;
  
  g_return_if_fail(container != NULL);
  g_return_if_fail(GTK_IS_MOZ_WINDOW(container));
  moz_window = GTK_MOZ_WINDOW(container);
  tmp_list = moz_window->children;
  while (tmp_list)
    {
      child = tmp_list->data;
      if (child->widget == widget)
        break;
      tmp_list = tmp_list->next;
    }

  if (tmp_list)
    {
      gtk_widget_unparent(widget);
      moz_window->children = g_list_remove_link(moz_window->children, tmp_list);
      g_list_free_1(tmp_list);
      g_free(child);
    }
  GTK_PRIVATE_UNSET_FLAG(widget, GTK_IS_OFFSCREEN);
}

static void gtk_moz_window_forall(GtkContainer *container,
                                  gboolean include_internals,
                                  GtkCallback callback,
                                  gpointer callback_data)
{
  GtkMozWindow *moz_window;
  GtkMozWindowChild *child;
  GList *tmp_list;

  g_return_if_fail(container != NULL);
  g_return_if_fail(GTK_IS_MOZ_WINDOW(container));
  g_return_if_fail(callback != NULL);

  moz_window = GTK_MOZ_WINDOW(container);
  tmp_list = moz_window->children;
  while (tmp_list)
    {
      child = tmp_list->data;
      tmp_list = tmp_list->next;
      (*callback)(child->widget, callback_data);
    }
}


static void gtk_moz_window_size_allocate(GtkWidget *widget,
                                         GtkAllocation *allocation)
{
  GList *tmp_list;
  GtkMozWindow *moz_window;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MOZ_WINDOW(widget));

  widget->allocation = *allocation;

  moz_window = GTK_MOZ_WINDOW(widget);
  tmp_list = moz_window->children;

  while (tmp_list)
    {
      GtkMozWindowChild *child = tmp_list->data;
      tmp_list = tmp_list->next;
      gtk_moz_window_position_child(moz_window, child);
      gtk_moz_window_allocate_child(moz_window, child);
    }
  if (GTK_WIDGET_REALIZED(widget))
    {
      gdk_window_move_resize(widget->window,
                             allocation->x, allocation->y,
                             allocation->width, allocation->height);
      gdk_window_move_resize(GTK_MOZ_WINDOW(widget)->bin_window,
                             0, 0,
                             allocation->width, allocation->height);
    }

  moz_window->hadjustment->page_size = allocation->width;
  moz_window->hadjustment->page_increment = allocation->width / 2;
  moz_window->hadjustment->lower = 0;
  moz_window->hadjustment->upper = moz_window->width;
  gtk_signal_emit_by_name(GTK_OBJECT(moz_window->hadjustment), "changed");

  moz_window->vadjustment->page_size = allocation->height;
  moz_window->vadjustment->page_increment = allocation->height / 2;
  moz_window->vadjustment->lower = 0;
  moz_window->vadjustment->upper = moz_window->height;
  
}

static void gtk_moz_window_position_child(GtkMozWindow *moz_window,
                                          GtkMozWindowChild *child)
{
  gint x;
  gint y;
  x = child->x - moz_window->xoffset;
  y = child->y - moz_window->yoffset;

  if (IS_ONSCREEN(x, y))
    {
      if (GTK_WIDGET_MAPPED(moz_window) &&
          GTK_WIDGET_VISIBLE(child->widget))
        {
          if (!GTK_WIDGET_MAPPED(child->widget))
            gtk_widget_map(child->widget);
        }
      if (GTK_WIDGET_IS_OFFSCREEN(child->widget))
        GTK_PRIVATE_UNSET_FLAG(child->widget, GTK_IS_OFFSCREEN);
    }
  else 
    {
      if (!GTK_WIDGET_IS_OFFSCREEN(child->widget))
        GTK_PRIVATE_SET_FLAG(child->widget, GTK_IS_OFFSCREEN);
      if (GTK_WIDGET_MAPPED(child->widget))
        gtk_widget_unmap(child->widget);
    }
}

static void gtk_moz_window_allocate_child(GtkMozWindow *moz_window,
                                          GtkMozWindowChild *child)
{
  GtkAllocation allocation;
  GtkRequisition requisition;
  
  allocation.x = child->x - moz_window->xoffset;
  allocation.y = child->y - moz_window->yoffset;
  gtk_widget_get_child_requisition (child->widget, &requisition);
  allocation.width = requisition.width;
  allocation.height = requisition.height;
  
  gtk_widget_size_allocate (child->widget, &allocation);

}

/* Although GDK does have a GDK_VISIBILITY_NOTIFY event,
 * there is no corresponding event in GTK, so we have
 * to get the events from a filter
 */
static GdkFilterReturn 
gtk_moz_window_main_filter (GdkXEvent *gdk_xevent,
                            GdkEvent  *event,
                            gpointer   data)
{
  XEvent *xevent;
  GtkMozWindow *moz_window;

  xevent = (XEvent *)gdk_xevent;
  moz_window = GTK_MOZ_WINDOW (data);

  if (xevent->type == VisibilityNotify)
    {
      switch (xevent->xvisibility.state)
        {
        case VisibilityFullyObscured:
          moz_window->visibility = GDK_VISIBILITY_FULLY_OBSCURED;
          break;

        case VisibilityPartiallyObscured:
          moz_window->visibility = GDK_VISIBILITY_PARTIAL;
          break;

        case VisibilityUnobscured:
          moz_window->visibility = GDK_VISIBILITY_UNOBSCURED;
          break;
        }

      return GDK_FILTER_REMOVE;
    }
  return GDK_FILTER_CONTINUE;
}

/* The main event filter. Actually, we probably don't really need
 * to install this as a filter at all, since we are calling it
 * directly above in the expose-handling hack. But in case scrollbars
 * are fixed up in some manner...
 *
 * This routine identifies expose events that are generated when
 * we've temporarily moved the bin_window_origin, and translates
 * them or discards them, depending on whether we are obscured
 * or not.
 */
static GdkFilterReturn 
gtk_moz_window_filter (GdkXEvent *gdk_xevent,
                       GdkEvent  *event,
                       gpointer   data)
{

  XEvent *xevent;
  GtkMozWindow *moz_window;

  xevent = (XEvent *)gdk_xevent;
  moz_window = GTK_MOZ_WINDOW (data);

  switch (xevent->type)
    {
    case Expose:
      if (xevent->xexpose.serial == moz_window->configure_serial)
        {
          if (moz_window->visibility == GDK_VISIBILITY_UNOBSCURED)
            return GDK_FILTER_REMOVE;
          else
            {
              xevent->xexpose.x += moz_window->scroll_x;
              xevent->xexpose.y += moz_window->scroll_y;
              
              break;
            }
        }
      break;
      
    case ConfigureNotify:
       if ((xevent->xconfigure.x != 0) || (xevent->xconfigure.y != 0))
        {
          moz_window->configure_serial = xevent->xconfigure.serial;
          moz_window->scroll_x = xevent->xconfigure.x;
          moz_window->scroll_y = xevent->xconfigure.y;
        }
      break;
    }
  return GDK_FILTER_CONTINUE;
}


static void
gtk_moz_window_expose_area (GtkMozWindow      *moz_window,
                            gint            x, 
                            gint            y, 
                            gint            width, 
                            gint            height)
{
  if (moz_window->visibility == GDK_VISIBILITY_UNOBSCURED)
    {
      GdkEventExpose event;
      
      event.type = GDK_EXPOSE;
      event.send_event = TRUE;
      event.window = moz_window->bin_window;
      event.count = 0;
      
      event.area.x = x;
      event.area.y = y;
      event.area.width = width;
      event.area.height = height;
      
      gdk_window_ref (event.window);
      gtk_widget_event (GTK_WIDGET (moz_window), (GdkEvent *)&event);
      gdk_window_unref (event.window);
    }

}

static void
gtk_moz_window_position_children (GtkMozWindow *moz_window)
{
  GList *tmp_list;

  tmp_list = moz_window->children;
  while (tmp_list)
    {
      GtkMozWindowChild *child = tmp_list->data;
      tmp_list = tmp_list->next;
      
      gtk_moz_window_position_child (moz_window, child);
    }
}
 
static void
gtk_moz_window_adjust_allocations (GtkMozWindow *moz_window,
                                   gint       dx,
                                   gint       dy)
{
  GList *tmp_list;
  GtkMozWindowAdjData data;

  data.dx = dx;
  data.dy = dy;

  tmp_list = moz_window->children;
  while (tmp_list)
    {
      GtkMozWindowChild *child = tmp_list->data;
      tmp_list = tmp_list->next;
      
      child->widget->allocation.x += dx;
      child->widget->allocation.y += dy;

      if (GTK_WIDGET_NO_WINDOW (child->widget) &&
          GTK_IS_CONTAINER (child->widget))
        gtk_container_forall (GTK_CONTAINER (child->widget), 
                              gtk_moz_window_adjust_allocations_recurse,
                              &data);
    }
}

static void gtk_moz_window_adjust_allocations_recurse (GtkWidget *widget,
                                                       gpointer   cb_data)
{
  GtkMozWindowAdjData *data = cb_data;
  
  widget->allocation.x += data->dx;
  widget->allocation.y += data->dy;
  
  if (GTK_WIDGET_NO_WINDOW (widget) &&
      GTK_IS_CONTAINER (widget))
    gtk_container_forall (GTK_CONTAINER (widget), 
                          gtk_moz_window_adjust_allocations_recurse,
                          cb_data);

}

static gint gtk_moz_window_expose             (GtkWidget      *widget, 
                                               GdkEventExpose *event)
{
  GList *tmp_list;
  GtkMozWindow *moz_window;
  GdkEventExpose child_event;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_MOZ_WINDOW (widget), FALSE);

  moz_window = GTK_MOZ_WINDOW (widget);

  if (event->window != moz_window->bin_window)
    return FALSE;
  tmp_list = moz_window->children;
  while (tmp_list)
    {
      GtkMozWindowChild *child = tmp_list->data;
      tmp_list = tmp_list->next;

      child_event = *event;
      if (GTK_WIDGET_DRAWABLE (child->widget) &&
          GTK_WIDGET_NO_WINDOW (child->widget) &&
          gtk_widget_intersect (child->widget, &event->area, &child_event.area))
        gtk_widget_event (child->widget, (GdkEvent*) &child_event);
    }
  return FALSE;

}

static void 
gtk_moz_window_draw (GtkWidget *widget, GdkRectangle *area)
{
  GList *tmp_list;
  GtkMozWindow *moz_window;
  GdkRectangle child_area;

  g_return_if_fail(widget != NULL);
  g_return_if_fail(GTK_IS_MOZ_WINDOW(widget));

  moz_window = GTK_MOZ_WINDOW(widget);

  /* We don't have any way of telling themes about this properly,
   * so we just assume a background pixmap
   */
  if (!GTK_WIDGET_APP_PAINTABLE (widget))
    gdk_window_clear_area (moz_window->bin_window,
                           area->x, area->y, area->width, area->height);
  tmp_list = moz_window->children;
  while (tmp_list)
    {
      GtkMozWindowChild *child = tmp_list->data;
      tmp_list = tmp_list->next;

      if (gtk_widget_intersect(child->widget, area, &child_area))
        gtk_widget_draw(child->widget, &child_area);
    }
}
