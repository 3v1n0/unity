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

