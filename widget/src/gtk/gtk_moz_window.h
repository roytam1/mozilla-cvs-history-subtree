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

#ifndef __GTK_MOZ_WINDOW_H
#define __GTK_MOZ_WINDOW_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <gdk/gdk.h>
#include <gtk/gtkadjustment.h>
#include <gtk/gtkcontainer.h>

#define GTK_TYPE_MOZ_WINDOW            (gtk_moz_window_get_type())
#define GTK_MOZ_WINDOW(obj)            (GTK_CHECK_CAST((obj), GTK_TYPE_MOZ_WINDOW, GtkMozWindow))
#define GTK_MOZ_WINDOW_CLASS(klass)    (GTK_CHECK_CLASS_CASE((klass), GTK_TYPE_MOZ_WINDOW, GtkMozWindowClass))
#define GTK_IS_MOZ_WINDOW(obj)         (GTK_CHECK_TYPE((obj), GTK_TYPE_MOZ_WINDOW))
#define GTK_IS_MOZ_WINDOW_CLASS(klass) (GTK_CHECK_CLASS_TYPE((klass), GTK_TYPE_MOZ_WINDOW))

typedef struct _GtkMozWindow      GtkMozWindow;
typedef struct _GtkMozWindowClass GtkMozWindowClass;

struct _GtkMozWindow {
  GtkContainer        container;
  GList              *children;
  guint               width;
  guint               height;
  guint               xoffset;
  guint               yoffset;
  GtkAdjustment      *hadjustment;
  GtkAdjustment      *vadjustment;
  GdkWindow          *bin_window;
  GdkVisibilityState  visibility;
  gulong              configure_serial;
  gint                scroll_x;
  gint                scroll_y;
  
};

struct _GtkMozWindowClass {
  GtkContainerClass parent_class;
  void (*set_scroll_adjustments ) (GtkMozWindow  *moz_window,
                                   GtkAdjustment *hadjustment,
                                   GtkAdjustment *vadjustment);
};

GtkType         gtk_moz_window_get_type        (void);
GtkWidget      *gtk_moz_window_new             (GtkAdjustment *hadjustment,
                                                GtkAdjustment *vadjustment);
void            gtk_moz_window_put             (GtkMozWindow *moz_window,
                                                GtkWidget *child_widget,
                                                gint x,
                                                gint y);
void            gtk_moz_window_move            (GtkMozWindow *moz_window,
                                                GtkWidget *child_widget,
                                                gint x,
                                                gint y);
void            gtk_moz_window_set_size        (GtkMozWindow *moz_window,
                                                guint width,
                                                guint height);

GtkAdjustment  *gtk_moz_window_get_hadjustment (GtkMozWindow *moz_window);
GtkAdjustment  *gtk_moz_window_get_vadjustment (GtkMozWindow *moz_window);
void            gtk_moz_window_set_hadjustment (GtkMozWindow *moz_window,
                                                GtkAdjustment *adjustment);
void            gtk_moz_window_set_vadjustment (GtkMozWindow *moz_window,
                                                GtkAdjustment *adjustment);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GTK_MOZ_WINDOW_H */




