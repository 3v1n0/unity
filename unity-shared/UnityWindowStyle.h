// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012-2013 Canonical Ltd
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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 *              Marco Trevisan <marco.trevisan@canonical.com>
 *              Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#ifndef UNITYWINDOWSTYLE_H
#define UNITYWINDOWSTYLE_H

#include <sigc++/sigc++.h>
#include <Nux/Nux.h>

#include "RawPixel.h"

namespace unity
{
namespace ui
{

enum class WindowTextureType : unsigned
{
  BORDER_TOP,
  BORDER_LEFT,
  BORDER_CORNER,
  CLOSE_ICON,
  CLOSE_ICON_HIGHLIGHTED,
  CLOSE_ICON_PRESSED,
  Size
};

class UnityWindowStyle
{
public:
  typedef std::shared_ptr<UnityWindowStyle> Ptr;
  typedef nux::ObjectPtr<nux::BaseTexture> BaseTexturePtr;

  static UnityWindowStyle::Ptr const& Get();

  BaseTexturePtr GetTexture(double scale, WindowTextureType const& type);
  RawPixel GetCloseButtonPadding() const;
  RawPixel GetBorderSize() const;
  RawPixel GetInternalOffset() const;

private:
  UnityWindowStyle();

  void ReloadIcons();
  void LoadAllTextureInScale(double scale);
  nux::BaseTexture* LoadTexture(std::string const& texture_name, double scale) const;
  RawPixel GetDefaultMaxTextureSize(std::string const& texture_path) const;

  void OnMonitorChanged(int primary, std::vector<nux::Geometry> const& monitors);
  void OnThemeChanged(std::string const&);
  void CleanUpUnusedTextures();

  typedef std::array<BaseTexturePtr, size_t(WindowTextureType::Size)> UnityWindowTextures;
  std::unordered_map<double, UnityWindowTextures> unity_window_textures_;

};

} // namespace ui
} // namespace unity

#endif
