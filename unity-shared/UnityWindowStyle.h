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

#ifndef UNITYWINDOWSTYLE_H
#define UNITYWINDOWSTYLE_H

#include <sigc++/sigc++.h>
#include <Nux/Nux.h>

namespace unity {
namespace ui {

class UnityWindowStyle
{
public:
  typedef std::shared_ptr<UnityWindowStyle> Ptr;
  typedef nux::ObjectPtr<nux::BaseTexture> BaseTexturePtr;

  static UnityWindowStyle::Ptr Get();

  BaseTexturePtr GetCloseIcon();
  BaseTexturePtr GetCloseIconHighligted();
  BaseTexturePtr GetCloseIconPressed();
  int GetCloseButtonPadding() const;

  BaseTexturePtr GetBackgroundTop() const;
  BaseTexturePtr GetBackgroundLeft() const;
  BaseTexturePtr GetBackgroundCorner() const;
  int GetBorderSize() const;
  int GetInternalOffset() const;

private:
  UnityWindowStyle();

  BaseTexturePtr background_top_;
  BaseTexturePtr background_left_;
  BaseTexturePtr background_corner_;
  BaseTexturePtr close_icon_;
  BaseTexturePtr close_icon_highlighted_;
  BaseTexturePtr close_icon_pressed_;
};

}
}

#endif
