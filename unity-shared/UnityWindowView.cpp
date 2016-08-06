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

#include "UnityWindowView.h"
#include <Nux/VLayout.h>
#include "unity-shared/ThemeSettings.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/WindowManager.h"

namespace unity {
namespace ui {

NUX_IMPLEMENT_OBJECT_TYPE(UnityWindowView);

UnityWindowView::UnityWindowView(NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
  , style(UnityWindowStyle::Get())
  , closable(false)
  , monitor(0)
  , scale(Settings::Instance().em()->DPIScale())
  , internal_layout_(nullptr)
  , bg_helper_(this)
{
  live_background.SetGetterFunction([this] { return bg_helper_.enabled(); });
  live_background.SetSetterFunction([this] (bool e) {
    if (bg_helper_.enabled() != e)
    {
      bg_helper_.enabled = e;
      return true;
    }
    return false;
  });

  live_background = false;

  scale.changed.connect([this] (double scale) {
    closable.changed.emit(closable());

    if (internal_layout_)
    {
      int offset = style()->GetInternalOffset().CP(scale);
      view_layout_->SetPadding(offset, offset);
    }
  });

  theme::Settings::Get()->theme.changed.connect(sigc::mem_fun(this, &UnityWindowView::OnThemeChanged));
  Settings::Instance().dpi_changed.connect(sigc::mem_fun(this, &UnityWindowView::OnDPIChanged));
  monitor.changed.connect(sigc::hide(sigc::mem_fun(this, &UnityWindowView::OnDPIChanged)));
  closable.changed.connect(sigc::mem_fun(this, &UnityWindowView::OnClosableChanged));
  background_color.changed.connect(sigc::hide(sigc::mem_fun(this, &View::QueueDraw)));
}

UnityWindowView::~UnityWindowView()
{
  if (close_button_)
    close_button_->UnParentObject();

  if (bounding_area_)
    bounding_area_->UnParentObject();
}

void UnityWindowView::OnDPIChanged()
{
  scale = Settings::Instance().em(monitor())->DPIScale();
}

void UnityWindowView::OnThemeChanged(std::string const&)
{
  closable.changed.emit(closable());
  QueueDraw();
}

void UnityWindowView::SetBackgroundHelperGeometryGetter(BackgroundEffectHelper::GeometryGetterFunc const& func)
{
  bg_helper_.SetGeometryGetter(func);
}

nux::Area* UnityWindowView::FindAreaUnderMouse(const nux::Point& mouse, nux::NuxEventType etype)
{
  if (close_button_ && close_button_->TestMousePointerInclusionFilterMouseWheel(mouse, etype))
    return close_button_.GetPointer();

  nux::Area* under = nux::View::FindAreaUnderMouse(mouse, etype);

  if (under == this)
  {
    if (internal_layout_ && !internal_layout_->TestMousePointerInclusionFilterMouseWheel(mouse, etype))
    {
      if (bounding_area_ && bounding_area_->TestMousePointerInclusionFilterMouseWheel(mouse, etype))
        return bounding_area_.GetPointer();

      return nullptr;
    }
  }

  return under;
}

nux::Area* UnityWindowView::FindKeyFocusArea(unsigned etype, unsigned long key_code, unsigned long modifiers)
{
  if (closable && etype == nux::NUX_KEYDOWN)
  {
    modifiers &= (nux::NUX_STATE_ALT | nux::NUX_STATE_CTRL | nux::NUX_STATE_SUPER | nux::NUX_STATE_SHIFT);
    auto const& close_key = WindowManager::Default().close_window_key();

    if (close_key.first == modifiers && close_key.second == key_code)
    {
      request_close.emit();
      return nullptr;
    }

    if (key_code == NUX_VK_ESCAPE)
    {
      request_close.emit();
      return nullptr;
    }
  }

  return View::FindKeyFocusArea(etype, key_code, modifiers);
}

void UnityWindowView::ReloadCloseButtonTexture()
{
  OnClosableChanged(closable);
}

void UnityWindowView::OnClosableChanged(bool closable)
{
  if (!closable)
  {
    if (close_button_)
    {
      close_button_->UnParentObject();
      close_button_ = nullptr;
    }

    return;
  }

  auto const& texture = style()->GetTexture(scale, WindowTextureType::CLOSE_ICON);
  int padding = style()->GetCloseButtonPadding().CP(scale);

  close_button_ = new IconTexture(texture);
  close_button_->SetBaseXY(padding, padding);
  close_button_->SetParentObject(this);

  close_button_->mouse_enter.connect([this](int, int, unsigned long, unsigned long) {
    if (close_button_->IsMouseOwner())
      close_button_->SetTexture(style()->GetTexture(scale, WindowTextureType::CLOSE_ICON_PRESSED));
    else
      close_button_->SetTexture(style()->GetTexture(scale, WindowTextureType::CLOSE_ICON_HIGHLIGHTED));
  });

  close_button_->mouse_leave.connect([this](int, int, unsigned long, unsigned long) {
    close_button_->SetTexture(style()->GetTexture(scale, WindowTextureType::CLOSE_ICON));
  });

  close_button_->mouse_down.connect([this](int, int, unsigned long, unsigned long) {
    close_button_->SetTexture(style()->GetTexture(scale, WindowTextureType::CLOSE_ICON_PRESSED));
  });

  close_button_->mouse_up.connect([this](int, int, unsigned long, unsigned long) {
    bool inside = close_button_->IsMouseInside();
    if (inside)
      close_button_->SetTexture(style()->GetTexture(scale, WindowTextureType::CLOSE_ICON_HIGHLIGHTED));
    else
      close_button_->SetTexture(style()->GetTexture(scale, WindowTextureType::CLOSE_ICON));
  });

  close_button_->mouse_click.connect([this](int, int, unsigned long, unsigned long) {
    close_button_->SetTexture(style()->GetTexture(scale, WindowTextureType::CLOSE_ICON));
    request_close.emit();
  });

  close_button_->texture_updated.connect(sigc::hide(sigc::mem_fun(this, &UnityWindowView::QueueDraw)));
}

bool UnityWindowView::SetLayout(nux::Layout* layout)
{
  if (layout && layout->IsLayout())
  {
    int offset = style()->GetInternalOffset().CP(scale);

    // We wrap the internal layout adding some padding, so that inherited classes
    // can ignore the offsets we define here.
    nux::ObjectPtr<nux::Layout> wrapper(new nux::VLayout());
    wrapper->SetPadding(offset, offset);
    wrapper->AddLayout(layout);

    if (View::SetLayout(wrapper.GetPointer()))
    {
      internal_layout_ = layout;
      return true;
    }
  }

  return false;
}

nux::Layout* UnityWindowView::GetLayout()
{
  return internal_layout_;
}

nux::Geometry UnityWindowView::GetInternalBackground()
{
  int offset = style()->GetInternalOffset().CP(scale);

  return GetBackgroundGeometry().GetExpand(-offset, -offset);
}

nux::Geometry UnityWindowView::GetBlurredBackgroundGeometry()
{
  return GetBackgroundGeometry();
}

nux::ObjectPtr<nux::InputArea> UnityWindowView::GetBoundingArea()
{
  if (!bounding_area_)
  {
    // The bounding area always matches this size, but only handles events outside
    // the internal layout (when defined)
    bounding_area_ = new nux::InputArea();
    bounding_area_->SetParentObject(this);
    bounding_area_->SetGeometry(GetGeometry());
    geometry_changed.connect([this] (nux::Area*, nux::Geometry const& g) { bounding_area_->SetGeometry(g); });
  }

  return bounding_area_;
}

void UnityWindowView::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  PreDraw(GfxContext, force_draw);
  unsigned push = 0;

  nux::Geometry const& base = GetGeometry();
  GfxContext.PushClippingRectangle(base); ++push;

  // clear region
  gPainter.PaintBackground(GfxContext, base);

  nux::Geometry const& internal_clip = GetInternalBackground();
  GfxContext.PushClippingRectangle(internal_clip); ++push;

  if (BackgroundEffectHelper::blur_type != BLUR_NONE)
  {
    bg_texture_ = bg_helper_.GetBlurRegion();
  }
  else
  {
    bg_texture_ = bg_helper_.GetRegion();
  }

  if (bg_texture_.IsValid())
  {
    nux::Geometry const& bg_geo = GetBlurredBackgroundGeometry();
    nux::TexCoordXForm texxform_blur_bg;
    texxform_blur_bg.flip_v_coord = true;
    texxform_blur_bg.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

    nux::ROPConfig rop;
    rop.Blend = false;
    rop.SrcBlend = GL_ONE;
    rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

#ifndef NUX_OPENGLES_20
    if (GfxContext.UsingGLSLCodePath())
    {
      auto temp_background_color = background_color();
      auto blend_mode = nux::LAYER_BLEND_MODE_OVERLAY;

      if (Settings::Instance().low_gfx() || BackgroundEffectHelper::blur_type == BLUR_NONE)
      {
        temp_background_color.alpha = 1.0f;
        blend_mode = nux::LAYER_BLEND_MODE_NORMAL;
      }

      gPainter.PushDrawCompositionLayer(GfxContext, bg_geo,
                                        bg_texture_,
                                        texxform_blur_bg,
                                        nux::color::White,
                                        temp_background_color,
                                        blend_mode,
                                        true, rop);
    }
    else
      gPainter.PushDrawTextureLayer(GfxContext, bg_geo,
                                    bg_texture_,
                                    texxform_blur_bg,
                                    nux::color::White,
                                    true,
                                    rop);
#else
      gPainter.PushDrawCompositionLayer(GfxContext, bg_geo,
                                        bg_texture_,
                                        texxform_blur_bg,
                                        nux::color::White,
                                        background_color, nux::LAYER_BLEND_MODE_OVERLAY,
                                        true, rop);
#endif
  }

  nux::ROPConfig rop;
  rop.Blend = true;

#ifndef NUX_OPENGLES_20
  if (GfxContext.UsingGLSLCodePath() == false)
  {
    rop.SrcBlend = GL_ONE;
    rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
    gPainter.PushDrawColorLayer (GfxContext, internal_clip, background_color, false, rop);
  }
#endif

  // Make round corners
  rop.Blend = true;
  rop.SrcBlend = GL_ZERO;
  rop.DstBlend = GL_SRC_ALPHA;
  gPainter.PaintShapeCornerROP(GfxContext,
                               internal_clip,
                               nux::color::White,
                               nux::eSHAPE_CORNER_ROUND4,
                               nux::eCornerTopLeft | nux::eCornerTopRight |
                               nux::eCornerBottomLeft | nux::eCornerBottomRight,
                               true,
                               rop);

  DrawOverlay(GfxContext, force_draw, internal_clip);

  for (unsigned i = 0; i < push; ++i)
    GfxContext.PopClippingRectangle();

  DrawBackground(GfxContext, GetBackgroundGeometry());

  if (close_button_)
  {
    nux::GetPainter().PushPaintLayerStack();
    GfxContext.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // Just doing ProcessDraw on the area should be enough, but unfortunately
    // this leads to a flickering close icon that is randomly drawn.
    // close_button_->ProcessDraw(GfxContext, force_draw);

    nux::TexCoordXForm texxform;
    auto const& geo = close_button_->GetGeometry();
    GfxContext.QRP_1Tex(geo.x, geo.y, geo.width, geo.height,
                        close_button_->texture()->GetDeviceTexture(),
                        texxform, nux::color::White);

    GfxContext.GetRenderStates().SetBlend(false);
    nux::GetPainter().PopPaintLayerStack();
  }
}

void UnityWindowView::DrawBackground(nux::GraphicsEngine& GfxContext, nux::Geometry const& geo)
{
  int border = style()->GetBorderSize().CP(scale);
  auto background_corner_textrue = style()->GetTexture(scale, WindowTextureType::BORDER_CORNER)->GetDeviceTexture();

  GfxContext.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  nux::TexCoordXForm texxform;
  texxform.SetTexCoordType (nux::TexCoordXForm::OFFSET_COORD);
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

  // Draw TOP-LEFT CORNER
  texxform.u0 = 0;
  texxform.v0 = 0;
  texxform.u1 = border;
  texxform.v1 = border;
  GfxContext.QRP_1Tex (geo.x, geo.y,
                       border, border, background_corner_textrue, texxform, nux::color::White);

  // Draw TOP-RIGHT CORNER
  texxform.u0 = 0;
  texxform.v0 = 0;
  texxform.u1 = border;
  texxform.v1 = border;
  texxform.flip_u_coord = true;
  texxform.flip_v_coord = false;
  GfxContext.QRP_1Tex (geo.x + geo.width - border, geo.y,
                       border, border, background_corner_textrue, texxform, nux::color::White);

  // Draw BOTTOM-LEFT CORNER
  texxform.u0 = 0;
  texxform.v0 = 0;
  texxform.u1 = border;
  texxform.v1 = border;
  texxform.flip_u_coord = false;
  texxform.flip_v_coord = true;
  GfxContext.QRP_1Tex (geo.x, geo.y + geo.height - border,
                       border, border, background_corner_textrue, texxform, nux::color::White);

  // Draw BOTTOM-RIGHT CORNER
  texxform.u0 = 0;
  texxform.v0 = 0;
  texxform.u1 = border;
  texxform.v1 = border;
  texxform.flip_u_coord = true;
  texxform.flip_v_coord = true;
  GfxContext.QRP_1Tex (geo.x + geo.width - border, geo.y + geo.height - border,
                       border, border, background_corner_textrue, texxform, nux::color::White);

  auto background_top = style()->GetTexture(scale, WindowTextureType::BORDER_TOP);
  int top_width  = background_top->GetWidth();
  int top_height = background_top->GetHeight();

  // Draw TOP BORDER
  texxform.u0 = 0;
  texxform.v0 = 0;
  texxform.u1 = top_width;
  texxform.v1 = top_height;
  texxform.flip_u_coord = false;
  texxform.flip_v_coord = false;
  GfxContext.QRP_1Tex (geo.x + border, geo.y, geo.width - border - border, border, background_top->GetDeviceTexture(), texxform, nux::color::White);

  // Draw BOTTOM BORDER
  texxform.u0 = 0;
  texxform.v0 = 0;
  texxform.u1 = top_width;
  texxform.v1 = top_height;
  texxform.flip_u_coord = false;
  texxform.flip_v_coord = true;
  GfxContext.QRP_1Tex (geo.x + border, geo.y + geo.height - border, geo.width - border - border, border, background_top->GetDeviceTexture(), texxform, nux::color::White);

  auto background_left = style()->GetTexture(scale, WindowTextureType::BORDER_LEFT);
  int left_width  = background_left->GetWidth();
  int left_height = background_left->GetHeight();

  // Draw LEFT BORDER
  texxform.u0 = 0;
  texxform.v0 = 0;
  texxform.u1 = left_width;
  texxform.v1 = left_height;
  texxform.flip_u_coord = false;
  texxform.flip_v_coord = false;
  GfxContext.QRP_1Tex (geo.x, geo.y + border, border, geo.height - border - border, background_left->GetDeviceTexture(), texxform, nux::color::White);

  // Draw RIGHT BORDER
  texxform.u0 = 0;
  texxform.v0 = 0;
  texxform.u1 = left_width;
  texxform.v1 = left_height;
  texxform.flip_u_coord = true;
  texxform.flip_v_coord = false;
  GfxContext.QRP_1Tex (geo.x + geo.width - border, geo.y + border, border, geo.height - border - border, background_left->GetDeviceTexture(), texxform, nux::color::White);

  GfxContext.GetRenderStates().SetBlend(false);
}

// Introspectable methods
std::string UnityWindowView::GetName() const
{
  return "UnityWindowView";
}

void UnityWindowView::AddProperties(debug::IntrospectionData& introspection)
{
  introspection
    .add("bg-texture-is-valid", bg_texture_.IsValid())
    .add("closable", closable())
    .add("close_geo", close_button_ ? close_button_->GetGeometry() : nux::Geometry());
}


}
}
