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
 *              Neil Jagdish Patel <neil.patel@canonical.com>
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
      WINDOW_BUTTON_CLOSE = 0,
      WINDOW_BUTTON_MINIMIZE,
      WINDOW_BUTTON_UNMAXIMIZE

    } WindowButtonType;

    typedef enum
    {
      WINDOW_STATE_NORMAL,
      WINDOW_STATE_PRELIGHT,
      WINDOW_STATE_PRESSED

    } WindowState;

    static PanelStyle * GetDefault ();

    PanelStyle ();
    ~PanelStyle ();

    nux::Color const& GetTextColor() const;
    nux::Color const& GetBackgroundTop() const;
    nux::Color const& GetBackgroundBottom() const;
    nux::Color const& GetTextShadow() const;
    nux::Color const& GetLineColor() const;

    GdkPixbuf * GetBackground (int width, int height);

    nux::BaseTexture * GetWindowButton (WindowButtonType type, WindowState state);

    GdkPixbuf * GetHomeButton ();

    sigc::signal<void> changed;

    bool IsAmbianceOrRadiance ();

  private:
    void        Refresh ();
    static void OnStyleChanged (GObject*    gobject,
                                GParamSpec* pspec,
                                gpointer    data);
    nux::BaseTexture * GetWindowButtonForTheme (WindowButtonType type, WindowState state);
  private:
    GtkWidget         *_offscreen;
    char              *_theme_name;
    nux::Color         _text;
    nux::Color         _bg_top;
    nux::Color         _bg_bottom;
    nux::Color         _text_shadow;
    nux::Color         _line;

    gulong            _gtk_theme_changed_id;
};

#endif // PANEL_STYLE_H
