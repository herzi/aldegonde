/* GStreamer Media Player
 * (c) 2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * video.h: video playback widget.
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

#ifndef __VIDEO_H__
#define __VIDEO_H__

#include <glib.h>
#include <gst/gst.h>
#include <gdk/gdk.h>
#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

#define GST_PLAYER_TYPE_VIDEO \
  (gst_player_video_get_type ())
#define GST_PLAYER_VIDEO(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_PLAYER_TYPE_VIDEO, GstPlayerVideo))
#define GST_PLAYER_VIDEO_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GST_PLAYER_TYPE_VIDEO, GstPlayerVideoClass))
#define GST_PLAYER_IS_VIDEO(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_PLAYER_TYPE_VIDEO))
#define GST_PLAYER_IS_VIDEO_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_PLAYER_TYPE_VIDEO))
#define GST_PLAYER_VIDEO_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_PLAYER_TYPE_VIDEO, GstPlayerVideoClass))

typedef struct _GstPlayerVideo {
  GtkWidget widget;

  /* video overlay element */
  GstElement *element, *play;
  gulong id, id2;
  gint width, height;
  GdkWindow *full_window, *video_window;
} GstPlayerVideo;

typedef struct _GstPlayerVideoClass {
  GtkWidgetClass klass;

  GdkPixbuf *logo;
} GstPlayerVideoClass;

GType		gst_player_video_get_type	(void);
GtkWidget *	gst_player_video_new		(GstElement *element,
						 GstElement *play);
void		gst_player_video_default_size	(GstPlayerVideo *video);

G_END_DECLS

#endif /* __VIDEO_H__ */
