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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

#include "GraphicsUtils.h"
#include <NuxGraphics/GLTextureResourceManager.h>

namespace unity
{
namespace graphics
{

std::stack<nux::ObjectPtr<nux::IOpenGLBaseTexture>> rendering_stack;

void PushOffscreenRenderTarget_(nux::ObjectPtr<nux::IOpenGLBaseTexture> texture)
{  
  int width = texture->GetWidth();
  int height = texture->GetHeight();

  auto graphics_display = nux::GetGraphicsDisplay();
  auto gpu_device = graphics_display->GetGpuDevice();
  gpu_device->FormatFrameBufferObject(width, height, nux::BITFMT_R8G8B8A8);
  gpu_device->SetColorRenderTargetSurface(0, texture->GetSurfaceLevel(0));
  gpu_device->ActivateFrameBuffer();

  auto graphics_engine = graphics_display->GetGraphicsEngine();
  graphics_engine->SetContext(0, 0, width, height);
  graphics_engine->SetViewport(0, 0, width, height);
  graphics_engine->Push2DWindow(width, height);
  graphics_engine->EmptyClippingRegion();
}

void PushOffscreenRenderTarget(nux::ObjectPtr<nux::IOpenGLBaseTexture> texture)
{
  PushOffscreenRenderTarget_(texture);
  rendering_stack.push(texture);
}

void PopOffscreenRenderTarget()
{
  g_assert(rendering_stack.size() > 0);

  rendering_stack.pop();
  if (rendering_stack.size() > 0)
  {
    nux::ObjectPtr<nux::IOpenGLBaseTexture>& texture = rendering_stack.top();
    PushOffscreenRenderTarget_(texture);
  }
  else
  {
    nux::GetWindowCompositor().RestoreRenderingSurface();
  }
}

}
}