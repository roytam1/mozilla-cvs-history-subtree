#ifndef __GTK_MOZ_WINDOW_H
#define __GTK_MOZ_WINDOW_H

#include <gdk/gdk.h>
#include <gtk/gtkcontainer.h>
#include <gtk/gtkadjustment.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GTK_TYPE_MOZ_WINDOW             (gtk_moz_window_get_type())
#define GTK_MOZ_WINDOW(obj)             (GTK_CHECK_CAST((obj), GTK_TYPE_MOZ_WINDOW, GtkMozWindow))
#define GTK_MOZ_WINDOW_CLASS(klass)     (GTK_CHECK_CLASS_CAST((klass), GTK_TYPE_MOZ_WINDOW, GtkMozWindowClass))
#define GTK_IS_MOZ_WINDOW(obj)          (GTK_CHECK_TYPE((obj), GTK_TYPE_MOZ_WINDOW))
#define GTK_IS_MOZ_WINDOW_CLASS(klass)  (GTK_CHECK_CLASS_TYPE((klass), GTK_TYPE_MOZ_WINDOW))

typedef struct _GtkMozWindow       GtkMozWindow;
typedef struct _GtkMozWindowClass  GtkMozWindowClass;
typedef struct _GtkMozWindowChild  GtkMozWindowChild;

struct _GtkMozWindow {
  GtkContainer  container;
  GList         *children;
  GtkAdjustment *hadjustment;
  GtkAdjustment *vadjustment;
  gint           xoffset;
  gint           yoffset;
};

struct _GtkMozWindowClass {
  GtkContainerClass parent_class;
  void (*set_scroll_adjustments)  (GtkMozWindow  *moz_window,
				   GtkAdjustment *hadjustment,
				   GtkAdjustment *vadjustment);
};

struct _GtkMozWindowChild {
  GtkWidget *widget;
  gint16 x;
  gint16 y;
};

GtkType        gtk_moz_window_get_type        (void);
GtkWidget     *gtk_moz_window_new             (void);
void           gtk_moz_window_put             (GtkMozWindow     *moz_window,
					       GtkWidget        *widget,
					       gint16            x,
					       gint16            y);
void           gtk_moz_window_move            (GtkMozWindow     *moz_window,
					       GtkWidget        *widget,
					       gint16            x,
					       gint16            y);
GtkAdjustment *gtk_moz_window_get_hadjustment (GtkMozWindow *moz_window);
GtkAdjustment *gtk_moz_window_get_vadjustment (GtkMozWindow *moz_window);
void           gtk_moz_window_set_hadjustment (GtkMozWindow  *moz_window,
					       GtkAdjustment *adjustment);
void           gtk_moz_window_set_vadjustment (GtkMozWindow  *moz_window,
					       GtkAdjustment *adjustment);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GTK_MOZ_WINDOW_H */






