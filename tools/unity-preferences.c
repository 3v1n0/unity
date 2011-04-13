/*
 * Copyright (C) 2011 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include "config.h"

static void on_edge_radio_toggled (GtkToggleButton *radio, GSettings *settings);

gint
main (gint argc, gchar **argv)
{
  GSettings *settings;
  GtkWidget *window;
  GtkWidget *main_box;
  GtkWidget *label, *align, *vbox;
  GtkWidget *edgeradio, *radio;

  bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

  gtk_init (&argc, &argv);

  /* Grab settings */
  settings = g_settings_new ("com.canonical.Unity.Launcher");

  /* Create the UI */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_container_set_border_width (GTK_CONTAINER (window), 12);
  gtk_window_set_title (GTK_WINDOW (window), _("Launcher & Menus"));
  g_signal_connect (window, "delete-event", G_CALLBACK (gtk_main_quit), NULL);

  main_box = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), main_box);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label), _("<b>Show the launcher when the pointer:</b>"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_container_add (GTK_CONTAINER (main_box), label);

  align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
  gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, 12, 0);
  gtk_container_add (GTK_CONTAINER (main_box), align);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (align), vbox);

  radio = gtk_radio_button_new_with_label (NULL, _("Pushes the left edge of the screen"));
  edgeradio = radio;
  gtk_container_add (GTK_CONTAINER (vbox), radio);
  
  radio = gtk_radio_button_new_with_label (gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio)),
                                           _("Touches the top left corner of the screen"));
  gtk_container_add (GTK_CONTAINER (vbox), radio);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio),
                                !g_settings_get_boolean (settings, "shows-on-edge"));
  g_signal_connect (edgeradio, "toggled", G_CALLBACK (on_edge_radio_toggled), settings);
 
  gtk_widget_show_all (window);

  gtk_main ();

  g_object_unref (settings);

  return 0;
}

static void
on_edge_radio_toggled (GtkToggleButton *radio, GSettings *settings)
{
  g_settings_set_boolean (settings, "shows-on-edge", gtk_toggle_button_get_active (radio));
}
