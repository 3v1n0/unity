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

PanelStyle*
PanelStyle::GetDefault ()
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
  gint       i;

  menuBar = gtk_menu_bar_new ();
  style = gtk_widget_get_style (menuBar);

  for (i = 0; i < 5; i++)
  {
    g_print ("%s[%d]: %4.3f/%4.3f/%4.3f\n",
             "fg",
             i,
             (float) style->fg[i].red / (float) 0xffff,
             (float) style->fg[i].green / (float) 0xffff,
             (float) style->fg[i].blue / (float) 0xffff);
  }
  g_print ("\n");

  for (i = 0; i < 5; i++)
  {
    g_print ("%s[%d]: %4.3f/%4.3f/%4.3f\n",
             "bg",
             i,
             (float) style->bg[i].red / (float) 0xffff,
             (float) style->bg[i].green / (float) 0xffff,
             (float) style->bg[i].blue / (float) 0xffff);
  }
  g_print ("\n");

  for (i = 0; i < 5; i++)
  {
    g_print ("%s[%d]: %4.3f/%4.3f/%4.3f\n",
             "light",
             i,
             (float) style->light[i].red / (float) 0xffff,
             (float) style->light[i].green / (float) 0xffff,
             (float) style->light[i].blue / (float) 0xffff);
  }
  g_print ("\n");

  for (i = 0; i < 5; i++)
  {
    g_print ("%s[%d]: %4.3f/%4.3f/%4.3f\n",
             "dark",
             i,
             (float) style->dark[i].red / (float) 0xffff,
             (float) style->dark[i].green / (float) 0xffff,
             (float) style->dark[i].blue / (float) 0xffff);
  }
  g_print ("\n");

  for (i = 0; i < 5; i++)
  {
    g_print ("%s[%d]: %4.3f/%4.3f/%4.3f\n",
             "mid",
             i,
             (float) style->mid[i].red / (float) 0xffff,
             (float) style->mid[i].green / (float) 0xffff,
             (float) style->mid[i].blue / (float) 0xffff);
  }
  g_print ("\n");

  for (i = 0; i < 5; i++)
  {
    g_print ("%s[%d]: %4.3f/%4.3f/%4.3f\n",
             "text",
             i,
             (float) style->text[i].red / (float) 0xffff,
             (float) style->text[i].green / (float) 0xffff,
             (float) style->text[i].blue / (float) 0xffff);
  }
  g_print ("\n");

  for (i = 0; i < 5; i++)
  {
    g_print ("%s[%d]: %4.3f/%4.3f/%4.3f\n",
             "base",
             i,
             (float) style->base[i].red / (float) 0xffff,
             (float) style->base[i].green / (float) 0xffff,
             (float) style->base[i].blue / (float) 0xffff);
  }
  g_print ("\n");

  _text.SetRed (style->text[4].red / 0xffff);
  _text.SetGreen (style->text[4].green / 0xffff);
  _text.SetBlue (style->text[4].blue / 0xffff);
  _text.SetAlpha (1.0f);

  _bgTop.SetRed (style->bg[1].red / 0xffff);
  _bgTop.SetGreen (style->bg[1].green / 0xffff);
  _bgTop.SetBlue (style->bg[1].blue / 0xffff);
  _bgTop.SetAlpha (1.0f);

  _bgBottom.SetRed (style->bg[3].red / 0xffff);
  _bgBottom.SetGreen (style->bg[3].green / 0xffff);
  _bgBottom.SetBlue (style->bg[3].blue / 0xffff);
  _bgBottom.SetAlpha (1.0f);

  _textShadow.SetRed (style->text[2].red / 0xffff);
  _textShadow.SetGreen (style->text[2].green / 0xffff);
  _textShadow.SetBlue (style->text[2].blue / 0xffff);
  _textShadow.SetAlpha (1.0f);

  g_object_unref (style);
  g_object_unref (menuBar);
}

PanelStyle::~PanelStyle ()
{
}
