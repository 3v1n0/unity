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
#include "unity-shared/RawPixel.h"

namespace nux {
  class AbstractPaintLayer;
  class BaseTexture;
  template <class T> class ObjectPtr;
}

namespace unity {
namespace dash {

class StyleInterface
{
public:
  virtual ~StyleInterface() {};

  virtual nux::AbstractPaintLayer* FocusOverlay(int width, int height) = 0;

  virtual nux::ObjectPtr<nux::BaseTexture> const& GetCategoryBackground() const = 0;
  virtual nux::ObjectPtr<nux::BaseTexture> const& GetCategoryBackgroundNoFilters() const = 0;

  virtual nux::ObjectPtr<nux::BaseTexture> const& GetGroupUnexpandIcon() const = 0;
  virtual nux::ObjectPtr<nux::BaseTexture> const& GetGroupExpandIcon() const = 0;

  virtual RawPixel GetCategoryIconSize() const = 0;
  virtual RawPixel GetCategoryHeaderLeftPadding() const = 0;
  virtual RawPixel GetPlacesGroupTopSpace() const = 0;
  virtual RawPixel GetPlacesGroupResultTopPadding() const = 0;
  virtual RawPixel GetPlacesGroupResultLeftPadding() const = 0;
};

}
}

#endif
