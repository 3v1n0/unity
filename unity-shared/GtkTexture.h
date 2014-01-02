// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2013 Canonical Ltd.
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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 *
 */

#ifndef UNITY_GDKTEXTURE_H
#define UNITY_GDK_TEXTURE_H

#include <Nux/Nux.h>
#include <NuxGraphics/GdkGraphics.h>
#include <NuxGraphics/NuxGraphics.h>
#include <NuxGraphics/GLTextureResourceManager.h>

namespace unity
{

typedef nux::ObjectPtr<nux::BaseTexture> BaseTexturePtr;

// Create a texture from the GdkGraphics object.
//
// Returns a new BaseTexture that has a ref count of 1.
inline nux::BaseTexture* texture_from_gdk_graphics(nux::GdkGraphics const& cg)
{
  nux::NBitmapData* bitmap = cg.GetBitmap();
  nux::BaseTexture* tex = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableTexture();
  tex->Update(bitmap);
  delete bitmap;
  return tex;
}

// Create a texture from the GdkGraphics object.
//
// Returns a new smart pointer to a texture where that smart pointer is the
// sole owner of the texture object.
inline BaseTexturePtr texture_ptr_from_gdk_graphics(nux::GdkGraphics const& cg)
{
  BaseTexturePtr result(texture_from_gdk_graphics(cg));
  // Since the ObjectPtr takes a reference, and the texture is initially
  // owned, the reference count now is two.
  nuxAssert(result->GetReferenceCount() == 2);
  result->UnReference();
  return result;
}

}

#endif
