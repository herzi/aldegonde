/* GStreamer Media Player
 * (c) 2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * timer.c: time slider and labels.
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

#include "timer.h"

static void	gst_player_timer_class_init	(GstPlayerTimerClass *klass);
static void	gst_player_timer_init		(GstPlayerTimer *timer);
static void	gst_player_timer_dispose	(GObject        *object);
static void	gst_player_timer_size_request	(GtkWidget      *widget,
						 GtkRequisition *req);

static gboolean	cb_button_press			(GtkWidget      *widget,
						 GdkEventButton *event,
						 gpointer        data);
static gboolean	cb_button_release		(GtkWidget      *widget,
						 GdkEventButton *event,
						 gpointer        data);

static void	cb_state			(GstElement     *play,
						 GstState        old_state,
						 GstState        new_state,
						 gpointer        data);

static void	cb_seek				(GtkRange       *range,
						 gpointer        data);

static GtkVBoxClass *parent_class = NULL;

GType
gst_player_timer_get_type (void)
{
  static GType gst_player_timer_type = 0;

  if (!gst_player_timer_type) {
    static const GTypeInfo gst_player_timer_info = {
      sizeof (GstPlayerTimerClass),
      NULL,
      NULL,
      (GClassInitFunc) gst_player_timer_class_init,
      NULL,
      NULL,
      sizeof (GstPlayerTimer),
      0,
      (GInstanceInitFunc) gst_player_timer_init,
      NULL
    };

    gst_player_timer_type =
	g_type_register_static (GTK_TYPE_VBOX, 
				"GstPlayerTimer",
				&gst_player_timer_info, 0);
  }

  return gst_player_timer_type;
}

static void
gst_player_timer_class_init (GstPlayerTimerClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_ref (GTK_TYPE_VBOX);

  gobject_class->dispose = gst_player_timer_dispose;

  widget_class->size_request = gst_player_timer_size_request;
}

static void
gst_player_timer_init (GstPlayerTimer *timer)
{
  GtkWidget *slider, *label;
  GtkBox *box = GTK_BOX (timer);

  timer->play = NULL;
  timer->lock = FALSE;
  timer->seeking = FALSE;
  timer->len = GST_CLOCK_TIME_NONE;
  timer->pos = GST_CLOCK_TIME_NONE;

  /* how-do-I-look stuff */
  gtk_container_set_border_width (GTK_CONTAINER (timer), 6);
  gtk_box_set_homogeneous (box, FALSE);
  gtk_box_set_spacing (box, 6);

  /* label and slider */
  label = gtk_label_new ("0:00");
  timer->label = GTK_LABEL (label);
  gtk_box_pack_start (box, label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  slider = gtk_hscale_new (NULL);
  timer->range = GTK_RANGE (slider);
  gtk_range_set_update_policy (timer->range, GTK_UPDATE_DISCONTINUOUS);
  gtk_scale_set_draw_value (GTK_SCALE (slider), FALSE);
  gtk_box_pack_start (box, slider, TRUE, TRUE, 0);
  gtk_widget_show (slider);
  g_signal_connect (slider, "value_changed", G_CALLBACK (cb_seek), timer);
  g_signal_connect (slider, "button-press-event",
      G_CALLBACK (cb_button_press), timer);
  g_signal_connect (slider, "button-release-event",
      G_CALLBACK (cb_button_release), timer);

  /* FIXME:
   * - show time we're seeking too if user moves slider.
   */
}

GtkWidget *
gst_player_timer_new (GstElement *play)
{
  GstPlayerTimer *timer =
      g_object_new (GST_PLAYER_TYPE_TIMER, NULL);

  if (play) {
    gst_object_ref (GST_OBJECT (play));
    timer->play = play;

    g_signal_connect (play, "state-change", G_CALLBACK (cb_state), timer);
  }
  cb_state (NULL, GST_STATE_PLAYING, GST_STATE_NULL, timer);

  return GTK_WIDGET (timer);
}

static void
gst_player_timer_dispose (GObject *object)
{
  GstPlayerTimer *timer = GST_PLAYER_TIMER (object);

  if (timer->play) {
    gst_object_unref (GST_OBJECT (timer->play));
    timer->play = NULL;
  }
}

static void
gst_player_timer_size_request (GtkWidget      *widget,
			       GtkRequisition *requisition)
{
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GST_PLAYER_IS_TIMER (widget));
  g_return_if_fail (requisition != NULL);

  /* let the parent figure out the height and minimum width */
  GTK_WIDGET_CLASS (parent_class)->size_request (widget, requisition);

  /* and then we make it somewhat larger so the slider always displays */
  requisition->width += 100;
}

static gchar *
time_to_string (guint64 time)
{
  if (time >= (GST_SECOND * 60 * 60))
    return g_strdup_printf ("%u:%02u:%02u",
			    (guint) (time / (GST_SECOND * 60 * 60)),
			    (guint) ((time / (GST_SECOND * 60)) % 60),
			    (guint) ((time / GST_SECOND) % 60));

  return g_strdup_printf ("%u:%02u",
			  (guint) (time / (GST_SECOND * 60)),
			  (guint) ((time / GST_SECOND) % 60));
}

void
gst_player_timer_progress (GstPlayerTimer *timer)
{
  GstFormat fmt = GST_FORMAT_TIME;
  GtkAdjustment *adj;
  GstQuery* query;
  gint64 value;
  gchar *label;
  gboolean new_len = FALSE, new_pos = FALSE;

  /* get length/position */
  if (!GST_CLOCK_TIME_IS_VALID (timer->len)) {
    query = gst_query_new_duration (fmt);
    if (gst_element_query (timer->play, query)) {
      gst_query_parse_duration (query, &fmt, &value);
      timer->len = value;
      new_len = TRUE;
    }
    gst_query_unref (query);
  }
  query = gst_query_new_position (fmt);
  if (gst_element_query (timer->play, query)) {
    gst_query_parse_position (query, &fmt, &value);
    if ((timer->pos / GST_SECOND) != (value / GST_SECOND))
      new_pos = TRUE;
    timer->pos = value;
  }
  gst_query_unref (query);

  if (GST_CLOCK_TIME_IS_VALID (timer->pos)) {
    gboolean new_label = FALSE;

    if (GST_CLOCK_TIME_IS_VALID (timer->len)) {
      if (new_len || new_pos) {
        gchar *tmp1 = time_to_string (timer->len),
            *tmp2 = time_to_string (timer->pos);
        label = g_strdup_printf ("%s / %s", tmp2, tmp1);
        g_free (tmp1);
        g_free (tmp2);
        new_label = TRUE;
      }
    } else {
      if (new_pos) {
        label = time_to_string (timer->pos);
        new_label = TRUE;
      }
    }

    if (new_label) {
      gtk_label_set_text (timer->label, label);
      g_free (label);
    }
  }

  if (timer->seeking)
    return;

  timer->lock = TRUE;

  /* if there was no slider yet, create one */
  if (new_len) {
    adj = GTK_ADJUSTMENT (gtk_adjustment_new (timer->pos, 0,
					      timer->len, GST_SECOND,
					      10 * GST_SECOND, 0));
    gtk_range_set_adjustment (timer->range, adj);
    gtk_widget_set_sensitive (GTK_WIDGET (timer), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET (timer->range), TRUE);
  }

  if (gtk_range_get_adjustment (timer->range)) {
    gtk_range_set_value (timer->range, timer->pos);
  }

  timer->lock = FALSE;
}

static gboolean
cb_button_press (GtkWidget      *widget,
		 GdkEventButton *event,
		 gpointer        data)
{
  GstPlayerTimer *timer = GST_PLAYER_TIMER (data);

  timer->seeking = TRUE;

  return FALSE;
}

static gboolean
cb_button_release (GtkWidget      *widget,
		   GdkEventButton *event,
		   gpointer        data)
{
  GstPlayerTimer *timer = GST_PLAYER_TIMER (data);

  timer->seeking = FALSE;

  return FALSE;
}

typedef struct _GstPlayerTimerStateChange {
  GstElement *play;
  GstState old_state, new_state;
  GstPlayerTimer *timer;
} GstPlayerTimerStateChange;

static gboolean
idle_state (gpointer data)
{
  GstPlayerTimerStateChange *st = data;

  if (st->old_state <= GST_STATE_READY &&
      st->new_state >= GST_STATE_PAUSED) {
    gtk_widget_set_sensitive (GTK_WIDGET (st->timer), TRUE);
  } else if (st->old_state >= GST_STATE_PAUSED &&
             st->new_state <= GST_STATE_READY) {
    gtk_widget_set_sensitive (GTK_WIDGET (st->timer), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (st->timer->range), FALSE);
    gtk_label_set_text (st->timer->label, "0:00");
    gtk_range_set_adjustment (st->timer->range, NULL);
    st->timer->len = GST_CLOCK_TIME_NONE;
    st->timer->pos = GST_CLOCK_TIME_NONE;
  }

  if (st->play)
    gst_object_unref (GST_OBJECT (st->play));
  g_object_unref (G_OBJECT (st->timer));
  g_free (st);

  /* once */
  return FALSE;
}

static void
cb_state (GstElement*play,
	  GstState   old_state,
	  GstState   new_state,
	  gpointer   data)
{
  GstPlayerTimer *timer = GST_PLAYER_TIMER (data);
  GstPlayerTimerStateChange *st = g_new (GstPlayerTimerStateChange, 1);

  st->play = play;
  if (st->play)
    gst_object_ref (GST_OBJECT (play));
  st->old_state = old_state;
  st->new_state = new_state;
  st->timer = timer;
  g_object_ref (G_OBJECT (timer));

  if (play)
    g_idle_add ((GSourceFunc) idle_state, st);
  else
    idle_state (st);
}

static void
cb_seek (GtkRange *range,
	 gpointer  data)
{
  GstPlayerTimer *timer = GST_PLAYER_TIMER (data);

  if (!timer->lock) {
    guint64 seek_val = gtk_range_get_value (range);

    /* try on video first */
    gst_element_seek_simple (timer->play, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, seek_val);
  }
}
