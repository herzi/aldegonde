/* GStreamer Media Player
 * (c) 2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * timer.h: time slider and label.
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

#ifndef __TIMER_H__
#define __TIMER_H__

#include <glib.h>
#include <gst/gst.h>
#include <gdk/gdk.h>
#include <gtk/gtkhbox.h>

G_BEGIN_DECLS

#define GST_PLAYER_TYPE_TIMER \
  (gst_player_timer_get_type ())
#define GST_PLAYER_TIMER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_PLAYER_TYPE_TIMER, GstPlayerTimer))
#define GST_PLAYER_TIMER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GST_PLAYER_TYPE_TIMER, GstPlayerTimerClass))
#define GST_PLAYER_IS_TIMER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_PLAYER_TYPE_TIMER))
#define GST_PLAYER_IS_TIMER_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_PLAYER_TYPE_TIMER))

typedef struct _GstPlayerTimer {
  GtkVBox parent;

  GstElement *play;

  GtkLabel *label;
  GtkRange *range;
  gboolean lock, seeking;

  guint64 len, pos;
} GstPlayerTimer;

typedef struct _GstPlayerTimerClass {
  GtkVBoxClass klass;
} GstPlayerTimerClass;

GType		gst_player_timer_get_type	(void);
GtkWidget *	gst_player_timer_new		(GstElement *play);
void		gst_player_timer_progress	(GstPlayerTimer *timer);

G_END_DECLS

#endif /* __TIME_H__ */
