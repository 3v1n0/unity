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

#include "ThemeSettings.h"
#include "UnitySettings.h"
#include "UScreen.h"
#include "config.h"

#include <unordered_set>

namespace unity
{
namespace ui
{
namespace
{
  const std::string DIALOG_BORDER_TOP     = "dialog_border_top";
  const std::string DIALOG_BORDER_LEFT    = "dialog_border_left";
  const std::string DIALOG_BORDER_CORNER  = "dialog_border_corner";

  const std::string DIALOG_CLOSE     = "dialog_close";
  const std::string DIALOG_HIGHLIGHT = "dialog_close_highlight";
  const std::string DIALOG_PRESS     = "dialog_close_press";

  const RawPixel BORDER_SIZE     = 30_em; // as measured from textures
  const RawPixel INTERNAL_OFFSET = 20_em;
  const RawPixel CLOSE_PADDING   =  3_em;

  DECLARE_LOGGER(logger, "unity.ui.window.style");
}

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
  theme::Settings::Get()->font.changed.connect(sigc::mem_fun(this, &UnityWindowStyle::OnThemeChanged));
}

void UnityWindowStyle::LoadAllTextureInScale(double scale)
{
  auto& window_textures = unity_window_textures_[scale];

  window_textures[unsigned(WindowTextureType::BORDER_TOP)]    = LoadTexture(DIALOG_BORDER_TOP, scale);
  window_textures[unsigned(WindowTextureType::BORDER_LEFT)]   = LoadTexture(DIALOG_BORDER_LEFT, scale);
  window_textures[unsigned(WindowTextureType::BORDER_CORNER)] = LoadTexture(DIALOG_BORDER_CORNER, scale);

  window_textures[unsigned(WindowTextureType::CLOSE_ICON)]             = LoadTexture(DIALOG_CLOSE, scale);
  window_textures[unsigned(WindowTextureType::CLOSE_ICON_HIGHLIGHTED)] = LoadTexture(DIALOG_HIGHLIGHT, scale);
  window_textures[unsigned(WindowTextureType::CLOSE_ICON_PRESSED)]     = LoadTexture(DIALOG_PRESS, scale);
}

nux::BaseTexture* UnityWindowStyle::LoadTexture(std::string const& texture_name, double scale) const
{
  auto const& texture_path = theme::Settings::Get()->ThemedFilePath(texture_name, {PKGDATADIR});
  RawPixel max_size = GetDefaultMaxTextureSize(texture_path);
  return nux::CreateTexture2DFromFile(texture_path.c_str(), max_size.CP(scale), true);
}

RawPixel UnityWindowStyle::GetDefaultMaxTextureSize(std::string const& texture_path) const
{
  nux::Size size;
  gdk_pixbuf_get_file_info(texture_path.c_str(), &size.width, &size.height);
  RawPixel max_size = std::max(std::round(size.width), std::round(size.height));

  return max_size;
}

void UnityWindowStyle::OnMonitorChanged(int primary, std::vector<nux::Geometry> const& monitors)
{
  CleanUpUnusedTextures();
}

void UnityWindowStyle::OnThemeChanged(std::string const&)
{
  unity_window_textures_.clear();
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

RawPixel UnityWindowStyle::GetBorderSize() const
{
  return BORDER_SIZE;
}

RawPixel UnityWindowStyle::GetInternalOffset() const
{
  return INTERNAL_OFFSET;
}

RawPixel UnityWindowStyle::GetCloseButtonPadding() const
{
  return CLOSE_PADDING;
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
      return BaseTexturePtr();
    }
  }

  return it->second[unsigned(type)];
}

} // namespace ui
} // namespace unity
