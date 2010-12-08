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

PanelStyle* PanelStyle::_panelStyle = NULL;

PanelStyle* PanelStyle::GetDefault ()
{
  if (_panelStyle == NULL)
    _panelStyle = new PanelStyle ();

  return _panelStyle;
}

void
PanelStyle::GetTextColor (nux::Color* color)
{
  if (!color)
    return;

  color = &_text;
}

void
PanelStyle::GetBackgroundTop (nux::Color* color)
{
  if (!color)
    return;

  color = &_bgTop;
}

void
PanelStyle::GetBackgroundBottom (nux::Color* color)
{
  if (!color)
    return;

  color = &_bgBottom;
}

void
PanelStyle::GetTextShadow (nux::Color* color)
{
  if (!color)
    return;

  color = &_textShadow;
}

PanelStyle::PanelStyle ()
{
  GtkWidget* menuBar = NULL;
  GtkStyle*  style   = NULL;

  menuBar = gtk_menu_bar_new ();
  style = gtk_widget_get_style (menuBar);

  _text.SetRed (style->text[0].red / 0xffff);
  _text.SetGreen (style->text[0].green / 0xffff);
  _text.SetBlue (style->text[0].blue / 0xffff);
  _text.SetAlpha (1.0f);

  _bgTop.SetRed (style->bg[0].red / 0xffff);
  _bgTop.SetGreen (style->bg[0].green / 0xffff);
  _bgTop.SetBlue (style->bg[0].blue / 0xffff);
  _bgTop.SetAlpha (1.0f);

  _bgBottom.SetRed (style->bg[1].red / 0xffff);
  _bgBottom.SetGreen (style->bg[1].green / 0xffff);
  _bgBottom.SetBlue (style->bg[1].blue / 0xffff);
  _bgBottom.SetAlpha (1.0f);

  _textShadow.SetRed (style->text[1].red / 0xffff);
  _textShadow.SetGreen (style->text[1].green / 0xffff);
  _textShadow.SetBlue (style->text[1].blue / 0xffff);
  _textShadow.SetAlpha (1.0f);

  g_object_unref (style);
  g_object_unref (menuBar);
}

PanelStyle::~PanelStyle ()
{
}
