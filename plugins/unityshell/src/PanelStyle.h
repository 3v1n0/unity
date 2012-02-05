// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010, 2011 Canonical Ltd
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
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/GLibSignal.h>

namespace unity
{
namespace panel
{

enum class WindowButtonType
{
  CLOSE,
  MINIMIZE,
  UNMAXIMIZE
};

enum class WindowState
{
  NORMAL,
  PRELIGHT,
  PRESSED
};

class Style
{
public:
  Style();
  ~Style();

  static Style& Instance();

  GtkStyleContext* GetStyleContext();

  nux::NBitmapData* GetBackground(int width, int height, float opacity);

  nux::BaseTexture* GetWindowButton(WindowButtonType type, WindowState state);

  GdkPixbuf* GetHomeButton();

  sigc::signal<void> changed;

private:
  void Refresh();

  nux::BaseTexture* GetWindowButtonForTheme(WindowButtonType type,
                                            WindowState state);
private:
  glib::Object<GtkStyleContext> _style_context;
  glib::Signal<void, GtkSettings*, GParamSpec*> _style_changed_signal;
  std::string _theme_name;
  nux::Color _text_color;
};

}
}
#endif // PANEL_STYLE_H
