/* GStreamer Media Player
 * (c) 2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * main.c: main application.
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

#include <getopt.h>
#include <glib.h>
#include <gst/gst.h>
#include <gnome.h>

#include "stock.h"
#include "window.h"

static void
register_stock_icons (void)
{
  GtkIconFactory *icon_factory;
  struct
  {
    gchar *filename, *stock_id;
  } list[] = {
    { "pause.png", GST_PLAYER_STOCK_PAUSE },
    { "play.png",  GST_PLAYER_STOCK_PLAY  },
    { NULL, NULL }
  };
  gint num;

  icon_factory = gtk_icon_factory_new ();
  gtk_icon_factory_add_default (icon_factory);

  for (num = 0; list[num].filename != NULL; num++) {
    gchar *filename =
	gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_APP_PIXMAP,
				   list[num].filename, TRUE, NULL);

    if (filename) {
      GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
      GtkIconSet *icon_set = gtk_icon_set_new_from_pixbuf (pixbuf);

      gtk_icon_factory_add (icon_factory, list[num].stock_id, icon_set);
      g_free (filename);
    }
  }
}

static void
cb_destroy (GtkWidget *widget,
	    gpointer   data)
{
  gtk_main_quit ();
}

gint
main (gint   argc,
      gchar *argv[])
{
  GError *err = NULL;
  gchar *appfile;
  GtkWidget *win;
  GOptionContext* options;

  g_thread_init (NULL);

  options = g_option_context_new ("");
  /* init gstreamer */
  g_option_context_add_group(options, gst_init_get_option_group ());

  /* init gtk/gnome */
  gnome_program_init (PACKAGE, VERSION, LIBGNOMEUI_MODULE, argc, argv,
		      GNOME_PARAM_GOPTION_CONTEXT, options,
		      GNOME_PARAM_APP_DATADIR, DATA_DIR, NULL);

  /* init ourselves */
  register_stock_icons ();

  /* add appicon image */
  appfile = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_APP_PIXMAP,
				       ICON_DIR "/gst-player.png", TRUE,
				       NULL);
  if (appfile) {
    gnome_window_icon_set_default_from_file (appfile);
    g_free (appfile);
  }

  /* window contains everything automagically */
  if (!(win = gst_player_window_new (&err))) {
    win = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR,
				  GTK_BUTTONS_CLOSE,
				  _("Failed to start player: %s"),
				  err ? err->message : _("unknown error"));
    gtk_widget_show (win);
    gtk_dialog_run (GTK_DIALOG (win));
    gtk_widget_destroy (win);
    return -1;
  }

  g_signal_connect (win, "destroy", G_CALLBACK (cb_destroy), NULL);
  gtk_widget_show (win);
  gtk_main ();

  return 0;
}
