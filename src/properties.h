/* GStreamer Media Player
 * (c) 2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * properties.h: tags and streaminfo display.
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

#ifndef __PROPERTIES_H__
#define __PROPERTIES_H__

#include <glib.h>
#include <gst/gst.h>
#include <gdk/gdk.h>
#include <gtk/gtkdialog.h>

G_BEGIN_DECLS

#define GST_PLAYER_TYPE_PROPERTIES \
  (gst_player_properties_get_type())
#define GST_PLAYER_PROPERTIES(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_PLAYER_TYPE_PROPERTIES, GstPlayerProperties))
#define GST_PLAYER_PROPERTIES_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_PLAYER_TYPE_PROPERTIES, GstPlayerPropertiesClass))
#define GST_PLAYER_IS_PROPERTIES(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_PLAYER_TYPE_PROPERTIES))
#define GST_PLAYER_IS_PROPERTIES_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_PLAYER_TYPE_PROPERTIES))

typedef struct _GstPlayerProperties {
  GtkDialog parent;

  GtkWidget *content;
} GstPlayerProperties;

typedef struct _GstPlayerPropertiesClass {
  GtkDialogClass klass;
} GstPlayerPropertiesClass;

GType		gst_player_properties_get_type	(void);
GtkWidget *	gst_player_properties_new	(void);
void		gst_player_properties_update	(GstPlayerProperties *props,
						 GstElement *play,
						 const GstTagList *taglist);

G_END_DECLS

#endif /* __PROPERTIES_H__ */
