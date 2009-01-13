/* GStreamer Media Player
 * (c) 2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * properties.c: tags and streaminfo display
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

#include <gnome.h>

#include "properties.h"

static void	gst_player_properties_class_init (GstPlayerPropertiesClass *klass);
static void	gst_player_properties_init	(GstPlayerProperties *props);

static void	gst_player_properties_response	(GtkDialog *dialog,
						 gint       response_id);

static GtkDialogClass *parent_class = NULL;

GType
gst_player_properties_get_type (void)
{
  static GType gst_player_properties_type = 0;

  if (!gst_player_properties_type) {
    static const GTypeInfo gst_player_properties_info = {
      sizeof (GstPlayerPropertiesClass),
      NULL,
      NULL,
      (GClassInitFunc) gst_player_properties_class_init,
      NULL,
      NULL,
      sizeof (GstPlayerProperties),
      0,
      (GInstanceInitFunc) gst_player_properties_init,
      NULL
    };

    gst_player_properties_type =
	g_type_register_static (GTK_TYPE_DIALOG, 
				"GstPlayerProperties",
				&gst_player_properties_info, 0);
  }

  return gst_player_properties_type;
}

static void
gst_player_properties_class_init (GstPlayerPropertiesClass *klass)
{
  GtkDialogClass *gtkdialog_class = GTK_DIALOG_CLASS (klass);

  parent_class = g_type_class_ref (GTK_TYPE_DIALOG);

  gtkdialog_class->response = gst_player_properties_response;
}

static void
gst_player_properties_init (GstPlayerProperties *props)
{
  props->content = NULL;

  gtk_window_set_title (GTK_WINDOW (props),
			_("Stream properties"));
  gtk_container_set_border_width (GTK_CONTAINER (props), 6);

  /* close button */
  gtk_dialog_add_button (GTK_DIALOG (props),
			 GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (props)->vbox), 6);
}

GtkWidget *
gst_player_properties_new (void)
{
  GstPlayerProperties *props;

  props = g_object_new (GST_PLAYER_TYPE_PROPERTIES, NULL);
  gst_player_properties_update (props, NULL, NULL);

  return GTK_WIDGET (props);
}

static void
gst_player_properties_response (GtkDialog *dialog,
				gint       response_id)
{
  switch (response_id) {
    case GTK_RESPONSE_CLOSE:
      gtk_widget_destroy (GTK_WIDGET (dialog));
      break;
    default:
      break;
  }

  if (parent_class->response)
    parent_class->response (dialog, response_id);
}

void
gst_player_properties_update (GstPlayerProperties *props,
			      GstElement *play,
			      const GstTagList *taglist)
{
  GtkWidget *label;
  GList *streaminfo = NULL;
  gboolean have_video = FALSE, have_audio = FALSE, have_metadata = FALSE;
  gchar *str;
  gchar *str2;
  gdouble fps = 0.;
  gint width = 0, height = 0, rate = 0, channels = 0, pos = 0, n;
  const gchar *tgl[] = { GST_TAG_ARTIST, GST_TAG_TITLE, GST_TAG_ALBUM,
      GST_TAG_GENRE, GST_TAG_COMMENT, NULL };

  if (props->content) {
    gtk_widget_destroy (props->content);
  }

  if (!play || GST_STATE (play) <= GST_STATE_READY) {
    props->content = gtk_label_new (_("No media loaded."));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (props)->vbox),
			props->content, TRUE, TRUE, 0);
    gtk_widget_show (props->content);
    return;
  }

  /* get metadata and streaminfo */
  g_object_get (G_OBJECT (play), "stream-info",
		&streaminfo, NULL);
  for ( ; streaminfo != NULL; streaminfo = streaminfo->next) {
    GObject *info = streaminfo->data;
    gint type;
    GParamSpec *pspec;
    GEnumValue *val;
    GstPad *pad = NULL;
    GstStructure *s = NULL;

    g_object_get (info, "type", &type, NULL);
    pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (info), "type");
    val = g_enum_get_value (G_PARAM_SPEC_ENUM (pspec)->enum_class, type);
    g_object_get (info, "object", &pad, NULL);
    if (pad && GST_IS_PAD (pad) && GST_PAD_CAPS (pad)) {
      s = gst_caps_get_structure (GST_PAD_CAPS (pad), 0);
    }

    if (strstr (val->value_name, "AUDIO")) {
      have_audio = TRUE;
      gst_structure_get_int (s, "channels", &channels);
      gst_structure_get_int (s, "rate", &rate);
    } else if (strstr (val->value_name, "VIDEO")) {
      have_video = TRUE;
      gst_structure_get_int (s, "width", &width);
      gst_structure_get_int (s, "height", &height);
      gst_structure_get_double (s, "framerate", &fps);
    }
  }

  props->content = gtk_table_new (2, 1, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (props->content), 0);
  gtk_table_set_row_spacings (GTK_TABLE (props->content), 0);

#define attach(t, w, x1, x2, y) \
  do { \
    gtk_table_attach_defaults (GTK_TABLE (t), w, x1, x2, y, y + 1); \
    y++; \
    gtk_widget_show (w); \
  } while (0);

  /* general metadata */
  label = gtk_label_new (_("<b>Stream information</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  attach (props->content, label, 0, 2, pos);
  gtk_widget_show (label);

  if (taglist) {
    for (n = 0; tgl[n] != NULL; n++) {
      if (gst_tag_list_get_string (taglist, tgl[n], &str)) {
        str2 = g_strdup_printf (_("  %s: "), gst_tag_get_nick (tgl[n]));
        label = gtk_label_new (str2);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        g_free (str2);
        attach (props->content, label, 0, 1, pos);
        pos--;
        label = gtk_label_new (str);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        attach (props->content, label, 1, 2, pos);
        have_metadata = TRUE;
      }
    }
  }
  if (!have_metadata) {
    label = gtk_label_new (_("  No stream information found"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    attach (props->content, label, 0, 2, pos);
  }

  if (have_video) {
    label = gtk_label_new (" ");
    attach (props->content, label, 0, 2, pos);

    label = gtk_label_new (_("<b>Video information</b>"));
    gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    attach (props->content, label, 0, 2, pos);
    gtk_widget_show (label);

    label = gtk_label_new (_("  Video size/framerate: "));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    attach (props->content, label, 0, 1, pos);
    pos--;
    str2 = g_strdup_printf (_("%dx%d at %.02lf fps"),
			    width, height, fps);
    label = gtk_label_new (str2);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    g_free (str2);
    attach (props->content, label, 1, 2, pos);

    if (taglist &&
        gst_tag_list_get_string (taglist, GST_TAG_VIDEO_CODEC, &str)) {
      str2 = g_strdup_printf (_("  %s: "),
			      gst_tag_get_nick (GST_TAG_VIDEO_CODEC));
      label = gtk_label_new (str2);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      g_free (str2);
      attach (props->content, label, 0, 1, pos);
      pos--;
      label = gtk_label_new (str);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      attach (props->content, label, 1, 2, pos);
    }
  }

  if (have_audio) {
    label = gtk_label_new (" ");
    attach (props->content, label, 0, 2, pos);

    label = gtk_label_new (_("<b>Audio information</b>"));
    gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    attach (props->content, label, 0, 2, pos);
    gtk_widget_show (label);

    label = gtk_label_new (_("  Audio channels/samplerate: "));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    attach (props->content, label, 0, 1, pos);
    pos--;
    str2 = g_strdup_printf (_("%d channels at %d Hz"),
			    channels, rate);
    label = gtk_label_new (str2);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    g_free (str2);
    attach (props->content, label, 1, 2, pos);

    if (taglist &&
        gst_tag_list_get_string (taglist, GST_TAG_AUDIO_CODEC, &str)) {
      str2 = g_strdup_printf (_("  %s: "),
			      gst_tag_get_nick (GST_TAG_AUDIO_CODEC));
      label = gtk_label_new (str2);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      g_free (str2);
      attach (props->content, label, 0, 1, pos);
      pos--;
      label = gtk_label_new (str);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      attach (props->content, label, 1, 2, pos);
    }
  }

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (props)->vbox),
		      props->content, TRUE, TRUE, 0);
  gtk_widget_show (props->content);
}
