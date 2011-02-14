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

PanelStyle::PanelStyle ()
{
  _offscreen = gtk_offscreen_window_new ();
  gtk_widget_set_name (_offscreen, "UnityPanelWidget");
  gtk_widget_set_size_request (_offscreen, 100, 24);
  gtk_widget_show (_offscreen);

  g_signal_connect (gtk_settings_get_default (), "notify::gtk-theme-name",
                    G_CALLBACK (PanelStyle::OnStyleChanged), this);
}

PanelStyle::~PanelStyle ()
{
  gtk_widget_destroy (_offscreen);
}

void
PanelStyle::Refresh ()
{
  GtkStyle*  style    = NULL;

  style = gtk_widget_get_style (_offscreen);

  _text.SetRed ((float) style->text[4].red / (float) 0xffff);
  _text.SetGreen ((float) style->text[4].green / (float) 0xffff);
  _text.SetBlue ((float) style->text[4].blue / (float) 0xffff);
  _text.SetAlpha (1.0f);

  _bg_top.SetRed ((float) style->bg[4].red / (float) 0xffff);
  _bg_top.SetGreen ((float) style->bg[4].green / (float) 0xffff);
  _bg_top.SetBlue ((float) style->bg[4].blue / (float) 0xffff);
  _bg_top = 0.4f * _bg_top;
  _bg_top.SetAlpha (1.0f);

  _bg_bottom.SetRed ((float) style->bg[4].red / (float) 0xffff);
  _bg_bottom.SetGreen ((float) style->bg[4].green / (float) 0xffff);
  _bg_bottom.SetBlue ((float) style->bg[4].blue / (float) 0xffff);
  _bg_bottom = 0.22f * _bg_bottom;
  _bg_bottom.SetAlpha (1.0f);

  _text_shadow.SetRed ((float) style->text[2].red / (float) 0xffff);
  _text_shadow.SetGreen ((float) style->text[2].green / (float) 0xffff);
  _text_shadow.SetBlue ((float) style->text[2].blue / (float) 0xffff);
  _text_shadow.SetAlpha (1.0f);

  changed.emit (this);
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

  return gtk_offscreen_window_get_pixbuf (GTK_OFFSCREEN_WINDOW (_offscreen));
}

