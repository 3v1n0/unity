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

UnityWindowStyle::UnityWindowStyle()
{
  background_top_.Adopt(nux::CreateTexture2DFromFile(PKGDATADIR"/switcher_top.png", -1, true));
  background_left_.Adopt(nux::CreateTexture2DFromFile(PKGDATADIR"/switcher_left.png", -1, true));
  background_corner_.Adopt(nux::CreateTexture2DFromFile(PKGDATADIR"/switcher_corner.png", -1, true));
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

UnityWindowStyle::BaseTexturePtr UnityWindowStyle::GetCloseIcon()
{
  if (!close_icon_)
    close_icon_.Adopt(nux::CreateTexture2DFromFile(PKGDATADIR"/dialog_close.png", -1, true));

  return close_icon_;
}

UnityWindowStyle::BaseTexturePtr UnityWindowStyle::GetCloseIconHighligted()
{
  if (!close_icon_highlighted_)
    close_icon_highlighted_.Adopt(nux::CreateTexture2DFromFile(PKGDATADIR"/dialog_close_highlight.png", -1, true));

  return close_icon_highlighted_;
}

UnityWindowStyle::BaseTexturePtr UnityWindowStyle::GetCloseIconPressed()
{
  if (!close_icon_pressed_)
    close_icon_pressed_.Adopt(nux::CreateTexture2DFromFile(PKGDATADIR"/dialog_close_press.png", -1, true));

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
