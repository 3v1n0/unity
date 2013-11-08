// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2012-2013 Canonical Ltd.
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
 * Authored by: Diego Sarmentero <diego.sarmentero@canonical.com>
 *              Manuel de la Pena <manuel.delapena@canonical.com>
 *
 */
#include <NuxCore/Logger.h>
#include "PaymentPreview.h"
#include "unity-shared/CoverArt.h"
#include "unity-shared/PreviewStyle.h"

namespace unity
{

namespace dash
{

namespace previews
{

namespace
{

nux::logging::Logger logger("unity.dash.previews.payment.preview");

}

class OverlaySpinner : public unity::debug::Introspectable, public nux::View
{
  NUX_DECLARE_OBJECT_TYPE(OverlaySpinner, nux::View);
public:
  OverlaySpinner();

  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);

protected:
  // Introspectable methods
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

  // Key navigation
  virtual bool AcceptKeyNavFocus();

private:
  bool OnFrameTimeout();

  nux::BaseTexture* spin_;

  glib::Source::UniquePtr frame_timeout_;

  nux::Matrix4 rotate_;
  float rotation_;
};

NUX_IMPLEMENT_OBJECT_TYPE(OverlaySpinner);

OverlaySpinner::OverlaySpinner()
  : nux::View(NUX_TRACKER_LOCATION),
    rotation_(0.0f)
{
  previews::Style& style = dash::previews::Style::Instance();

  spin_ = style.GetSearchSpinIcon();

  rotate_.Identity();
  rotate_.Rotate_z(0.0);
}

void OverlaySpinner::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();
  nux::TexCoordXForm texxform;

  GfxContext.PushClippingRectangle(geo);

  nux::GetPainter().PaintBackground(GfxContext, geo);

  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  texxform.min_filter = nux::TEXFILTER_LINEAR;
  texxform.mag_filter = nux::TEXFILTER_LINEAR;

  unsigned int current_alpha_blend;
  unsigned int current_src_blend_factor;
  unsigned int current_dest_blend_factor;
  GfxContext.GetRenderStates().GetBlend(current_alpha_blend, current_src_blend_factor, current_dest_blend_factor);
  GfxContext.GetRenderStates().SetBlend(true,  GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  nux::Geometry spin_geo(geo.x + ((geo.width - spin_->GetWidth()) / 2),
                         geo.y + ((geo.height - spin_->GetHeight()) / 2),
                         spin_->GetWidth(),
                         spin_->GetHeight());
  // Geometry (== Rect) uses integers which were rounded above,
  // hence an extra 0.5 offset for odd sizes is needed
  // because pure floating point is not being used.
  int spin_offset_w = !(geo.width % 2) ? 0 : 1;
  int spin_offset_h = !(geo.height % 2) ? 0 : 1;

  nux::Matrix4 matrix_texture;
  matrix_texture = nux::Matrix4::TRANSLATE(-spin_geo.x - (spin_geo.width + spin_offset_w) / 2.0f,
                                          -spin_geo.y - (spin_geo.height + spin_offset_h) / 2.0f, 0) * matrix_texture;
  matrix_texture = rotate_ * matrix_texture;
  matrix_texture = nux::Matrix4::TRANSLATE(spin_geo.x + (spin_geo.width + spin_offset_w) / 2.0f,
                                             spin_geo.y + (spin_geo.height + spin_offset_h) / 2.0f, 0) * matrix_texture;

  GfxContext.SetModelViewMatrix(GfxContext.GetModelViewMatrix() * matrix_texture);

  GfxContext.QRP_1Tex(spin_geo.x,
                      spin_geo.y,
                      spin_geo.width,
                      spin_geo.height,
                      spin_->GetDeviceTexture(),
                      texxform,
                      nux::color::White);

  // revert to model view matrix stack
  GfxContext.ApplyModelViewMatrix();

  GfxContext.PopClippingRectangle();

  GfxContext.GetRenderStates().SetBlend(current_alpha_blend, current_src_blend_factor, current_dest_blend_factor);

  if (!frame_timeout_)
  {
    frame_timeout_.reset(new glib::Timeout(22, sigc::mem_fun(this, &OverlaySpinner::OnFrameTimeout)));
  }
}


void OverlaySpinner::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
}


bool OverlaySpinner::OnFrameTimeout()
{
  rotation_ += 0.1f;

  if (rotation_ >= 360.0f)
    rotation_ = 0.0f;

  rotate_.Rotate_z(rotation_);
  QueueDraw();

  frame_timeout_.reset();
  return false;
}

std::string OverlaySpinner::GetName() const
{
  return "OverlaySpinner";
}

void OverlaySpinner::AddProperties(GVariantBuilder* builder)
{
  variant::BuilderWrapper(builder).add(GetAbsoluteGeometry());
}


bool OverlaySpinner::AcceptKeyNavFocus()
{
  return false;
}

PaymentPreview::PaymentPreview(dash::Preview::Ptr preview_model)
: Preview(preview_model)
, data_(nullptr)
, full_data_layout_(nullptr)
{
}

std::string PaymentPreview::GetName() const
{
  return "PaymentPreview";
}

void PaymentPreview::AddProperties(GVariantBuilder* builder)
{
  Preview::AddProperties(builder);
}

nux::Layout* PaymentPreview::GetHeader()
{
  nux::HLayout* header_data_layout = new nux::HLayout();
  header_data_layout->SetSpaceBetweenChildren(10);
  header_data_layout->SetMaximumHeight(76);
  header_data_layout->SetMinimumHeight(76);

  image_ = new CoverArt();
  image_->SetMinMaxSize(64, 64);
  AddChild(image_.GetPointer());
  UpdateCoverArtImage(image_.GetPointer());

  header_data_layout->AddView(image_.GetPointer(), 0);
  header_data_layout->AddLayout(GetTitle(), 0);
  header_data_layout->AddSpace(10, 1);
  header_data_layout->AddLayout(GetPrice(), 0);
  return header_data_layout;
}

nux::ObjectPtr<ActionLink> PaymentPreview::CreateLink(dash::Preview::ActionPtr action)
{
  previews::Style& style = dash::previews::Style::Instance();

  nux::ObjectPtr<ActionLink> link;
  link = new ActionLink(action->id,
         action->display_name, NUX_TRACKER_LOCATION);
  link->font_hint.Set(style.payment_form_labels_font().c_str());
  link->SetMinimumWidth(178);
  link->SetMaximumHeight(34);
  return link;
}


nux::ObjectPtr<ActionButton> PaymentPreview::CreateButton(dash::Preview::ActionPtr action)
{
  previews::Style& style = dash::previews::Style::Instance();

  nux::ObjectPtr<ActionButton> button;
  button = new ActionButton(action->id,
           action->display_name, action->icon_hint,
           NUX_TRACKER_LOCATION);
  button->SetFont(style.action_font());
  button->SetExtraHint(action->extra_text, style.action_extra_font());
  button->SetMinimumWidth(178);
  button->SetMaximumHeight(34);
  return button;
}


void PaymentPreview::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();

  gfx_engine.PushClippingRectangle(base);
  nux::GetPainter().PaintBackground(gfx_engine, base);

  if (full_data_layout_)
  {
    unsigned int alpha, src, dest = 0;
    gfx_engine.GetRenderStates().GetBlend(alpha, src, dest);
    gfx_engine.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    details_bg_layer_->SetGeometry(full_data_layout_->GetGeometry());
    nux::GetPainter().RenderSinglePaintLayer(gfx_engine, full_data_layout_->GetGeometry(), details_bg_layer_.get());

    gfx_engine.GetRenderStates().SetBlend(alpha, src, dest);
  }

  gfx_engine.PopClippingRectangle();
}

void PaymentPreview::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
    nux::Geometry const& base = GetGeometry();
    gfx_engine.PushClippingRectangle(base);

    if (!IsFullRedraw())
      nux::GetPainter().PushLayer(gfx_engine, details_bg_layer_->GetGeometry(), details_bg_layer_.get());

    unsigned int alpha, src, dest = 0;
    gfx_engine.GetRenderStates().GetBlend(alpha, src, dest);
    gfx_engine.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    if (GetCompositionLayout())
      GetCompositionLayout()->ProcessDraw(gfx_engine, force_draw);

    gfx_engine.GetRenderStates().SetBlend(alpha, src, dest);

    if (!IsFullRedraw())
      nux::GetPainter().PopBackground();

    gfx_engine.PopClippingRectangle();
}

void PaymentPreview::ShowOverlay(bool isShown)
{
  if (!full_data_layout_)
    return;

  if (isShown)
  {
    full_data_layout_->SetActiveLayerN(1);
  }
  else
  {
    full_data_layout_->SetActiveLayerN(0);
  }
  QueueDraw();
}

void PaymentPreview::ShowOverlay()
{
  ShowOverlay(true);
}

void PaymentPreview::HideOverlay()
{
  ShowOverlay(false);
}

void PaymentPreview::SetupBackground()
{
  details_bg_layer_.reset(dash::previews::Style::Instance().GetBackgroundLayer());
}

void PaymentPreview::SetupViews()
{
  full_data_layout_ = new nux::LayeredLayout();

  // layout to be used to show the info
  content_data_layout_ = new nux::VLayout();
  content_data_layout_->SetSpaceBetweenChildren(5);
  content_data_layout_->SetPadding(10, 10, 0, 10);

  header_layout_ = GetHeader();

  content_data_layout_->AddLayout(header_layout_.GetPointer(), 1);

  body_layout_ = GetBody();
  content_data_layout_->AddLayout(body_layout_.GetPointer(), 1);

  footer_layout_ = GetFooter();
  content_data_layout_->AddLayout(footer_layout_.GetPointer(), 1);

  full_data_layout_->AddLayout(content_data_layout_.GetPointer());

  // layout to draw an overlay
  overlay_layout_ = new nux::VLayout();
  StaticCairoText* calculating = new StaticCairoText(
                                   "Performing purchase", true,
                                   NUX_TRACKER_LOCATION);

  OverlaySpinner* spinner_ = new OverlaySpinner();
  overlay_layout_->AddSpace(20, 1);
  overlay_layout_->AddView(calculating, 0, nux::MINOR_POSITION_CENTER);
  overlay_layout_->AddView(spinner_, 1, nux::MINOR_POSITION_CENTER);
  overlay_layout_->AddSpace(20, 1);

  full_data_layout_->AddLayout(overlay_layout_.GetPointer());

  SetLayout(full_data_layout_.GetPointer());
}

}

}

}
