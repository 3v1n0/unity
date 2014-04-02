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
 *              Marco Trevisan <3v1n0@ubuntu.com>
 */

#ifndef PANEL_STYLE_H
#define PANEL_STYLE_H

#include <NuxCore/ObjectPtr.h>
#include <NuxCore/Property.h>

#include <gtk/gtk.h>
#include <UnityCore/GLibWrapper.h>

namespace nux
{
class BaseTexture;
}

namespace unity
{
namespace decoration
{
enum class WindowButtonType : unsigned;
enum class WidgetState : unsigned;
}

namespace panel
{
using WindowButtonType = decoration::WindowButtonType;
using WindowState = decoration::WidgetState;
using BaseTexturePtr = nux::ObjectPtr<nux::BaseTexture>;

enum class PanelItem
{
  INDICATOR,
  MENU,
  TITLE
};

class Style : public sigc::trackable
{
public:
  Style();
  ~Style();

  static Style& Instance();

  GtkStyleContext* GetStyleContext(PanelItem);
  BaseTexturePtr GetBackground(int monitor = 0);
  BaseTexturePtr GetWindowButton(WindowButtonType type, WindowState state, int monitor = 0);
  BaseTexturePtr GetDashWindowButton(WindowButtonType type, WindowState state, int monitor = 0);
  std::string GetFontDescription(PanelItem);

  int PanelHeight(int monitor = 0) const;

  sigc::signal<void> changed;

private:
  void OnThemeChanged(std::string const&);
  void RefreshContext();
  void DPIChanged();

  void UpdateFontSize();
  void UpdatePanelHeight();

  glib::Object<GtkStyleContext> style_context_;
  std::vector<BaseTexturePtr> bg_textures_;
  mutable std::vector<unsigned> panel_heights_;
};

}
}
#endif // PANEL_STYLE_H
