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
 */

#include "UnityWindowStyle.h"
#include "config.h"

namespace unity {
namespace ui {
namespace
{
  const char* const SWITCHER_TOP    = PKGDATADIR"/switcher_top.png";
  const char* const SWITCHER_LEFT   = PKGDATADIR"/switcher_left.png";
  const char* const SWITCHER_CORNER = PKGDATADIR"/switcher_corner.png";

  const char* const DIALOG_CLOSE     = PKGDATADIR"/dialog_close.png";
  const char* const DIALOG_HIGHLIGHT = PKGDATADIR"/dialog_close_highlight.png";
  const char* const DIALOG_PRESS     = PKGDATADIR"/dialog_close_press.png";
}

UnityWindowStyle::UnityWindowStyle()
  : scale(1.0)
{
  ReloadIcons();

  scale.changed.connect([this] (double new_scale) {
    ReloadIcons();
  });
}

void UnityWindowStyle::ReloadIcons()
{
  background_top_.Adopt(LoadTexture(SWITCHER_TOP));
  background_left_.Adopt(LoadTexture(SWITCHER_LEFT));
  background_corner_.Adopt(LoadTexture(SWITCHER_CORNER));

  close_icon_.Adopt(LoadTexture(DIALOG_CLOSE));
  close_icon_highlighted_.Adopt(LoadTexture(DIALOG_HIGHLIGHT));
  close_icon_pressed_.Adopt(LoadTexture(DIALOG_PRESS));
}

nux::BaseTexture* UnityWindowStyle::LoadTexture(const char* const texture_name) const
{
  RawPixel max_size = GetDefaultMaxTextureSize(texture_name);
  return nux::CreateTexture2DFromFile(texture_name, max_size.CP(scale), true);
}

RawPixel UnityWindowStyle::GetDefaultMaxTextureSize(const char* const texture_name) const
{
  nux::Size size;
  gdk_pixbuf_get_file_info(texture_name, &size.width, &size.height);
  RawPixel max_size = std::max(std::round(size.width * scale), std::round(size.height * scale));

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

UnityWindowStyle::BaseTexturePtr UnityWindowStyle::GetCloseIcon() const
{
  return close_icon_;
}

UnityWindowStyle::BaseTexturePtr UnityWindowStyle::GetCloseIconHighligted() const
{
  return close_icon_highlighted_;
}

UnityWindowStyle::BaseTexturePtr UnityWindowStyle::GetCloseIconPressed() const
{
  return close_icon_pressed_;
}

UnityWindowStyle::BaseTexturePtr UnityWindowStyle::GetBackgroundTop() const
{
  return background_top_;
}

UnityWindowStyle::BaseTexturePtr UnityWindowStyle::GetBackgroundLeft() const
{
  return background_left_;
}

UnityWindowStyle::BaseTexturePtr UnityWindowStyle::GetBackgroundCorner() const
{
  return background_corner_;
}


}
}
