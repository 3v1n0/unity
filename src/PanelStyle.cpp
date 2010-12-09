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

PanelStyle* PanelStyle::_panel_style = NULL;

PanelStyle*
PanelStyle::GetDefault ()
{
  if (_panel_style == NULL)
    _panel_style = new PanelStyle ();

  _panel_style->Reference (); 
  return _panel_style;
}

nux::Color&
PanelStyle::GetTextColor ()
{
  nux::Color& color = _text;

  return color;
}

nux::Color&
PanelStyle::GetBackgroundTop ()
{
  nux::Color& color = _bg_top;

  return color;
}

nux::Color&
PanelStyle::GetBackgroundBottom ()
{
  nux::Color& color = _bg_bottom;

  return color;
}

nux::Color&
PanelStyle::GetTextShadow ()
{
  nux::Color& color = _text_shadow;

  return color;
}

PanelStyle::PanelStyle ()
{
  GtkWidget* menu_bar = NULL;
  GtkStyle*  style    = NULL;

  menu_bar = gtk_menu_bar_new ();
  style = gtk_widget_get_style (menu_bar);

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

  g_object_unref (style);
  g_object_unref (menu_bar);
}

PanelStyle::~PanelStyle ()
{
  if (GetReferenceCount () != 0)
    g_warning ("PanelStyle::~PanelStyle() - Reference-counter is not 0!");
}
