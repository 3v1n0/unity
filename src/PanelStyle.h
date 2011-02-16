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

#ifndef PANEL_STYLE_H
#define PANEL_STYLE_H

#include <Nux/Nux.h>

#include <gtk/gtk.h>

class PanelStyle : public nux::Object
{
  public:
    typedef enum
    {
      CLOSE = 0,
      MINIMIZE,
      MAXIMIZE

    } PanelWindowButtonType;

    typedef enum
    {
      NORMAL,
      PRELIGHT,
      PRESSED
    
    } PanelWindowState;
    
    static PanelStyle * GetDefault ();

    PanelStyle ();
    ~PanelStyle ();

    nux::Color& GetTextColor ();
    nux::Color& GetBackgroundTop ();
    nux::Color& GetBackgroundBottom ();
    nux::Color& GetTextShadow ();

    GdkPixbuf * GetBackground (int width, int height);

    nux::BaseTexture * GetWindowButton (PanelWindowButtonType type, PanelWindowState state);

    sigc::signal<void> changed;

  private:
    void        Refresh ();
    static void OnStyleChanged (GObject*    gobject,
                                GParamSpec* pspec,
                                gpointer    data);
  private:
    GtkWidget         *_offscreen;
    nux::Color         _text;
    nux::Color         _bg_top;
    nux::Color         _bg_bottom;
    nux::Color         _text_shadow;
};

#endif // PANEL_STYLE_H
