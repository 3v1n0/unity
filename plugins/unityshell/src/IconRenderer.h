// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonical Ltd
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
 */

#ifndef ICONRENDERER_H
#define ICONRENDERER_H

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <Nux/View.h>

#include "AbstractLauncherIcon.h"

namespace unity {
namespace ui {

class IconRenderer
{
public:
  IconRenderer();
  virtual ~IconRenderer();

  void RenderIcon (nux::GraphicsEngine& GfxContext, RenderArg const &arg, nux::Geometry geo);

protected:
  void RenderElement (nux::GraphicsEngine& GfxContext,
                      RenderArg const &arg,
                      nux::IntrusiveSP<nux::IOpenGLBaseTexture> icon,
                      nux::Color bkg_color,
                      float alpha,
                      nux::Vector4 xform_coords[]);
};

}
}

#endif // ICONRENDERER_H

