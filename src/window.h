/* GStreamer Media Player
 * (c) 2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * window.h: main window.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __WINDOW_H__
#define __WINDOW_H__

#include <glib.h>
#include <gst/gst.h>
#include <gdk/gdk.h>
#include <gtk/gtkwidget.h>

#include "timer.h"

G_BEGIN_DECLS

#define GST_PLAYER_TYPE_WINDOW \
  (gst_player_window_get_type())
#define GST_PLAYER_WINDOW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_PLAYER_TYPE_WINDOW, GstPlayerWindow))
#define GST_PLAYER_WINDOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_PLAYER_TYPE_WINDOW, GstPlayerWindowClass))
#define GST_PLAYER_IS_WINDOW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_PLAYER_TYPE_WINDOW))
#define GST_PLAYER_IS_WINDOW_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_PLAYER_TYPE_WINDOW))

typedef struct _GstPlayerWindow {
  GnomeApp parent;

  GstElement *play;
  GstPlayerTimer *timer;
  GtkWidget *video;
  guint idle_id;

  /* tagging and streaminfo */
  GtkWidget *props;
  GstTagList *tagcache;

  gboolean fullscreen;
} GstPlayerWindow;

typedef struct _GstPlayerWindowClass {
  GnomeAppClass klass;
} GstPlayerWindowClass;

GType		gst_player_window_get_type	(void);
GtkWidget *	gst_player_window_new		(GError **err);

void            gst_player_window_play          (GstPlayerWindow* self,
                                                 gchar const    * uri);

G_END_DECLS

#endif /* __WINDOW_H__ */
