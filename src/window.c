/* GStreamer Media Player
 * (c) 2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * window.c: main window.
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
#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>

#include "disc.h"
#include "properties.h"
#include "stock.h"
#include "video.h"
#include "window.h"

static void	gst_player_window_class_init	(GstPlayerWindowClass *klass);
static void	gst_player_window_init		(GstPlayerWindow *win);
static void	gst_player_window_dispose	(GObject         *object);

static gboolean	gst_player_window_keypress	(GtkWidget       *widget,
						 GdkEventKey     *event);

static void	cb_open_file			(GtkWidget       *widget,
						 gpointer         data);
static void	cb_open_disc			(GtkWidget       *widget,
						 gpointer         data);
static void	cb_open_location		(GtkWidget       *widget,
						 gpointer         data);
static void	cb_properties			(GtkWidget       *widget,
						 gpointer         data);
static void	cb_play_or_pause		(GtkWidget       *widget,
						 gpointer         data);

static void	cb_zoom_1_1			(GtkWidget       *widget,
						 gpointer         data);
static void	cb_zoom_full			(GtkWidget       *widget,
						 gpointer         data);

static void	cb_exit				(GtkWidget       *widget,
						 gpointer         data);
static void	cb_about			(GtkWidget       *widget,
						 gpointer         data);

static void	cb_error			(GstElement      *play,
						 GstElement      *source,
						 GError          *error,
						 gchar           *debug,
						 gpointer         data);
static void	cb_state			(GstElement      *play,
						 GstState         old_state,
						 GstState         new_state,
						 gpointer         data);
static void	cb_found_tag			(GstElement      *play,
						 GstElement      *source,
						 const GstTagList *taglist,
						 gpointer         data);
static void	cb_eos				(GstElement      *play,
						 gpointer         data);

static GnomeAppClass *parent_class = NULL;

#define GST_PLAYER_ERROR \
  (gst_player_error_quark ())

static GQuark
gst_player_error_quark (void)
{
  static GQuark quark = 0;

  if (quark == 0)
    quark = g_quark_from_static_string ("gst-player-error-quark");

  return quark;
}

GType
gst_player_window_get_type (void)
{
  static GType gst_player_window_type = 0;

  if (!gst_player_window_type) {
    static const GTypeInfo gst_player_window_info = {
      sizeof (GstPlayerWindowClass),
      NULL,
      NULL,
      (GClassInitFunc) gst_player_window_class_init,
      NULL,
      NULL,
      sizeof (GstPlayerWindow),
      0,
      (GInstanceInitFunc) gst_player_window_init,
      NULL
    };

    gst_player_window_type =
	g_type_register_static (GNOME_TYPE_APP, 
				"GstPlayerWindow",
				&gst_player_window_info, 0);
  }

  return gst_player_window_type;
}

static void
gst_player_window_class_init (GstPlayerWindowClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_ref (GNOME_TYPE_APP);

  gobject_class->dispose = gst_player_window_dispose;
  gtkwidget_class->key_press_event = gst_player_window_keypress;
}

/*
 * Menus.
 */

#define GNOMEUIINFO_ITEM_QUICKKEY(label, tip, cb, stock_id, mask, key) \
	{ GNOME_APP_UI_ITEM, label, tip, (gpointer) cb, \
	  NULL, NULL, GNOME_APP_PIXMAP_STOCK, stock_id, \
	  key, mask, NULL }

static GnomeUIInfo open_menu[] = {
  GNOMEUIINFO_ITEM_QUICKKEY (N_("File"), N_("Open a local file"),
			     cb_open_file, GTK_STOCK_OPEN,
			     GDK_CONTROL_MASK, 'o'),
  GNOMEUIINFO_ITEM_QUICKKEY (N_("Disc"), N_("Open video or audio disc"),
			     cb_open_disc, GTK_STOCK_CDROM,
			     GDK_CONTROL_MASK, 'd'),
  GNOMEUIINFO_ITEM_QUICKKEY (N_("Location"), N_("Open a remote stream"),
			     cb_open_location, GTK_STOCK_NETWORK,
			     GDK_CONTROL_MASK, 'l'),
  GNOMEUIINFO_END
};

static GnomeUIInfo file_menu[] = {
  GNOMEUIINFO_SUBTREE (N_("Open"), open_menu),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_QUICKKEY (N_("Propertie_s"), N_("Stream properties"),
			     cb_properties, GTK_STOCK_PROPERTIES,
			     GDK_CONTROL_MASK, 's'),
  GNOMEUIINFO_ITEM_QUICKKEY (N_("_Play / Pause"), N_("Play or pause"),
			     cb_play_or_pause, GST_PLAYER_STOCK_PLAY,
			     GDK_CONTROL_MASK, 'p'),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_MENU_EXIT_ITEM (cb_exit, NULL),
  GNOMEUIINFO_END
};

#if 0
static GnomeUIInfo edit_menu[] = {
  /* preferences */
  GNOMEUIINFO_END
};
#endif

static GnomeUIInfo view_menu[] = {
  GNOMEUIINFO_ITEM_QUICKKEY (N_("Fullscreen"), N_("Toggle fullscreen"),
			     cb_zoom_full, GTK_STOCK_ZOOM_FIT,
			     GDK_MOD1_MASK, GDK_Return),
  GNOMEUIINFO_ITEM_QUICKKEY (N_("Zoom 1:1"), N_("Zoom to original media size"),
			     cb_zoom_1_1, GTK_STOCK_ZOOM_100,
			     GDK_MOD1_MASK, GDK_Home),
  GNOMEUIINFO_END
};

static GnomeUIInfo help_menu[] = {
  GNOMEUIINFO_HELP (PACKAGE),
  GNOMEUIINFO_MENU_ABOUT_ITEM (cb_about, NULL),
  GNOMEUIINFO_END
};

static GnomeUIInfo menu[] = {
  GNOMEUIINFO_MENU_FILE_TREE (file_menu),
  /*GNOMEUIINFO_MENU_EDIT_TREE (edit_menu),*/
  GNOMEUIINFO_MENU_VIEW_TREE (view_menu),
  GNOMEUIINFO_MENU_HELP_TREE (help_menu),
  GNOMEUIINFO_END
};

/*
 * Toolbar.
 */
                                                                                
static GnomeUIInfo tool[] = {
  GNOMEUIINFO_ITEM_STOCK (N_("Play"), N_("Start playing"),
			  cb_play_or_pause, GST_PLAYER_STOCK_PLAY),
  GNOMEUIINFO_ITEM_STOCK (N_("Pause"), N_("Pause playback"),
			  cb_play_or_pause, GST_PLAYER_STOCK_PAUSE),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_END
};

static void
gst_player_window_init (GstPlayerWindow *win)
{
  GnomeApp *app = GNOME_APP (win);
  GtkWidget *bar;

  win->fullscreen = FALSE;
  win->play = NULL;
  win->video = NULL;
  win->idle_id = 0;
  win->props = NULL;
  win->tagcache = NULL;

  /* init */
  gnome_app_construct (app, PACKAGE, PACKAGE_NAME);

  /* menus, toolbar */
  gnome_app_create_menus_with_data (app, menu, win);
  gnome_app_create_toolbar_with_data (app, tool, win);
  gtk_widget_hide (tool[1].widget);

  /* statusbar */
  bar = gnome_appbar_new (FALSE, TRUE, GNOME_PREFERENCES_USER);
  gnome_app_set_statusbar (app, bar);
  gnome_app_install_appbar_menu_hints (GNOME_APPBAR (bar), menu);
}

static void
cb_message (GstBus    *bus,
            GstMessage*message,
            gpointer   user_data)
{
  switch (message->type) {
    case GST_MESSAGE_EOS:
      cb_eos (GST_PLAYER_WINDOW (user_data)->play,
              user_data);
      break;
    case GST_MESSAGE_ERROR:
      {
        GError* error = NULL;
        gchar * debug = NULL;

        gst_message_parse_error (message,
                                 &error,
                                 &debug);

        cb_error (GST_PLAYER_WINDOW (user_data)->play,
                  GST_ELEMENT (message->src),
                  error,
                  debug,
                  user_data);

        g_error_free (error);
        g_free (debug);
      }
      break;
    case GST_MESSAGE_STATE_CHANGED:
      {
        GstState old_state, new_state;

        gst_message_parse_state_changed (message,
                                         &old_state,
                                         &new_state,
                                         NULL);

        cb_state (GST_PLAYER_WINDOW (user_data)->play,
                  old_state,
                  new_state,
                  user_data);
      }
      break;
    case GST_MESSAGE_TAG:
      {
        GstTagList* tags = NULL;

        gst_message_parse_tag (message,
                               &tags);

        cb_found_tag (GST_PLAYER_WINDOW (user_data)->play,
                      GST_ELEMENT (message->src),
                      tags,
                      user_data);

        gst_tag_list_free (tags);
      }
      break;
    default:
      break;
  }
}

GtkWidget *
gst_player_window_new (GError **err)
{
  GstPlayerWindow *win;
  BonoboDockItem  *item;
  GstElement *play;
  GstElement *audio, *video;
  GnomeApp *app;
  GtkWidget       *videow, *toolbar, *slider;
  GstBus          *bus;

  /* start with the player */
  if (!(play = gst_element_factory_make ("playbin", "player"))) {
    g_set_error (err, GST_PLAYER_ERROR, 1,
		 _("Failed to create playbin element"));
    return NULL;
  }

  /* set video/audio output */
  if (!(audio = gst_element_factory_make ("gconfaudiosink", "audio-sink"))) {
    g_set_error (err, GST_PLAYER_ERROR, 1,
		 _("Failed to obtain default audio sink from GConf"));
    goto fail;
  }
  g_object_set (play, "audio-sink", audio, NULL);

  if (!(video = gst_element_factory_make ("ximagesink", "video-sink"))) {
    g_set_error (err, GST_PLAYER_ERROR, 1,
		 _("Failed to obtain default video sink from GConf"));
    goto fail;
  }
  g_object_set (play, "video-sink", video, NULL);

  if (gst_element_set_state (GST_ELEMENT (play),
			     GST_STATE_READY) == GST_STATE_CHANGE_FAILURE) {
    g_set_error (err, GST_PLAYER_ERROR, 1,
		 _("Failed to set player to initial ready state - fatal"));
    goto fail;
  }

  /* actual window */
  win = g_object_new (GST_PLAYER_TYPE_WINDOW, NULL);
  app = GNOME_APP (win);
  win->play = play;
  bus = gst_pipeline_get_bus (GST_PIPELINE (play));
  gst_bus_add_signal_watch (bus);
  g_signal_connect (bus, "message", G_CALLBACK (cb_message), win);

  /* add slider */
  slider = gst_player_timer_new (play);
  win->timer = GST_PLAYER_TIMER (slider);
  item = gnome_app_get_dock_item_by_name (app, GNOME_APP_TOOLBAR_NAME);
  toolbar = bonobo_dock_item_get_child (item);
  gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar), slider,
                             _("Time"), NULL);
  gtk_widget_show (slider);

  /* video widget */
  videow = gst_player_video_new (video, play);
  win->video = videow;
  gnome_app_set_contents (app, videow);
  gtk_widget_show (videow);

  return GTK_WIDGET (win);

fail:
  gst_object_unref (GST_OBJECT (play));
  return NULL;
}

static void
gst_player_window_dispose (GObject *object)
{
  GstPlayerWindow *win = GST_PLAYER_WINDOW (object);

  if (win->play) {
    gst_element_set_state (GST_ELEMENT (win->play), GST_STATE_NULL);
    gst_object_unref (GST_OBJECT (win->play));
    win->play = NULL;
  }
  if (win->tagcache) {
    gst_tag_list_free (win->tagcache);
    win->tagcache = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gboolean
gst_player_window_keypress (GtkWidget   *widget,
			    GdkEventKey *event)
{
  if (GST_PLAYER_WINDOW (widget)->fullscreen &&
      (event->keyval == GDK_Escape ||
       (event->keyval == GDK_Return &&
        event->state == GDK_MOD1_MASK))) {
    cb_zoom_full (NULL, widget);

    return TRUE;
  }

  return GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event);
}

/*
 * Iterate callback, do some maintainance in the timer.
 */

static gboolean
cb_iterate (gpointer data)
{
  GstPlayerWindow *win = GST_PLAYER_WINDOW (data);
  gboolean res;
  GstState state;
  gst_element_get_state (win->play, &state, NULL, 100 /* usec */ * GST_NSECOND / GST_USECOND);
  res = state == GST_STATE_PLAYING;

  /* update timer - EOS might have killed us already so stop then*/
  if (res && win->idle_id != 0)
    gst_player_timer_progress (win->timer);
  else if (!res)
    win->idle_id = 0;

  return res;
}

/*
 * Menu/Toolbar actions.
 */

static gboolean
cb_play (gpointer data)
{
  GstPlayerWindow *win = GST_PLAYER_WINDOW (data);

  /* done in idle so that the dialog for location/file selection
   * is gone. now we can actually show new UI. */
  gst_element_set_state (win->play, GST_STATE_PLAYING);

  return FALSE;
}

void
gst_player_window_play (GstPlayerWindow* self,
                        gchar const    * uri)
{
  g_return_if_fail (GST_PLAYER_IS_WINDOW (self));
  g_return_if_fail (uri && *uri);

  g_object_set (G_OBJECT (self->play), "uri", uri, NULL);
  g_idle_add (cb_play, self);
}

static void
cb_open_file (GtkWidget *widget,
	      gpointer   data)
{
  GstPlayerWindow *win = GST_PLAYER_WINDOW (data);
  GtkWidget *filesel;
  const gchar *location;
  gchar *str;

  filesel = gtk_file_chooser_dialog_new (_("Open file"),
					 GTK_WINDOW (win),
					 GTK_FILE_CHOOSER_ACTION_OPEN,
					 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					 GTK_STOCK_OPEN, GTK_RESPONSE_OK,
					 NULL);
  gtk_widget_show (filesel);
  if (gtk_dialog_run (GTK_DIALOG (filesel)) == GTK_RESPONSE_OK) {
    gst_element_set_state (win->play, GST_STATE_READY);
    location = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filesel));
    if (location[0] == '/')
      str = g_strdup_printf ("file://%s", location);
    else
      str = g_strdup (location);
    gtk_widget_destroy (filesel);

    gst_player_window_play (win, str);
    g_free (str);
  } else {
    gtk_widget_destroy (filesel);
  }
}

static void
cb_open_disc (GtkWidget *widget,
	      gpointer   data)
{
  GstPlayerWindow *win = GST_PLAYER_WINDOW (data);
  /* FIXME: drive detection */
  GError *error = NULL;
  CdType type = cd_detect_type ("/dev/cdrom", &error);

  if (type == CD_TYPE_ERROR) {
    cb_error (win->play, win->play, error, "no details", win);
    g_error_free (error);
  } else {
    const gchar *uri = NULL;

    switch (type) {
      case CD_TYPE_DATA:
        /* FIXME:
         * - open correct location to mount path by default.
         */
        cb_open_file (widget, data);
        return;
      case CD_TYPE_CDDA:
        uri = "cdda://";
        break;
      case CD_TYPE_VCD:
        uri = "vcd://";
        break;
      case CD_TYPE_DVD:
        uri = "dvd://";
        break;
      default:
        g_assert_not_reached ();
    }
    gst_element_set_state (win->play, GST_STATE_READY);
    g_object_set (G_OBJECT (win->play), "uri", uri, NULL);

    g_idle_add (cb_play, win);
  }
}

static void
cb_open_location (GtkWidget *widget,
		  gpointer   data)
{
  GstPlayerWindow *win = GST_PLAYER_WINDOW (data);
  GtkWidget *dialog, *label, *entry, *box;
  gint response;

  dialog = gtk_dialog_new_with_buttons (_("Open location"),
					GTK_WINDOW (win),
					GTK_DIALOG_MODAL,
					GTK_STOCK_CANCEL, GTK_STOCK_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_OK,
					NULL);
  box = gtk_hbox_new (FALSE, 6);
  label = gtk_label_new (_("Location:"));
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (box), entry, TRUE, TRUE, 0);
  gtk_widget_show (entry);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), box,
		      TRUE, FALSE, 0);
  gtk_widget_show (box);
  gtk_widget_show (dialog);
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  if (response == GTK_RESPONSE_OK) {
    const gchar *location = gtk_entry_get_text (GTK_ENTRY (entry));

    gst_element_set_state (win->play, GST_STATE_READY);
    g_object_set (G_OBJECT (win->play), "uri", location, NULL);
    gtk_widget_destroy (dialog);

    g_idle_add (cb_play, win);
  } else {
    gtk_widget_destroy (dialog);
  }
}

static void
cb_destroy (GtkWidget *widget,
	    gpointer   data)
{
  GST_PLAYER_WINDOW (data)->props = NULL;
}

static void
cb_properties (GtkWidget *widget,
	       gpointer   data)
{
  GstPlayerWindow *win = GST_PLAYER_WINDOW (data);

  if (win->props) {
    gtk_window_present (GTK_WINDOW (win->props));
  } else {
    win->props = gst_player_properties_new ();
    gst_player_properties_update (GST_PLAYER_PROPERTIES (win->props),
				  win->play, win->tagcache);
    g_signal_connect (win->props, "destroy",
		      G_CALLBACK (cb_destroy), win);
    gtk_widget_show (win->props);
  }
}

static void
cb_play_or_pause (GtkWidget *widget,
		  gpointer   data)
{
  GstPlayerWindow *win = GST_PLAYER_WINDOW (data);
  GstState state;

  if (GST_STATE (win->play) == GST_STATE_PLAYING)
    state = GST_STATE_PAUSED;
  else
    state = GST_STATE_PLAYING;

  gst_element_set_state (GST_ELEMENT (win->play), state);
}

static void
cb_exit (GtkWidget *widget,
	 gpointer   data)
{
  gtk_widget_destroy (GTK_WIDGET (data));
}

static gboolean
idle_zoom_1_1 (gpointer data)
{
  GstPlayerVideo *video = GST_PLAYER_VIDEO (data);

  gst_player_video_default_size (video);

  /* once */
  return FALSE;
}

static void
cb_zoom_1_1 (GtkWidget *widget,
	     gpointer   data)
{
  GstPlayerWindow *win = GST_PLAYER_WINDOW (data);
  GstPlayerVideo *video = GST_PLAYER_VIDEO (win->video);

  /* this is hackish... */
  gtk_window_resize (GTK_WINDOW (win), 1, 1);
  g_idle_add (idle_zoom_1_1, video);
}

static void
cb_zoom_full (GtkWidget *widget,
	      gpointer   data)
{
  GstPlayerWindow *win = GST_PLAYER_WINDOW (data);
  GnomeApp *app = GNOME_APP (win);
  BonoboDockItem *item;

  item = gnome_app_get_dock_item_by_name (app, GNOME_APP_TOOLBAR_NAME);

  /* toggle */
  win->fullscreen = !win->fullscreen;

  /* effect */
  if (win->fullscreen) {
    gtk_widget_hide (app->menubar);
    gtk_widget_hide (app->statusbar);
    gtk_widget_hide (GTK_WIDGET (item));

    gtk_window_fullscreen (GTK_WINDOW (win));
  } else {
    gtk_window_unfullscreen (GTK_WINDOW (win));

    gtk_widget_show (app->menubar);
    gtk_widget_show (app->statusbar);
    gtk_widget_show (GTK_WIDGET (item));
  }
}

static void
cb_about (GtkWidget *widget,
	  gpointer   data)
{
  GtkWidget *about;
  const gchar *authors[] = { "Ronald Bultje <rbultje@ronald.bitfreak.net>",
                             NULL };

  about = gnome_about_new (_(PACKAGE_NAME), VERSION,
                           "(c) 2004 Ronald Bultje",
                           _("A GNOME/GStreamer-based media player"),
                           authors, NULL, NULL, NULL);

  gtk_widget_show (about);
}

static void
cb_error (GstElement *play,
	  GstElement *source,
	  GError     *error,
	  gchar      *debug,
	  gpointer    data)
{
  GstPlayerWindow *win = GST_PLAYER_WINDOW (data);
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new (GTK_WINDOW (win),
                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                   error->message);
  gtk_widget_show (dialog);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  gst_element_set_state (play, GST_STATE_READY);
}

static void
cb_state (GstElement*play,
	  GstState   old_state,
	  GstState   new_state,
	  gpointer   data)
{
  GstPlayerWindow *win = GST_PLAYER_WINDOW (data);

  if (old_state == GST_STATE_PLAYING) {
    /* show play button */
    gtk_widget_show (tool[0].widget);
    gtk_widget_hide (tool[1].widget);
    if (win->idle_id != 0) {
      g_source_remove (win->idle_id);
      win->idle_id = 0;
    }
  } else if (new_state == GST_STATE_PLAYING) {
    /* show pause button */
    gtk_widget_show (tool[1].widget);
    gtk_widget_hide (tool[0].widget);
    if (win->idle_id != 0)
      g_source_remove (win->idle_id);
    win->idle_id = g_idle_add (cb_iterate, win);
  }

  /* new movie loaded? */
  if (old_state == GST_STATE_READY &&
      new_state > GST_STATE_READY) {
    GList *streaminfo = NULL;
    gboolean have_video = FALSE, have_audio = FALSE;

    g_object_get (G_OBJECT (win->play), "stream-info",
		  &streaminfo, NULL);
    for ( ; streaminfo != NULL; streaminfo = streaminfo->next) {
      GObject *info = streaminfo->data;
      gint type;
      GParamSpec *pspec;
      GEnumValue *val;

      gchar* name;

      g_object_get (info, "type", &type, NULL);
      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (info), "type");
      val = g_enum_get_value (G_PARAM_SPEC_ENUM (pspec)->enum_class, type);

      name = g_utf8_strdown (val->value_name, -1);

      if (strstr (name, "audio")) {
        have_audio = TRUE;
      } else if (strstr (name, "video")) {
        have_video = TRUE;
      }

      g_free (name);
    }

    /* show/hide video window */
    if (have_video)
      gtk_widget_show (win->video);
    else
      gtk_widget_hide (win->video);

    /* well, this resizes the window, which is what I want. */
    gtk_window_resize (GTK_WINDOW (win), 1, 1);

    if (win->props) {
      gst_player_properties_update (GST_PLAYER_PROPERTIES (win->props),
				    win->play, NULL);
    }
  }

  /* discarded movie? */
  if (old_state > GST_STATE_READY &&
      new_state == GST_STATE_READY) {
    if (win->tagcache) {
      gst_tag_list_free (win->tagcache);
      win->tagcache = NULL;
    }

    if (win->props) {
      gst_player_properties_update (GST_PLAYER_PROPERTIES (win->props),
				    win->play, NULL);
    }
  }
}

static void
cb_found_tag (GstElement       *play,
	      GstElement       *source,
	      const GstTagList *taglist,
	      gpointer          data)
{
  GstPlayerWindow *win = GST_PLAYER_WINDOW (data);

  if (win->tagcache && taglist) {
    GstTagList *taglist2;

    taglist2 = gst_tag_list_merge (win->tagcache, taglist,
				  GST_TAG_MERGE_APPEND);
    gst_tag_list_free (win->tagcache);
    win->tagcache = taglist2;
  } else if (taglist) {
    win->tagcache = gst_tag_list_copy (taglist);
  }

  if (win->props) {
    gst_player_properties_update (GST_PLAYER_PROPERTIES (win->props),
				  win->play, win->tagcache);
  }
}

static void
cb_eos (GstElement *play,
	gpointer    data)
{
  GstPlayerWindow *win = GST_PLAYER_WINDOW (data);

  if (win->idle_id != 0) {
    g_source_remove (win->idle_id);
    win->idle_id = 0;
  }

  gst_element_set_state (win->play, GST_STATE_READY);
}
