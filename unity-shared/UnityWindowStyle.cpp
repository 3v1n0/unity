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

#include <NuxCore/Logger.h>

#include "UnityWindowStyle.h"
#include "config.h"

namespace unity
{
namespace ui
{
namespace
{
  const char* const SWITCHER_TOP    = PKGDATADIR"/switcher_top.png";
  const char* const SWITCHER_LEFT   = PKGDATADIR"/switcher_left.png";
  const char* const SWITCHER_CORNER = PKGDATADIR"/switcher_corner.png";

  const char* const DIALOG_CLOSE     = PKGDATADIR"/dialog_close.png";
  const char* const DIALOG_HIGHLIGHT = PKGDATADIR"/dialog_close_highlight.png";
  const char* const DIALOG_PRESS     = PKGDATADIR"/dialog_close_press.png";

  double const DEFAULT_SCALE = 1.0;
}

DECLARE_LOGGER(logger, "unity.ui.unity.window.style");

UnityWindowStyle::UnityWindowStyle()
{
  LoadAllTextureInScale(DEFAULT_SCALE);
}

void UnityWindowStyle::LoadAllTextureInScale(double scale)
{
  auto& window_textures = unity_window_textures_[scale];

  window_textures[unsigned(WindowTextureType::BACKGROUND_TOP)]    = LoadTexture(scale, SWITCHER_TOP);
  window_textures[unsigned(WindowTextureType::BACKGROUND_LEFT)]   = LoadTexture(scale, SWITCHER_LEFT);
  window_textures[unsigned(WindowTextureType::BACKGROUND_CORNER)] = LoadTexture(scale, SWITCHER_CORNER);

  window_textures[unsigned(WindowTextureType::CLOSE_ICON)]             = LoadTexture(scale, DIALOG_CLOSE);
  window_textures[unsigned(WindowTextureType::CLOSE_ICON_HIGHLIGHTED)] = LoadTexture(scale, DIALOG_HIGHLIGHT);
  window_textures[unsigned(WindowTextureType::CLOSE_ICON_PRESSED)]     = LoadTexture(scale, DIALOG_PRESS);
}

nux::BaseTexture* UnityWindowStyle::LoadTexture(double scale, const char* const texture_name) const
{
  RawPixel max_size = GetDefaultMaxTextureSize(texture_name);
  return nux::CreateTexture2DFromFile(texture_name, max_size.CP(scale), true);
}

RawPixel UnityWindowStyle::GetDefaultMaxTextureSize(const char* const texture_name) const
{
  nux::Size size;
  gdk_pixbuf_get_file_info(texture_name, &size.width, &size.height);
  RawPixel max_size = std::max(std::round(size.width), std::round(size.height));

  return max_size;
}

UnityWindowStyle::Ptr const& UnityWindowStyle::Get()
{
  // This is set only the first time;
  static UnityWindowStyle::Ptr instance(new UnityWindowStyle());
  return instance;
}

int UnityWindowStyle::GetBorderSize() const
{
  return 30; // as measured from textures
}

int UnityWindowStyle::GetInternalOffset() const
{
  return 20;
}

int UnityWindowStyle::GetCloseButtonPadding() const
{
  return 3;
}

UnityWindowStyle::BaseTexturePtr UnityWindowStyle::GetTexture(double scale, WindowTextureType const& type)
{
  auto it = unity_window_textures_.find(scale);
  if (it == unity_window_textures_.end())
  {
    LoadAllTextureInScale(scale);

    it = unity_window_textures_.find(scale);
    if (it == unity_window_textures_.end())
    {
      LOG_ERROR(logger) << "Failed to create unity window style textures, for scale size: " << scale;
      return BaseTexturePtr(nullptr);
    }
  }

  return it->second[unsigned(type)];
}

} // namespace ui
} // namespace unity
