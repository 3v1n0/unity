/*
 * Copyright (C) 2010 Canonical Ltd
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
 * Authored by: Mirco Müller <mirco.mueller@canonical.com>
 */

#include <gtk/gtk.h>

#include "PanelStyle.h"

static PanelStyle *_style = NULL;

PanelStyle::PanelStyle ()
: _theme_name (NULL)
{
  _offscreen = gtk_offscreen_window_new ();
  gtk_widget_set_name (_offscreen, "UnityPanelWidget");
  gtk_widget_set_size_request (_offscreen, 100, 24);
  gtk_widget_show_all (_offscreen);

  g_signal_connect (gtk_settings_get_default (), "notify::gtk-theme-name",
                    G_CALLBACK (PanelStyle::OnStyleChanged), this);

  Refresh ();
}

PanelStyle::~PanelStyle ()
{
  gtk_widget_destroy (_offscreen);

  if (_style == this)
    _style = NULL;

  g_free (_theme_name);
}

PanelStyle *
PanelStyle::GetDefault ()
{
  if (G_UNLIKELY (!_style))
    _style = new PanelStyle ();

  return _style;
}

void
PanelStyle::Refresh ()
{
  GtkStyle*  style    = NULL;

  if (_theme_name)
    g_free (_theme_name);

  _theme_name = NULL;
  g_object_get (gtk_settings_get_default (), "gtk-theme-name", &_theme_name, NULL);
  
  style = gtk_widget_get_style (_offscreen);

  _text.SetRed ((float) style->text[0].red / (float) 0xffff);
  _text.SetGreen ((float) style->text[0].green / (float) 0xffff);
  _text.SetBlue ((float) style->text[0].blue / (float) 0xffff);
  _text.SetAlpha (1.0f);

  _text_shadow.SetRed ((float) style->dark[0].red / (float) 0xffff);
  _text_shadow.SetGreen ((float) style->dark[0].green / (float) 0xffff);
  _text_shadow.SetBlue ((float) style->dark[0].blue / (float) 0xffff);
  _text_shadow.SetAlpha (1.0f);

  _bg_top.SetRed ((float) style->bg[1].red / (float) 0xffff);
  _bg_top.SetGreen ((float) style->bg[1].green / (float) 0xffff);
  _bg_top.SetBlue ((float) style->bg[1].blue / (float) 0xffff);
  _bg_top.SetAlpha (1.0f);

  _bg_bottom.SetRed ((float) style->bg[0].red / (float) 0xffff);
  _bg_bottom.SetGreen ((float) style->bg[0].green / (float) 0xffff);
  _bg_bottom.SetBlue ((float) style->bg[0].blue / (float) 0xffff);
  _bg_bottom.SetAlpha (1.0f);

  changed.emit ();
}

nux::Color&
PanelStyle::GetTextColor ()
{
  return _text;
}

nux::Color&
PanelStyle::GetBackgroundTop ()
{
  return _bg_top;
}

nux::Color&
PanelStyle::GetBackgroundBottom ()
{
  return _bg_bottom;
}

nux::Color&
PanelStyle::GetTextShadow ()
{
  return _text_shadow;
}

void
PanelStyle::OnStyleChanged (GObject*    gobject,
                            GParamSpec* pspec,
                            gpointer    data)
{
  PanelStyle* self = (PanelStyle*) data;

  self->Refresh ();
}

GdkPixbuf *
PanelStyle::GetBackground (int width, int height)
{
  gtk_widget_set_size_request (_offscreen, width, height);
  gdk_window_process_updates (gtk_widget_get_window (_offscreen), TRUE);
  
  return gtk_offscreen_window_get_pixbuf (GTK_OFFSCREEN_WINDOW (_offscreen));
}

nux::BaseTexture *
PanelStyle::GetWindowButton (WindowButtonType type, WindowState state)
{
#define ICON_LOCATION "/usr/share/themes/%s/metacity-1/%s%s.png"
  nux::BaseTexture * texture = NULL;
  const char *names[] = { "close", "minimize", "unmaximize" };
  const char *states[] = { "", "_focused_prelight", "_focused_pressed" };

  // I wish there was a magic bullet here, but not all themes actually set the panel to be
  // the same style as the window titlebars (e.g. Clearlooks) so we can just grab the 
  // metacity window buttons as that would look horrible
  if (g_strcmp0 (_theme_name, "Ambiance") == 0
      || g_strcmp0 (_theme_name, "Radiance") == 0)
  {
    char      *filename;
    GdkPixbuf *pixbuf;
    GError    *error = NULL;

    filename = g_strdup_printf (ICON_LOCATION, _theme_name, names[type], states[state]);

    pixbuf = gdk_pixbuf_new_from_file (filename, &error);
    if (error)
    {
      g_warning ("Unable to load window button %s: %s", filename, error->message);
      g_error_free (error);
      error = NULL;
    }
    else
      texture = nux::CreateTexture2DFromPixbuf (pixbuf, true);

    g_free (filename);
    g_object_unref (pixbuf);
  }
  else
  {

  }

  return texture;
}
