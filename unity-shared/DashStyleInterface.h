// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#ifndef UNITYSHELL_DASH_STYLE_INTERFACE_H
#define UNITYSHELL_DASH_STYLE_INTERFACE_H

#include <memory>

namespace nux {
  class AbstractPaintLayer;
  class BaseTexture;
}

namespace unity {
namespace dash {

class StyleInterface
{
public:
  virtual ~StyleInterface() {};

  virtual nux::AbstractPaintLayer* FocusOverlay(int width, int height) = 0;

  virtual nux::BaseTexture* GetCategoryBackground() = 0;
  virtual nux::BaseTexture* GetCategoryBackgroundNoFilters() = 0;

  virtual nux::BaseTexture* GetGroupUnexpandIcon() = 0;
  virtual nux::BaseTexture* GetGroupExpandIcon() = 0;

  virtual int GetCategoryIconSize() const = 0;
  virtual int GetCategoryHeaderLeftPadding() const = 0;
  virtual int GetPlacesGroupTopSpace() const = 0;
  virtual int GetPlacesGroupResultTopPadding() const = 0;
  virtual int GetPlacesGroupResultLeftPadding() const = 0;  
};

}
}

#endif
