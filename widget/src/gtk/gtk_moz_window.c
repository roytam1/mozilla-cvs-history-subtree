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
 * NPL.
 */

#include "gtk_moz_window.h"
#include <gtk/gtksignal.h>

static void gtk_moz_window_class_init     (GtkMozWindowClass      *klass);
static void gtk_moz_window_init           (GtkMozWindow           *moz_window);
static void gtk_moz_window_map            (GtkWidget              *widget);
static void gtk_moz_window_realize        (GtkWidget              *widget);
static void gtk_moz_window_size_request   (GtkWidget              *widget,
					   GtkRequisition         *requistion);
static void gtk_moz_window_size_allocate  (GtkWidget              *widget,
					   GtkAllocation          *allocation);
static void gtk_moz_window_paint          (GtkWidget              *widget,
					   GdkRectangle           *area);
static void gtk_moz_window_draw           (GtkWidget              *widget,
					   GdkRectangle           *area);
static gint gtk_moz_window_expose         (GtkWidget              *widget,
					   GdkEventExpose         *event);
static void gtk_moz_window_add            (GtkContainer           *container,
					   GtkWidget              *widget);
static void gtk_moz_window_remove         (GtkContainer           *container,
					   GtkWidget              *widget);
static void gtk_moz_window_forall         (GtkContainer           *container,
					   gboolean                include_internals,
					   GtkCallback             callback,
					   gpointer                callback_data);
static GtkType gtk_moz_window_child_type  (GtkContainer           *container);
static void gtk_moz_window_set_adjustments (GtkMozWindow          *moz_window,
					    GtkAdjustment         *hadj,
					    GtkAdjustment         *vadj);
static void gtk_moz_window_adjustment_changed (GtkAdjustment *adjustment,
					       GtkMozWindow  *moz_window);
static void gtk_moz_window_expose_area        (GtkMozWindow  *moz_window,
					       gint            x, 
					       gint            y, 
					       gint            width, 
					       gint            height);

static GtkContainerClass *parent_class = NULL;

GtkType
gtk_moz_window_get_type (void) {
  static GtkType moz_window_type = 0;
  
  if (!moz_window_type) {
    static const GtkTypeInfo moz_window_info =
    {
      "GtkMozWindow",
      sizeof(GtkMozWindow),
      sizeof(GtkMozWindowClass),
      (GtkClassInitFunc) gtk_moz_window_class_init,
      (GtkObjectInitFunc) gtk_moz_window_init,
      /* reserved_1 */ NULL,
      /* reserved_2 */ NULL,
      (GtkClassInitFunc) NULL,
    };
    
    moz_window_type = gtk_type_unique (GTK_TYPE_CONTAINER, &moz_window_info);
  }
  return moz_window_type;
}

static void
gtk_moz_window_class_init (GtkMozWindowClass *class) {
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;
  
  object_class = (GtkObjectClass *)class;
  widget_class = (GtkWidgetClass *)class;
  container_class = (GtkContainerClass *)class;
  
  parent_class = gtk_type_class(GTK_TYPE_CONTAINER);
  
  widget_class->map = gtk_moz_window_map;
  widget_class->realize = gtk_moz_window_realize;
  widget_class->size_request = gtk_moz_window_size_request;
  widget_class->size_allocate = gtk_moz_window_size_allocate;
  widget_class->draw = gtk_moz_window_draw;
  widget_class->expose_event = gtk_moz_window_expose;

  container_class->add = gtk_moz_window_add;
  container_class->remove = gtk_moz_window_remove;
  container_class->forall = gtk_moz_window_forall;
  container_class->child_type = gtk_moz_window_child_type;
}

static GtkType
gtk_moz_window_child_type (GtkContainer *container) {
  return GTK_TYPE_WIDGET;
}

static void
gtk_moz_window_init (GtkMozWindow *moz_window) {
  GTK_WIDGET_UNSET_FLAGS (moz_window, GTK_NO_WINDOW);
  moz_window->children = NULL;
}

GtkWidget *
gtk_moz_window_new (void) {
  GtkMozWindow *moz_window;
  g_print("gtk_moz_window_new()\n");
  
  moz_window = gtk_type_new (GTK_TYPE_MOZ_WINDOW);
  gtk_moz_window_set_adjustments(moz_window, FALSE, FALSE);
  return GTK_WIDGET(moz_window);
}

void
gtk_moz_window_put (GtkMozWindow *moz_window,
		    GtkWidget    *widget,
		    gint16        x,
		    gint16        y) {
  GtkMozWindowChild *child_info;
  g_print("gtk_moz_window_put()\n");
  
  g_return_if_fail (moz_window != NULL);
  g_return_if_fail (GTK_IS_MOZ_WINDOW(moz_window));
  g_return_if_fail (widget != NULL);

  child_info = g_new (GtkMozWindowChild, 1);
  child_info->widget = widget;
  child_info->x = x;
  child_info->y = y;

  gtk_widget_set_parent(widget, GTK_WIDGET(moz_window));
  moz_window->children = g_list_append(moz_window->children, child_info);

  if (GTK_WIDGET_REALIZED(moz_window) && !GTK_WIDGET_REALIZED (widget))
    gtk_widget_realize(widget);

  /* XXX added the check to see if we were visible. */
  if (GTK_WIDGET_MAPPED(moz_window) && !GTK_WIDGET_MAPPED(widget) &&
      GTK_WIDGET_VISIBLE(widget))
    gtk_widget_map(widget);
  
  if (GTK_WIDGET_VISIBLE(widget) && GTK_WIDGET_VISIBLE(moz_window))
    gtk_widget_queue_resize (GTK_WIDGET(moz_window));
}

void
gtk_moz_window_move (GtkMozWindow *moz_window,
		     GtkWidget    *widget,
		     gint16        x,
		     gint16        y) {
  GtkMozWindowChild *child;
  GList *children;
  g_print("gtk_moz_window_move()\n");

  g_return_if_fail (moz_window != NULL);
  g_return_if_fail (GTK_IS_MOZ_WINDOW(moz_window));
  g_return_if_fail (widget != NULL);

  children = moz_window->children;
  while (children) {
    child = children->data;
    children = children->next;

    if (child->widget == widget) {
      child->x = x;
      child->y = y;

      if (GTK_WIDGET_VISIBLE(widget) && GTK_WIDGET_VISIBLE(moz_window))
	gtk_widget_queue_resize(GTK_WIDGET(moz_window));
      break;
    }
  }
}

static void
gtk_moz_window_map (GtkWidget *widget) {
  GtkMozWindow *moz_window;
  GtkMozWindowChild *child;
  GList *children;

  g_print("gtk_moz_window_map()\n");

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MOZ_WINDOW(widget));

  GTK_WIDGET_SET_FLAGS(widget, GTK_MAPPED);
  moz_window = GTK_MOZ_WINDOW(widget);

  children = moz_window->children;
  while (children) {
    child = children->data;
    children = children->next;
    if (GTK_WIDGET_VISIBLE(child->widget) &&
	!GTK_WIDGET_MAPPED(child->widget))
      gtk_widget_map(child->widget);
  }
  gdk_window_show(widget->window);
}

static void
gtk_moz_window_realize (GtkWidget *widget) {
  GdkWindowAttr attributes;
  gint attributes_mask;

  g_print("gtk_moz_window_realize()\n");
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MOZ_WINDOW(widget));
  
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual(widget);
  attributes.colormap = gtk_widget_get_colormap(widget);
  attributes.event_mask = gtk_widget_get_events(widget);
  attributes.event_mask |= GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK;

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

  widget->window = gdk_window_new(gtk_widget_get_parent_window(widget), &attributes,
				  attributes_mask);
  gdk_window_set_user_data(widget->window, widget);

  widget->style = gtk_style_attach (widget->style, widget->window);
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);

}

static void
gtk_moz_window_size_request (GtkWidget      *widget,
			     GtkRequisition *requisition) {
  GtkMozWindow *moz_window;
  GtkMozWindowChild *child;
  GList *children;
  GtkRequisition child_requisition;

  g_print("gtk_moz_window_size_request()\n");
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MOZ_WINDOW(widget));
  g_return_if_fail (requisition != NULL);

  moz_window = GTK_MOZ_WINDOW (widget);
  requisition->width = 0;
  requisition->height = 0;
  
  children = moz_window->children;
  while (children) {
    child = children->data;
    children = children->next;

    if (GTK_WIDGET_VISIBLE(child->widget)) {
      gtk_widget_size_request(child->widget, &child_requisition);
      requisition->height = MAX(requisition->height,
				child->y +
				child_requisition.height);
      requisition->width = MAX(requisition->width,
			       child->x + 
			       child_requisition.width);
    }
  }
  requisition->height += GTK_CONTAINER(moz_window)->border_width * 2;
  requisition->width += GTK_CONTAINER(moz_window)->border_width * 2;
}

static void
gtk_moz_window_size_allocate (GtkWidget *widget,
			      GtkAllocation *allocation) {
  GtkMozWindow *moz_window;
  GtkMozWindowChild *child;
  GtkAllocation child_allocation;
  GtkRequisition child_requisition;
  GList *children;
  guint16 border_width;

  g_print("gtk_moz_window_size_allocate()\n");

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MOZ_WINDOW(widget));
  g_return_if_fail (allocation != NULL);

  moz_window = GTK_MOZ_WINDOW(widget);
  
  widget->allocation = *allocation;
  if (GTK_WIDGET_REALIZED(widget))
    gdk_window_move_resize (widget->window,
			    allocation->x,
			    allocation->y,
			    allocation->width,
			    allocation->height);

  border_width = GTK_CONTAINER(moz_window)->border_width;

  children = moz_window->children;
  while (children) {
    child = children->data;
    children = children->next;

    if (GTK_WIDGET_VISIBLE(child->widget)) {
      gtk_widget_get_child_requisition(child->widget, &child_requisition);
      child_allocation.x = child->x + border_width;
      child_allocation.y = child->y + border_width;
      child_allocation.width = child_requisition.width;
      child_allocation.height = child_requisition.height;
      gtk_widget_size_allocate (child->widget, &child_allocation);
    }
  }
}

static void
gtk_moz_window_paint (GtkWidget    *widget,
		      GdkRectangle *area) {
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MOZ_WINDOW(widget));
  g_return_if_fail (area != NULL);
  /* don't do this. */
  /*  g_print("gtk_moz_window_paint()\n"); */
#if 0
  if (GTK_WIDGET_DRAWABLE (widget))
    gdk_window_clear_area (widget->window,
			   area->x, area->y,
			   area->width, area->height);
#endif
}

static void
gtk_moz_window_draw (GtkWidget    *widget,
		     GdkRectangle *area) {
  GtkMozWindow *moz_window;
  GtkMozWindowChild *child;
  GdkRectangle child_area;
  GList *children;

  /* g_print("gtk_moz_window_draw()\n");*/
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MOZ_WINDOW(widget));

  if (GTK_WIDGET_DRAWABLE (widget)) {
    moz_window = GTK_MOZ_WINDOW(widget);
    gtk_moz_window_paint(widget, area);

    children = moz_window->children;
    while (children) {
      child = children->data;
      children = children->next;

      if (gtk_widget_intersect (child->widget, area, &child_area))
	gtk_widget_draw (child->widget, &child_area);
    }
  }
}

static gint
gtk_moz_window_expose (GtkWidget *widget,
		       GdkEventExpose *event)
{
  GtkMozWindow *moz_window;
  GtkMozWindowChild *child;
  GdkEventExpose child_event;
  GList *children;

  g_print("gtk_moz_window_expose()\n");
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_MOZ_WINDOW(widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (GTK_WIDGET_DRAWABLE (widget)) {
    moz_window = GTK_MOZ_WINDOW(widget);
    child_event = *event;
    children = moz_window->children;
    while (children) {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_NO_WINDOW(child->widget) &&
	  gtk_widget_intersect (child->widget, &event->area,
				&child_event.area))
	gtk_widget_event (child->widget, (GdkEvent *)&child_event);
    }
  }
  return FALSE;
}

static void
gtk_moz_window_add (GtkContainer *container,
		    GtkWidget    *widget) {
  g_return_if_fail(container != NULL);
  g_return_if_fail(GTK_IS_MOZ_WINDOW(container));
  g_return_if_fail(widget != NULL);

  g_print("gtk_moz_window_add()\n");
  gtk_moz_window_put (GTK_MOZ_WINDOW(container), widget, 0, 0);
}

static void
gtk_moz_window_remove (GtkContainer *container,
		       GtkWidget    *widget) {
  GtkMozWindow *moz_window;
  GtkMozWindowChild *child;
  GList *children;

  g_print("gtk_moz_window_remove()\n");
  g_return_if_fail (container != NULL);
  g_return_if_fail (GTK_IS_MOZ_WINDOW (container));
  g_return_if_fail (widget != NULL);

  moz_window = GTK_MOZ_WINDOW(container);
  
  children = moz_window->children;
  while (children) {
    child = children->data;
    if (child->widget == widget) {
      gboolean was_visible = GTK_WIDGET_VISIBLE(widget);
      gtk_widget_unparent (widget);
      moz_window->children = g_list_remove_link(moz_window->children,
						children);
      g_list_free(children);
      g_free(child);
      if (was_visible && GTK_WIDGET_VISIBLE(container))
	gtk_widget_queue_resize(GTK_WIDGET(container));
      break;
    }
    children = children->next;
  }
}

static void
gtk_moz_window_forall (GtkContainer *container,
		       gboolean      include_internals,
		       GtkCallback   callback,
		       gpointer      callback_data) {
  GtkMozWindow *moz_window;
  GtkMozWindowChild *child;
  GList *children;

  g_print("gtk_moz_window_forall()\n");
  g_return_if_fail (container != NULL);
  g_return_if_fail (GTK_IS_MOZ_WINDOW(container));
  g_return_if_fail (callback != NULL);

  moz_window = GTK_MOZ_WINDOW(container);
  
  children = moz_window->children;
  while (children) {
    child = children->data;
    children = children->next;

    (*callback)(child->widget, callback_data);
  }
}

GtkAdjustment *
gtk_moz_window_get_hadjustment(GtkMozWindow *moz_window) {
  g_return_val_if_fail(moz_window != NULL, NULL);
  g_return_val_if_fail(GTK_IS_MOZ_WINDOW(moz_window), NULL);
  return moz_window->hadjustment;
}

GtkAdjustment *
gtk_moz_window_get_vadjustment(GtkMozWindow *moz_window) {
  g_return_val_if_fail(moz_window != NULL, NULL);
  g_return_val_if_fail(GTK_IS_MOZ_WINDOW(moz_window), NULL);
  return moz_window->vadjustment;
}

static void
gtk_moz_window_set_adjustments(GtkMozWindow *moz_window,
			       GtkAdjustment *hadj,
			       GtkAdjustment *vadj) {
  gboolean need_adjust = FALSE;

  g_print("gtk_moz_window_set_adjustments()\n");
  g_return_if_fail (moz_window != NULL);
  g_return_if_fail (GTK_IS_MOZ_WINDOW(moz_window));
  
  if (hadj)
    g_return_if_fail(GTK_IS_ADJUSTMENT(hadj));
  else
    hadj = GTK_ADJUSTMENT (gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
  if (vadj)
    g_return_if_fail(GTK_IS_ADJUSTMENT(vadj));
  else
    vadj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0));

  if (moz_window->hadjustment && (moz_window->hadjustment != hadj)) {
    gtk_signal_disconnect_by_data (GTK_OBJECT(moz_window->hadjustment), moz_window);
    gtk_object_unref(GTK_OBJECT(moz_window->hadjustment));
  }

  if (moz_window->vadjustment && (moz_window->vadjustment != vadj)) {
    gtk_signal_disconnect_by_data(GTK_OBJECT(moz_window->vadjustment), moz_window);
    gtk_object_unref(GTK_OBJECT(moz_window->vadjustment));
  }

  if (moz_window->hadjustment != hadj) {
    moz_window->hadjustment = hadj;
    gtk_object_ref(GTK_OBJECT(moz_window->hadjustment));
    gtk_object_sink(GTK_OBJECT(moz_window->hadjustment));
    gtk_signal_connect(GTK_OBJECT(moz_window->hadjustment), "value_changed",
		       (GtkSignalFunc) gtk_moz_window_adjustment_changed,
		       moz_window);
    need_adjust = TRUE;
  }

  if (moz_window->vadjustment != vadj) {
    moz_window->vadjustment = vadj;
    gtk_object_ref(GTK_OBJECT(moz_window->vadjustment));
    gtk_object_sink(GTK_OBJECT(moz_window->vadjustment));
    gtk_signal_connect(GTK_OBJECT(moz_window->vadjustment), "value_changed",
		       (GtkSignalFunc)gtk_moz_window_adjustment_changed,
		       moz_window);
    need_adjust = TRUE;
  }

  if (need_adjust)
    gtk_moz_window_adjustment_changed(NULL, moz_window);
}

static void
gtk_moz_window_adjustment_changed (GtkAdjustment *adjustment,
				   GtkMozWindow *moz_window)
{
  GtkWidget *widget;
  gint dx, dy;

  g_print("gtk_moz_window_adjustment_changed()\n");
  widget = GTK_WIDGET(moz_window);
  dx = (gint)moz_window->hadjustment->value - moz_window->xoffset;
  dy = (gint)moz_window->vadjustment->value - moz_window->yoffset;
  
  moz_window->xoffset = (gint)moz_window->hadjustment->value;
  moz_window->yoffset = (gint)moz_window->vadjustment->value;

  /* XXX do we need to impl freezing here? */

  if (!GTK_WIDGET_MAPPED(moz_window)) {
    /* position children? */
    return;
  }
  /* adjust allocations? */

  if (dx > 0) {
    /* resize and move bin window? */
    gtk_moz_window_expose_area(moz_window,
			       MAX((gint)widget->allocation.width - dx, 0),
			       0,
			       MIN(dx, widget->allocation.width),
			       widget->allocation.height);
			     
  }
  else if (dx < 0) {
    /* resize and move bin window? */
    gtk_moz_window_expose_area(moz_window,
			       0, 0,
			       MIN(-dx, widget->allocation.width),
			       widget->allocation.height);
  }
  if (dy > 0) {
    /* err, yeah.  see above */
    gtk_moz_window_expose_area(moz_window,
			       0, 
			       MAX((gint)widget->allocation.height - dy, 0),
			       widget->allocation.width,
			       MIN(dy, widget->allocation.height));
  }
  else if (dy < 0) {
   gtk_moz_window_expose_area(moz_window,
			      0, 0,
			      widget->allocation.width,
			      MIN(-dy, (gint)widget->allocation.height));
  }
  /* position children? */
  gdk_flush();

}

static void gtk_moz_window_expose_area        (GtkMozWindow  *moz_window,
					       gint            x, 
					       gint            y, 
					       gint            width, 
					       gint            height) {
  GdkEventExpose event;

  g_print("gtk_moz_window_expose_area()\n");
  
  event.type = GDK_EXPOSE;
  event.send_event = TRUE;
  event.window = GTK_WIDGET(moz_window)->window;
  event.count = 0;
  
  event.area.x = x;
  event.area.y = y;
  event.area.width = width;
  event.area.height = height;
  
  gdk_window_ref (event.window);
  gtk_widget_event (GTK_WIDGET (moz_window), (GdkEvent *)&event);
  gdk_window_unref (event.window);
}
