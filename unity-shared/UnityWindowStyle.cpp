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

#include "UnitySettings.h"
#include "UnityWindowStyle.h"
#include "UScreen.h"
#include "config.h"

#include <unordered_set>

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


  RawPixel const INTERNAL_OFFSET = 20_em;
  RawPixel const BORDER_SIZE     = 30_em;
  RawPixel const CLOSE_PADDING   =  3_em;
}

DECLARE_LOGGER(logger, "unity.ui.unity.window.style");


UnityWindowStyle::UnityWindowStyle()
{
  unsigned monitors = UScreen::GetDefault()->GetPluggedMonitorsNumber();
  auto& settings = Settings::Instance();

  // Pre-load scale values per monitor
  for (unsigned i = 0; i < monitors; ++i)
  {
    double scale = settings.Instance().em(i)->DPIScale();

    if (unity_window_textures_.find(scale) == unity_window_textures_.end())
      LoadAllTextureInScale(scale);
  }

  settings.Instance().dpi_changed.connect(sigc::mem_fun(this, &UnityWindowStyle::CleanUpUnusedTextures));
  UScreen::GetDefault()->changed.connect(sigc::mem_fun(this, &UnityWindowStyle::OnMonitorChanged));
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

void UnityWindowStyle::OnMonitorChanged(int primary, std::vector<nux::Geometry> const& monitors)
{
  CleanUpUnusedTextures();
}

// Get current in use scale values, if a scaled value is allocated, but
// not in use clean up the scaled textures in unity_window_textures
void UnityWindowStyle::CleanUpUnusedTextures()
{
  unsigned monitors = UScreen::GetDefault()->GetPluggedMonitorsNumber();
  auto& settings = Settings::Instance();
  std::unordered_set<double> used_scales;

  for (unsigned i = 0; i < monitors; ++i)
    used_scales.insert(settings.em(i)->DPIScale());

  for (auto it = unity_window_textures_.begin(); it != unity_window_textures_.end();)
  {
    if (used_scales.find(it->first) == used_scales.end())
    {
      it = unity_window_textures_.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

UnityWindowStyle::Ptr const& UnityWindowStyle::Get()
{
  // This is set only the first time;
  static UnityWindowStyle::Ptr instance(new UnityWindowStyle());
  return instance;
}

int UnityWindowStyle::GetBorderSize(double scale) const
{
  return BORDER_SIZE.CP(scale); // as measured from textures
}

int UnityWindowStyle::GetInternalOffset(double scale) const
{
  return INTERNAL_OFFSET.CP(scale);
}

int UnityWindowStyle::GetCloseButtonPadding(double scale) const
{
  return CLOSE_PADDING.CP(scale);
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
