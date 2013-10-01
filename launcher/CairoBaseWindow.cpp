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
 * Authored by: Marco Trevisan <3v1n0@ubuntu.com>
 */


#include <Nux/Nux.h>
#include <Nux/WindowCompositor.h>

#include "unity-shared/AnimationUtils.h"
#include "unity-shared/CairoTexture.h"
#include "unity-shared/UnitySettings.h"
#include "CairoBaseWindow.h"

namespace unity
{
namespace
{
  const int ANCHOR_WIDTH = 14;
  const int ANCHOR_HEIGHT = 18;
  const int CORNER_RADIUS = 4;
  const int PADDING = 15;
  const int TEXT_PADDING = 8;
  const int MINIMUM_TEXT_WIDTH = 100;
  const int TOP_SIZE = 0;
  const int FADE_DURATION = 80;
}

NUX_IMPLEMENT_OBJECT_TYPE(CairoBaseWindow);

CairoBaseWindow::CairoBaseWindow()
  : use_blurred_background_(!Settings::Instance().GetLowGfxMode())
  , compute_blur_bkg_(use_blurred_background_)
  , fade_animator_(FADE_DURATION)
{
  SetWindowSizeMatchLayout(true);
  sigVisible.connect([this] (BaseWindow*) { compute_blur_bkg_ = true; });

  fade_animator_.updated.connect(sigc::mem_fun(this, &BaseWindow::SetOpacity));
  fade_animator_.finished.connect([this] {
    if (animation::GetDirection(fade_animator_) == animation::Direction::BACKWARD)
      ShowWindow(false);
  });
}

void CairoBaseWindow::Show()
{
  animation::StartOrReverse(fade_animator_, animation::Direction::FORWARD);
  ShowWindow(true);
  PushToFront();
}

void CairoBaseWindow::Hide()
{
  animation::StartOrReverse(fade_animator_, animation::Direction::BACKWARD);
}

bool CairoBaseWindow::HasBlurredBackground() const
{
  return use_blurred_background_;
}

void CairoBaseWindow::NeedSoftRedraw()
{
  compute_blur_bkg_ = true;
  QueueDraw();
}

void CairoBaseWindow::Draw(nux::GraphicsEngine& gfxContext, bool forceDraw)
{
  nux::Geometry base(GetGeometry());

  // Get the background and apply some blur
  if (use_blurred_background_ && compute_blur_bkg_)
  {
    auto current_fbo = nux::GetGraphicsDisplay()->GetGpuDevice()->GetCurrentFrameBufferObject();
    nux::GetWindowCompositor ().RestoreMainFramebuffer();

    gfxContext.SetViewport(0, 0, gfxContext.GetWindowWidth(), gfxContext.GetWindowHeight());
    gfxContext.SetScissor(0, 0, gfxContext.GetWindowWidth(), gfxContext.GetWindowHeight());
    gfxContext.GetRenderStates().EnableScissor(false);

    nux::ObjectPtr <nux::IOpenGLBaseTexture> bkg_texture = gfxContext.CreateTextureFromBackBuffer(base.x, base.y, base.width, base.height);

    nux::TexCoordXForm texxform_bkg;
    bg_blur_texture_ = gfxContext.QRP_GetBlurTexture(0, 0, base.width, base.height, bkg_texture, texxform_bkg, nux::color::White, 1.0f, 3);

    if (current_fbo.IsValid())
    {
      current_fbo->Activate(true);
      gfxContext.Push2DWindow(current_fbo->GetWidth(), current_fbo->GetHeight());
    }
    else
    {
      gfxContext.SetViewport(0, 0, gfxContext.GetWindowWidth(), gfxContext.GetWindowHeight());
      gfxContext.Push2DWindow(gfxContext.GetWindowWidth(), gfxContext.GetWindowHeight());
      gfxContext.ApplyClippingRectangle();
    }
    compute_blur_bkg_ = false;
  }

  // the elements position inside the window are referenced to top-left window
  // corner. So bring base to (0, 0).
  base.SetX(0);
  base.SetY(0);

  gfxContext.PushClippingRectangle(base);

  /*"Clear" out the background. Blending is disabled if blur is disabled. This might need to change, but for the moment both classes
   * which are children of CairoBaseWindow don't have any alpha blending when not using the blurred texture.*/
  nux::ROPConfig rop;
  rop.Blend = use_blurred_background_;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  nux::ColorLayer layer(nux::Color(0x00000000), use_blurred_background_, rop);
  nux::GetPainter().PushDrawLayer(gfxContext, base, &layer);

  nux::TexCoordXForm texxform_bg;
  texxform_bg.SetWrap(nux::TEXWRAP_CLAMP, nux::TEXWRAP_CLAMP);
  texxform_bg.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

  nux::TexCoordXForm texxform_mask;
  texxform_mask.SetWrap(nux::TEXWRAP_CLAMP, nux::TEXWRAP_CLAMP);
  texxform_mask.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

  nux::GetWindowThread()->GetGraphicsEngine().GetRenderStates().SetBlend(true);
  nux::GetWindowThread()->GetGraphicsEngine().GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);

  if (bg_blur_texture_.IsValid() && texture_mask_.IsValid())
  {
    nux::TexCoordXForm texxform_blur_bkg;

    gfxContext.QRP_2TexMod(
      base.x,
      base.y,
      base.width,
      base.height,
      bg_blur_texture_,
      texxform_blur_bkg,
      nux::color::White,
      texture_mask_->GetDeviceTexture(),
      texxform_mask,
      nux::color::White);
  }

  if (texture_bg_.IsValid() && texture_mask_.IsValid())
  {
    gfxContext.QRP_2TexMod(base.x,
                           base.y,
                           base.width,
                           base.height,
                           texture_bg_->GetDeviceTexture(),
                           texxform_bg,
                           nux::color::White,
                           texture_mask_->GetDeviceTexture(),
                           texxform_mask,
                           nux::color::White);
  }

  if (texture_outline_.IsValid())
  {
    nux::TexCoordXForm texxform;
    texxform.SetWrap(nux::TEXWRAP_CLAMP, nux::TEXWRAP_CLAMP);
    texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

    gfxContext.QRP_1Tex(base.x,
                        base.y,
                        base.width,
                        base.height,
                        texture_outline_->GetDeviceTexture(),
                        texxform,
                        nux::color::White);
  }

  nux::GetWindowThread()->GetGraphicsDisplay().GetGraphicsEngine()->GetRenderStates().SetBlend(false);
  nux::GetPainter().PopBackground();
  gfxContext.PopClippingRectangle();
}

} // namespace nux
