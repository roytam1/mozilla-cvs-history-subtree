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

#include "gtkmozarea.h"
#include <X11/Xlib.h>

static void gtk_mozarea_class_init (GtkMozAreaClass *klass);
static void gtk_mozarea_init       (GtkMozArea      *mozarea);
static void gtk_mozarea_realize    (GtkWidget       *widget);
static void gtk_mozarea_size_allocate (GtkWidget    *widget, GtkAllocation *allocation);        

GtkType
gtk_mozarea_get_type (void)
{
  static GtkType mozarea_type = 0;

  if (!mozarea_type)
    {
      static const GtkTypeInfo mozarea_info =
      {
	"GtkMozArea",
	sizeof (GtkMozArea),
	sizeof (GtkMozAreaClass),
	(GtkClassInitFunc) gtk_mozarea_class_init,
	(GtkObjectInitFunc) gtk_mozarea_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      mozarea_type = gtk_type_unique (GTK_TYPE_WIDGET, &mozarea_info);
    }

  return mozarea_type;
}

static void
gtk_mozarea_class_init (GtkMozAreaClass *klass)
{
  GtkWidgetClass *widget_class;

  widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->realize = gtk_mozarea_realize;
  widget_class->size_allocate = gtk_mozarea_size_allocate;
}

static void
gtk_mozarea_init (GtkMozArea *mozarea)
{
  mozarea->superwin = NULL;
}

static void 
gtk_mozarea_realize (GtkWidget *widget)
{
  GtkMozArea *mozarea;
  
  g_return_if_fail (GTK_IS_MOZAREA (widget));

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  mozarea = GTK_MOZAREA (widget);

  mozarea->superwin = gdk_superwin_new (gtk_widget_get_parent_window (widget),
					widget->allocation.x, widget->allocation.y,
					widget->allocation.width, widget->allocation.height);
  widget->window = mozarea->superwin->shell_window;
}

static void 
gtk_mozarea_size_allocate (GtkWidget    *widget,
			   GtkAllocation *allocation)
{
  GtkMozArea *mozarea;
  
  g_return_if_fail (GTK_IS_MOZAREA (widget));

  mozarea = GTK_MOZAREA (widget);
  
  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_move (mozarea->superwin->shell_window,
		       allocation->x, allocation->y);
      gdk_superwin_resize (mozarea->superwin,
			   allocation->width, allocation->height);
    }
}

GtkWidget*
gtk_mozarea_new (GdkWindow *parent_window)
{
  return GTK_WIDGET (gtk_type_new (GTK_TYPE_MOZAREA));
}
