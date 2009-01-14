/* GStreamer Media Player
 * (c) 2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * video.c: video widget.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gdk/gdkx.h>
#include <gnome.h>
#include <gst/interfaces/xoverlay.h>

#include "video.h"

static void	gst_player_video_class_init	(GstPlayerVideoClass *klass);
static void	gst_player_video_init		(GstPlayerVideo *video);
static void	gst_player_video_dispose	(GObject        *object);
static void	gst_player_video_realize	(GtkWidget      *widget);
static void	gst_player_video_size_request	(GtkWidget      *widget,
						 GtkRequisition *req);
static void	cb_preferred_video_size		(GtkWidget      *widget,
						 GtkRequisition *req,
						 gpointer        data);
static void	gst_player_video_size_allocate	(GtkWidget      *widget,
						 GtkAllocation  *alloc);
static gboolean	gst_player_video_expose		(GtkWidget      *widget,
						 GdkEventExpose *event);
static void	gst_player_video_draw		(GstPlayerVideo *video,
						 gboolean        on);

static void	cb_state_change			(GstElement     *element,
						 GstState        old_state,
						 GstState        new_state,
						 gpointer        data);

static GtkWidgetClass *parent_class = NULL;

GType
gst_player_video_get_type (void)
{
  static GType gst_player_video_type = 0;

  if (!gst_player_video_type) {
    static const GTypeInfo gst_player_video_info = {
      sizeof (GstPlayerVideoClass),
      NULL,
      NULL,
      (GClassInitFunc) gst_player_video_class_init,
      NULL,
      NULL,
      sizeof (GstPlayerVideo),
      0,
      (GInstanceInitFunc) gst_player_video_init,
      NULL
    };

    gst_player_video_type =
	g_type_register_static (GTK_TYPE_WIDGET, 
				"GstPlayerVideo",
				&gst_player_video_info, 0);
  }

  return gst_player_video_type;
}

static void
gst_player_video_class_init (GstPlayerVideoClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  gchar *filename;

  parent_class = g_type_class_ref (GTK_TYPE_WIDGET);

  gobject_class->dispose = gst_player_video_dispose;

  widget_class->realize = gst_player_video_realize;
  widget_class->size_allocate = gst_player_video_size_allocate;
  widget_class->size_request = gst_player_video_size_request;
  widget_class->expose_event = gst_player_video_expose;

  /* icon */
  filename = gnome_program_locate_file (NULL,
      GNOME_FILE_DOMAIN_APP_PIXMAP, "logo.png", TRUE, NULL);
  if (filename) {
    klass->logo = gdk_pixbuf_new_from_file (filename, NULL);
    g_free (filename);
  }
}

static void
gst_player_video_init (GstPlayerVideo *video)
{
  GstPlayerVideoClass *klass = GST_PLAYER_VIDEO_GET_CLASS (video);

  video->element = NULL;
  video->id = 0;
  video->width = gdk_pixbuf_get_width (klass->logo);
  video->height = gdk_pixbuf_get_height (klass->logo);

  video->id2 = g_signal_connect (video, "size-request",
      G_CALLBACK (cb_preferred_video_size), NULL);
}

GtkWidget *
gst_player_video_new (GstElement *element,
		      GstElement *play)
{
  GstPlayerVideo *video =
      g_object_new (GST_PLAYER_TYPE_VIDEO, NULL);

  if (element && play) {
    if (GST_IS_BIN (element)) {
      video->element = gst_bin_get_by_interface (GST_BIN (element),
						 GST_TYPE_X_OVERLAY);
    } else {
      video->element = element;
    }
    video->play = play;

    gst_object_ref (GST_OBJECT (video->element));
    gst_object_ref (GST_OBJECT (video->play));
    video->id = g_signal_connect (play, "state-change",
        G_CALLBACK (cb_state_change), video);
  }
  
  return GTK_WIDGET (video);
}

static void
gst_player_video_dispose (GObject *object)
{
  GstPlayerVideo *video = GST_PLAYER_VIDEO (object);

  if (video->id != 0) {
    g_signal_handler_disconnect (video->play, video->id);
    video->id = 0;
  }

  if (video->element) {
    /* shut down overlay... */
    gst_player_video_draw (video, FALSE);

    /* ...and be happy */
    gst_object_unref (GST_OBJECT (video->element));
    video->element = NULL;
  }

  if (video->play) {
    gst_object_unref (GST_OBJECT (video->play));
    video->play = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_player_video_realize (GtkWidget *widget)
{
  GstPlayerVideo *video = GST_PLAYER_VIDEO (widget);
  GdkWindowAttr attributes;
  gint attributes_mask;
  gfloat ratio;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GST_PLAYER_IS_VIDEO (widget));

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = gtk_widget_get_events (widget) | 
                            GDK_EXPOSURE_MASK |
			    GDK_BUTTON_PRESS_MASK | 
                            GDK_BUTTON_RELEASE_MASK |
			    GDK_POINTER_MOTION_MASK |
                            GDK_POINTER_MOTION_HINT_MASK;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);

  attributes_mask = GDK_WA_X | GDK_WA_Y |
		      GDK_WA_VISUAL | GDK_WA_COLORMAP;
  video->full_window = gdk_window_new (widget->parent->window,
				       &attributes, attributes_mask);
  gdk_window_show (video->full_window);

  /* specific for video only */
  attributes.x = 0;
  attributes.y = 0;
  attributes.width = video->width;
  attributes.height = video->height;
  if ((gfloat) video->width / widget->allocation.width >
      (gfloat) video->height / widget->allocation.height) {
    ratio = (gfloat) video->height / widget->allocation.height;
  } else {
    ratio = (gfloat) video->width / widget->allocation.width;
  }
  attributes.width *= ratio;
  attributes.height *= ratio;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = gtk_widget_get_events (widget) | 
                            GDK_EXPOSURE_MASK |
			    GDK_BUTTON_PRESS_MASK | 
                            GDK_BUTTON_RELEASE_MASK |
			    GDK_POINTER_MOTION_MASK |
                            GDK_POINTER_MOTION_HINT_MASK;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);

  attributes_mask = GDK_WA_X | GDK_WA_Y |
	GDK_WA_VISUAL | GDK_WA_COLORMAP;
  video->video_window = gdk_window_new (video->full_window,
					&attributes, attributes_mask);
  gdk_window_show (video->video_window);

  widget->window = video->video_window;

  widget->style = gtk_style_attach (widget->style, widget->window);

  gdk_window_set_user_data (widget->window, widget);

  gtk_style_set_background (widget->style, widget->window, GTK_STATE_ACTIVE);
}

/*
 * The hack below takes care that the widget has a minimal size of
 * 240x180, but a recommended (default) size of the playing movie
 * or the logo being shown.
 */

static void
gst_player_video_size_request (GtkWidget      *widget,
			       GtkRequisition *requisition)
{
  requisition->width = 240;
  requisition->height = 180;
}

static gboolean
idle_unset_size (gpointer data)
{
  GstPlayerVideo *video = GST_PLAYER_VIDEO (data);

  if (video->id2) {
    g_signal_handler_disconnect (video, video->id2);
    video->id2 = 0;
  }
  gtk_widget_queue_resize_no_redraw (GTK_WIDGET (data));

  /* once */
  return FALSE;
}

static void
cb_preferred_video_size (GtkWidget      *widget,
			 GtkRequisition *requisition,
			 gpointer        data)
{
  GstPlayerVideo *video = GST_PLAYER_VIDEO (widget);

  /* recommended */
  requisition->width = video->width;
  requisition->height = video->height;

  g_idle_add (idle_unset_size, video);
}

static void
gst_player_video_size_allocate (GtkWidget     *widget,
				GtkAllocation *allocation)
{
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GST_PLAYER_IS_VIDEO(widget));
  g_return_if_fail (allocation != NULL);

  widget->allocation = *allocation;

  if (GTK_WIDGET_REALIZED (widget)) {
    GstPlayerVideo *video = GST_PLAYER_VIDEO (widget);
    gfloat ratio, width = video->width, height = video->height;

    gdk_window_move_resize (video->full_window,
                            allocation->x, allocation->y,
                            allocation->width, allocation->height);
    if ((gfloat) widget->allocation.width / video->width >
        (gfloat) widget->allocation.height / video->height) {
      ratio = (gfloat) widget->allocation.height / video->height;
    } else {
      ratio = (gfloat) widget->allocation.width / video->width;
    }
    width *= ratio;
    height *= ratio;
    gdk_window_move_resize (video->video_window,
                            (allocation->width - width) / 2,
			    (allocation->height - height) / 2,
                            width, height);
  }
}

static void
gst_player_video_draw (GstPlayerVideo *video,
		       gboolean        on)
{
  XID window;
  GstXOverlay *overlay;
  GtkWidget *widget = GTK_WIDGET (video);

  if (!GST_IS_X_OVERLAY (video->element))
    return;
  overlay = GST_X_OVERLAY (video->element);

  if (on) {
    window = GDK_WINDOW_XWINDOW (video->video_window);
  } else {
    window = 0;
  }

  gst_x_overlay_set_xwindow_id (overlay, window);

  if (on) {
    gdk_draw_rectangle (video->full_window, widget->style->black_gc,
        TRUE, 0, 0, widget->allocation.width, widget->allocation.height);
    if (GST_STATE (video->element) >= GST_STATE_PAUSED) {
      gst_x_overlay_expose (overlay);
    } else {
      GstPlayerVideoClass *klass = GST_PLAYER_VIDEO_GET_CLASS (video);
      GdkPixbuf *logo;
      gfloat width = video->width, height = video->height;
      gfloat ratio;

      if ((gfloat) widget->allocation.width / video->width >
          (gfloat) widget->allocation.height / video->height) {
        ratio = (gfloat) widget->allocation.height / video->height;
      } else {
        ratio = (gfloat) widget->allocation.width / video->width;
      }
      width *= ratio;
      height *= ratio;

      logo = gdk_pixbuf_scale_simple (klass->logo,
          width, height, GDK_INTERP_BILINEAR);

      gdk_draw_pixbuf (GDK_DRAWABLE (video->video_window),
          widget->style->fg_gc[0], logo, 0, 0, 0, 0,
          width, height, GDK_RGB_DITHER_NONE, 0, 0);

      gdk_pixbuf_unref (logo);
    }
  }
}

static gboolean
gst_player_video_expose (GtkWidget      *widget,
			 GdkEventExpose *event)
{
  GstPlayerVideo *video;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GST_PLAYER_IS_VIDEO (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (event->count > 0)
    return TRUE;

  video = GST_PLAYER_VIDEO (widget);
 
  gst_player_video_draw (video, TRUE);

  return TRUE;
}

/*
 * Is called when we load a new video or when it's done playing.
 */

static gboolean
idle_desired_size (gpointer data)
{
  GstPlayerVideo *video = GST_PLAYER_VIDEO (data);

  if (video->id2) {
    g_signal_handler_disconnect (video, video->id2);
  }
  video->id2 = g_signal_connect (video, "size-request",
      G_CALLBACK (cb_preferred_video_size), NULL);
  gtk_widget_queue_resize (GTK_WIDGET (data));
  g_object_unref (G_OBJECT (data));

  /* once */
  return FALSE;
}

static void
cb_caps_set (GObject    *obj,
	     GParamSpec *pspec,
	     gpointer    data)
{
  GstPlayerVideo *video = GST_PLAYER_VIDEO (data);
  GstPad *pad = GST_PAD (obj);
  GstStructure *s;

  if (!GST_PAD_CAPS (pad))
    return;

  s = gst_caps_get_structure (GST_PAD_CAPS (pad), 0);
  if (s) {
    const GValue *par;

    gst_structure_get_int (s, "width", &video->width);
    gst_structure_get_int (s, "height", &video->height);
    if ((par = gst_structure_get_value (s,
                   "pixel-aspect-ratio"))) {
      gint num = gst_value_get_fraction_numerator (par),
          den = gst_value_get_fraction_denominator (par);

      if (num > den)
        video->width *= (gfloat) num / den;
      else
        video->height *= (gfloat) den / num;
    }
  }
                                                                                
  /* and disable ourselves */
  g_signal_handlers_disconnect_by_func (pad, cb_caps_set, data);

  /* resize */
  g_object_ref (G_OBJECT (video));
  g_idle_add (idle_desired_size, video);
}

static void
cb_state_change (GstElement*element,
		 GstState   old_state,
		 GstState   new_state,
		 gpointer   data)
{
  GstPlayerVideo *video = GST_PLAYER_VIDEO (data);

  /* FIXME: on paused->ready, show logo, otherwise
   * check streaminfo if we have video, if so check
   * if we have caps and else set a notify::caps
   * handler. Resize window after that. */
  if ((old_state >= GST_STATE_PAUSED &&
       new_state <= GST_STATE_READY)) {
    GstPlayerVideoClass *klass = GST_PLAYER_VIDEO_GET_CLASS (video);

    video->width = gdk_pixbuf_get_width (klass->logo);
    video->height = gdk_pixbuf_get_height (klass->logo);

    g_object_ref (G_OBJECT (video));
    g_idle_add (idle_desired_size, video);
  } else if ((new_state >= GST_STATE_PAUSED &&
	      old_state <= GST_STATE_READY)) {
    const GList *sinfo = NULL;

    g_object_get (G_OBJECT (element), "stream-info", &sinfo, NULL);
    for (; sinfo != NULL; sinfo = sinfo->next) {
      GObject *info = sinfo->data;
      gint type;
      GParamSpec *pspec;
      GEnumValue *val;
      GstPad *pad = NULL;

      g_object_get (info, "type", &type, NULL);
      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (info), "type");
      val = g_enum_get_value (G_PARAM_SPEC_ENUM (pspec)->enum_class, type);
      g_object_get (info, "object", &pad, NULL);

      if (strstr (val->value_name, "VIDEO") && pad && GST_IS_PAD (pad)) {
        if (GST_PAD_CAPS (pad)) {
          cb_caps_set (G_OBJECT (pad), NULL, video);
        } else {
          g_signal_connect (pad, "notify::caps",
              G_CALLBACK (cb_caps_set), video);
        }
      }
    }
  }
}

/*
 * Set to media size.
 */

void
gst_player_video_default_size (GstPlayerVideo *video)
{
  g_object_ref (G_OBJECT (video));

  g_idle_add (idle_desired_size, video);
}
