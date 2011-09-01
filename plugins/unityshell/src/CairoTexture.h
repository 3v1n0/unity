// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2011 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Tim Penhey <tim.penhey@canonical.com>
 *
 */

#ifndef UNITY_CAIROTEXTURE_H
#define UNITY_CAIROTEXTURE_H

#include <NuxImage/CairoGraphics.h>
#include <NuxGraphics/NuxGraphics.h>
#include <NuxGraphics/GLTextureResourceManager.h>

namespace unity
{

inline nux::BaseTexture* texture_from_cairo_graphics(nux::CairoGraphics& cg)
{
  nux::NBitmapData* bitmap = cg.GetBitmap();
  nux::BaseTexture* tex = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableTexture();
  tex->Update(bitmap);
  delete bitmap;
  return tex;
}

}

#endif
