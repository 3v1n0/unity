// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 */


#include "OverlayRenderer.h"

#include <NuxGraphics/GraphicsEngine.h>
#include <Nux/PaintLayer.h>
#include <NuxCore/Logger.h>

#include "BackgroundEffectHelper.h"
#include "DashStyle.h"
#include "DashSettings.h"

#include "UBusMessages.h"
#include "UBusWrapper.h"

namespace unity 
{
namespace
{
nux::logging::Logger logger("unity.overlayrenderer");

const int INNER_CORNER_RADIUS = 5;
}

// Impl class
class OverlayRendererImpl
{
public:
  OverlayRendererImpl(OverlayRenderer *parent_);
  ~OverlayRendererImpl();
  
  void Init();
  void OnBackgroundColorChanged(GVariant* args);
  
  void Draw(nux::GraphicsEngine& gfx_context, nux::Geometry content_geo, nux::Geometry absolute_geo, nux::Geometry geometry, bool force_draw);
  void DrawContent(nux::GraphicsEngine& gfx_context, nux::Geometry content_geo, nux::Geometry absolute_geo, nux::Geometry geometry);
  void DrawContentCleanup(nux::GraphicsEngine& gfx_context, nux::Geometry content_geo, nux::Geometry absolute_geo, nux::Geometry geometry);
  
  BackgroundEffectHelper bg_effect_helper_;
  nux::ColorLayer* bg_layer_;
  nux::ColorLayer* bg_darken_layer_;
  nux::Color bg_color_;
  
  nux::Geometry content_geo;
  nux::ObjectPtr <nux::IOpenGLBaseTexture> bg_blur_texture_;
  nux::ObjectPtr <nux::IOpenGLBaseTexture> bg_shine_texture_;
  
  // temporary variable that stores the number of backgrounds we have rendered
  int bgs;
  bool visible;
  
  UBusManager ubus_manager_;
  
  OverlayRenderer *parent;
};

OverlayRendererImpl::OverlayRendererImpl(OverlayRenderer *parent_)
  : visible(false)
  , parent(parent_)
{
  bg_effect_helper_.enabled = false;
  Init();
}

OverlayRendererImpl::~OverlayRendererImpl()
{
  delete bg_layer_;
  delete bg_darken_layer_;
}

void OverlayRendererImpl::Init()
{
  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  bg_layer_ = new nux::ColorLayer(nux::Color(0.0f, 0.0f, 0.0f, 0.9), true, rop);
  
  rop.Blend = true;
  rop.SrcBlend = GL_ZERO;
  rop.DstBlend = GL_SRC_COLOR;
  bg_darken_layer_ = new nux::ColorLayer(nux::Color(0.7f, 0.7f, 0.7f, 1.0f), false, rop);
  bg_shine_texture_ = unity::dash::Style::Instance().GetDashShine()->GetDeviceTexture();
  
  ubus_manager_.RegisterInterest(UBUS_BACKGROUND_COLOR_CHANGED,
                                 sigc::mem_fun(this, &OverlayRendererImpl::OnBackgroundColorChanged));
  
  ubus_manager_.SendMessage(UBUS_BACKGROUND_REQUEST_COLOUR_EMIT);
}

void OverlayRendererImpl::OnBackgroundColorChanged(GVariant* args)
{
  gdouble red, green, blue, alpha;
  g_variant_get (args, "(dddd)", &red, &green, &blue, &alpha);

  nux::Color color = nux::Color(red, green, blue, alpha);
  bg_layer_->SetColor(color);
  bg_color_ = color;
  
  parent->need_redraw.emit();
}

void OverlayRendererImpl::Draw(nux::GraphicsEngine& gfx_context, nux::Geometry content_geo, nux::Geometry absolute_geo, nux::Geometry geometry, bool force_edges)
{
  bool paint_blur = BackgroundEffectHelper::blur_type != BLUR_NONE;
  nux::Geometry geo = content_geo;

  if (dash::Settings::Instance().GetFormFactor() != dash::FormFactor::NETBOOK || force_edges)
  {
    // Paint the edges
    {
      dash::Style& style = dash::Style::Instance();
      nux::BaseTexture* bottom = style.GetDashBottomTile();
      nux::BaseTexture* right = style.GetDashRightTile();
      nux::BaseTexture* corner = style.GetDashCorner();
      nux::BaseTexture* corner_mask = style.GetDashCornerMask();
      nux::BaseTexture* left_corner = style.GetDashLeftCorner();
      nux::BaseTexture* left_corner_mask = style.GetDashLeftCornerMask();
      nux::BaseTexture* left_tile = style.GetDashLeftTile();
      nux::BaseTexture* top_corner = style.GetDashTopCorner();
      nux::BaseTexture* top_corner_mask = style.GetDashTopCornerMask();
      nux::BaseTexture* top_tile = style.GetDashTopTile();
      nux::TexCoordXForm texxform;

      int left_corner_offset = 10;
      int top_corner_offset = 10;

      geo.width += corner->GetWidth() - 10;
      geo.height += corner->GetHeight() - 10;
      {
        // Corner
        texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
        texxform.SetWrap(nux::TEXWRAP_CLAMP_TO_BORDER, nux::TEXWRAP_CLAMP_TO_BORDER);

        gfx_context.QRP_1Tex(geo.x + (geo.width - corner->GetWidth()),
                             geo.y + (geo.height - corner->GetHeight()),
                             corner->GetWidth(),
                             corner->GetHeight(),
                             corner->GetDeviceTexture(),
                             texxform,
                             nux::color::White);
      }
      {
        // Bottom repeated texture
        int real_width = geo.width - (left_corner->GetWidth() - left_corner_offset) - corner->GetWidth();
        int offset = real_width % bottom->GetWidth();

        texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
        texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

        gfx_context.QRP_1Tex(left_corner->GetWidth() - left_corner_offset - offset,
                             geo.y + (geo.height - bottom->GetHeight()),
                             real_width + offset,
                             bottom->GetHeight(),
                             bottom->GetDeviceTexture(),
                             texxform,
                             nux::color::White);
      }
      {
        // Bottom left corner
        texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
        texxform.SetWrap(nux::TEXWRAP_CLAMP_TO_BORDER, nux::TEXWRAP_CLAMP_TO_BORDER);

        gfx_context.QRP_1Tex(geo.x - left_corner_offset,
                             geo.y + (geo.height - left_corner->GetHeight()),
                             left_corner->GetWidth(),
                             left_corner->GetHeight(),
                             left_corner->GetDeviceTexture(),
                             texxform,
                             nux::color::White);
      }
      {
        // Left repeated texture
        nux::Geometry real_geo = geometry;
        int real_height = real_geo.height - geo.height;
        int offset = real_height % left_tile->GetHeight();

        texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
        texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

        gfx_context.QRP_1Tex(geo.x - 10,
                             geo.y + geo.height - offset,
                             left_tile->GetWidth(),
                             real_height + offset,
                             left_tile->GetDeviceTexture(),
                             texxform,
                             nux::color::White);
      }
      {
        // Right edge
        texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
        texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

        gfx_context.QRP_1Tex(geo.x + geo.width - right->GetWidth(),
                             geo.y + top_corner->GetHeight() - top_corner_offset,
                             right->GetWidth(),
                             geo.height - corner->GetHeight() - (top_corner->GetHeight() - top_corner_offset),
                             right->GetDeviceTexture(),
                             texxform,
                             nux::color::White);
      }
      {
        // Top right corner
        texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
        texxform.SetWrap(nux::TEXWRAP_CLAMP_TO_BORDER, nux::TEXWRAP_CLAMP_TO_BORDER);

        gfx_context.QRP_1Tex(geo.x + geo.width - right->GetWidth(),
                             geo.y - top_corner_offset,
                             top_corner->GetWidth(),
                             top_corner->GetHeight(),
                             top_corner->GetDeviceTexture(),
                             texxform,
                             nux::color::White);
      }
      {
        // Top edge
        texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
        texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

        gfx_context.QRP_1Tex(geo.x + geo.width,
                             geo.y - 10,
                             geometry.width - (geo.x + geo.width),
                             top_tile->GetHeight(),
                             top_tile->GetDeviceTexture(),
                             texxform,
                             nux::color::White);
      }

      if (paint_blur)
      {
        texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
        texxform.SetWrap(nux::TEXWRAP_CLAMP_TO_BORDER, nux::TEXWRAP_CLAMP_TO_BORDER);

        // save current blending state
        unsigned int alpha = 0;
        unsigned int src   = 0;
        unsigned int dest  = 0;
        gfx_context.GetRenderStates().GetBlend(alpha, src, dest);
        gfx_context.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        // top right corner
        nux::Geometry blur_geo(geo.x + geo.width - right->GetWidth(),
                               geo.y - top_corner_offset,
                               top_corner->GetWidth(),
                               top_corner->GetHeight());
        bg_blur_texture_ = bg_effect_helper_.GetBlurRegion(blur_geo);
        if (bg_blur_texture_.IsValid())
        {
          gfx_context.QRP_TexMaskTexAlpha(blur_geo.x,
                                          blur_geo.y,
                                          blur_geo.width,
                                          blur_geo.height,
                                          bg_blur_texture_,
                                          texxform,
                                          nux::color::White,
                                          top_corner_mask->GetDeviceTexture(),
                                          texxform,
                                          nux::color::White);
        }

        // bottom left corner
        blur_geo.x      = geo.x - left_corner_offset;
        blur_geo.y      = geo.y + (geo.height - left_corner->GetHeight());
        blur_geo.width  = left_corner->GetWidth();
        blur_geo.height = left_corner->GetHeight();
        bg_blur_texture_ = bg_effect_helper_.GetBlurRegion(blur_geo);
        if (bg_blur_texture_.IsValid())
        {
          gfx_context.QRP_TexMaskTexAlpha(blur_geo.x,
                                          blur_geo.y,
                                          blur_geo.width,
                                          blur_geo.height,
                                          bg_blur_texture_,
                                          texxform,
                                          nux::color::White,
                                          left_corner_mask->GetDeviceTexture(),
                                          texxform,
                                          nux::color::White);
        }

        // bottom right corner
        blur_geo.x      = geo.x + (geo.width - corner->GetWidth());
        blur_geo.y      = geo.y + (geo.height - corner->GetHeight());
        blur_geo.width  = corner->GetWidth();
        blur_geo.height = corner->GetHeight();
        bg_blur_texture_ = bg_effect_helper_.GetBlurRegion(blur_geo);
        if (bg_blur_texture_.IsValid())
        {
          gfx_context.QRP_TexMaskTexAlpha(blur_geo.x,
                                          blur_geo.y,
                                          blur_geo.width,
                                          blur_geo.height,
                                          bg_blur_texture_,
                                          texxform,
                                          nux::color::White,
                                          corner_mask->GetDeviceTexture(),
                                          texxform,
                                          nux::color::White);
        }

        // restore last blending state
        gfx_context.GetRenderStates().SetBlend(alpha, src, dest);
      }
    }
  }
    
  nux::TexCoordXForm texxform_absolute_bg;
  texxform_absolute_bg.flip_v_coord = true;
  texxform_absolute_bg.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
  texxform_absolute_bg.uoffset = ((float) content_geo.x) / absolute_geo.width;
  texxform_absolute_bg.voffset = ((float) content_geo.y) / absolute_geo.height;
  texxform_absolute_bg.SetWrap(nux::TEXWRAP_CLAMP, nux::TEXWRAP_CLAMP);
  
  if (paint_blur)
  {
    nux::Geometry blur_geo(absolute_geo.x, absolute_geo.y, content_geo.width, content_geo.height);
    bg_blur_texture_ = bg_effect_helper_.GetBlurRegion(blur_geo);
    
    if (bg_blur_texture_.IsValid())
    {
      nux::Geometry bg_clip = geo;
      gfx_context.PushClippingRectangle(bg_clip);
      
      gfx_context.GetRenderStates().SetBlend(false);
      gfx_context.QRP_1Tex (content_geo.x, content_geo.y,
                            content_geo.width, content_geo.height,
                            bg_blur_texture_, texxform_absolute_bg, nux::color::White);
      gPainter.PopBackground();
      
      gfx_context.PopClippingRectangle();
    }
  }

  // Draw the left and top lines
  gfx_context.GetRenderStates().SetColorMask(true, true, true, true);
  gfx_context.GetRenderStates().SetBlend(true);
  gfx_context.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);

  const double line_opacity = 0.22;
  nux::Color line_color = nux::color::White * line_opacity;
  nux::GetPainter().Paint2DQuadColor(gfx_context,
                                     nux::Geometry(geometry.x,
                                                   geometry.y,
                                                   1,
                                                   content_geo.height + INNER_CORNER_RADIUS),
                                     nux::color::Transparent,
                                     line_color,
                                     line_color,
                                     nux::color::Transparent);

  nux::GetPainter().Paint2DQuadColor(gfx_context,
                                     nux::Geometry(geometry.x,
                                                   geometry.y,
                                                   content_geo.width + INNER_CORNER_RADIUS,
                                                   1),
                                     nux::color::Transparent,
                                     nux::color::Transparent,
                                     line_color,
                                     line_color);
  
  // Draw the background
  bg_darken_layer_->SetGeometry(content_geo);
  nux::GetPainter().RenderSinglePaintLayer(gfx_context, content_geo, bg_darken_layer_);
  
  bg_layer_->SetGeometry(content_geo);
  nux::GetPainter().RenderSinglePaintLayer(gfx_context, content_geo, bg_layer_);
  
  
  texxform_absolute_bg.flip_v_coord = false;
  texxform_absolute_bg.uoffset = (1.0f / bg_shine_texture_->GetWidth()) * parent->x_offset;
  texxform_absolute_bg.voffset = (1.0f / bg_shine_texture_->GetHeight()) * parent->y_offset;
  
  gfx_context.GetRenderStates().SetColorMask(true, true, true, false);
  gfx_context.GetRenderStates().SetBlend(true, GL_DST_COLOR, GL_ONE);
  
  gfx_context.QRP_1Tex (content_geo.x, content_geo.y,
                        content_geo.width, content_geo.height,
                        bg_shine_texture_, texxform_absolute_bg, nux::color::White);
  
  if (dash::Settings::Instance().GetFormFactor() != dash::FormFactor::NETBOOK)
  {
    // Make bottom-right corner rounded
    nux::ROPConfig rop;
    rop.Blend = true;
    rop.SrcBlend = GL_ZERO;
    rop.DstBlend = GL_SRC_ALPHA;
    nux::GetPainter().PaintShapeCornerROP(gfx_context,
                                          content_geo,
                                          nux::color::White,
                                          nux::eSHAPE_CORNER_ROUND4,
                                          nux::eCornerBottomRight,
                                          true,
                                          rop);

    geo = content_geo;

    gfx_context.GetRenderStates().SetColorMask(true, true, true, true);
    gfx_context.GetRenderStates().SetBlend(true);
    gfx_context.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);

    // Fill in corners (meh)
    for (int i = 0; i < INNER_CORNER_RADIUS; ++i)
    {
      nux::Geometry fill_geo (geo.x + geo.width, geo.y + i, INNER_CORNER_RADIUS - i, 1);
      nux::GetPainter().Paint2DQuadColor(gfx_context, fill_geo, bg_color_);
      
      nux::Color dark = bg_color_ * 0.8f;
      dark.alpha = bg_color_.alpha;
      fill_geo = nux::Geometry(geo.x + i, geo.y + geo.height, 1, INNER_CORNER_RADIUS - i);
      nux::GetPainter().Paint2DQuadColor(gfx_context, fill_geo, dark);
    }
  }
}

void OverlayRendererImpl::DrawContent(nux::GraphicsEngine& gfx_context, nux::Geometry content_geo, nux::Geometry absolute_geo, nux::Geometry geometry)
{
  bool paint_blur = BackgroundEffectHelper::blur_type != BLUR_NONE;
  nux::Geometry geo = geometry;
  bgs = 0;
  
  gfx_context.PushClippingRectangle(geo);
  
  gfx_context.GetRenderStates().SetBlend(true);
  gfx_context.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);
  
  nux::TexCoordXForm texxform_absolute_bg;
  texxform_absolute_bg.flip_v_coord = true;
  texxform_absolute_bg.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
  texxform_absolute_bg.uoffset = ((float) content_geo.x) / absolute_geo.width;
  texxform_absolute_bg.voffset = ((float) content_geo.y) / absolute_geo.height;
  texxform_absolute_bg.SetWrap(nux::TEXWRAP_CLAMP, nux::TEXWRAP_CLAMP);
  
  nux::ROPConfig rop;
  rop.Blend = false;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  
  if (bg_blur_texture_.IsValid() && paint_blur)
  {
    gPainter.PushTextureLayer(gfx_context, content_geo,
                              bg_blur_texture_,
                              texxform_absolute_bg,
                              nux::color::White,
                              true, // write alpha?
                              rop);
    bgs++;
  }
  
  // draw the darkening behind our paint
  nux::GetPainter().PushLayer(gfx_context, bg_darken_layer_->GetGeometry(), bg_darken_layer_);
  bgs++;
  
  nux::GetPainter().PushLayer(gfx_context, bg_layer_->GetGeometry(), bg_layer_);
  bgs++;
  
  // apply the shine
  rop.Blend = true;
  rop.SrcBlend = GL_DST_COLOR;
  rop.DstBlend = GL_ONE;
  texxform_absolute_bg.flip_v_coord = false;
  texxform_absolute_bg.uoffset = (1.0f / bg_shine_texture_->GetWidth()) * parent->x_offset;
  texxform_absolute_bg.voffset = (1.0f / bg_shine_texture_->GetHeight()) * parent->y_offset;
  
  nux::GetPainter().PushTextureLayer(gfx_context, bg_layer_->GetGeometry(),
                                     bg_shine_texture_,
                                     texxform_absolute_bg,
                                     nux::color::White,
                                     false,
                                     rop);
  bgs++;
}

void OverlayRendererImpl::DrawContentCleanup(nux::GraphicsEngine& gfx_context, nux::Geometry content_geo, nux::Geometry absolute_geo, nux::Geometry geometry)
{
  nux::GetPainter().PopBackground(bgs);
  
  gfx_context.GetRenderStates().SetBlend(false);
  gfx_context.PopClippingRectangle();
  
  if (dash::Settings::Instance().GetFormFactor() != dash::FormFactor::NETBOOK)
  {
    // Make bottom-right corner rounded
    nux::ROPConfig rop;
    rop.Blend = true;
    rop.SrcBlend = GL_ZERO;
    rop.DstBlend = GL_SRC_ALPHA;
    nux::GetPainter().PaintShapeCornerROP(gfx_context,
                                          content_geo,
                                          nux::color::White,
                                          nux::eSHAPE_CORNER_ROUND4,
                                          nux::eCornerBottomRight,
                                          true,
                                          rop);
  }
  
  bgs = 0;
}


 
OverlayRenderer::OverlayRenderer()
  : pimpl_(new OverlayRendererImpl(this))
{
  
}


OverlayRenderer::~OverlayRenderer()
{
  delete pimpl_;
}

void OverlayRenderer::AboutToHide()
{
  pimpl_->visible = false;
  pimpl_->bg_effect_helper_.enabled = false;
  need_redraw.emit();
}

void OverlayRenderer::AboutToShow()
{
  pimpl_->ubus_manager_.SendMessage(UBUS_BACKGROUND_REQUEST_COLOUR_EMIT);
  pimpl_->visible = true;
  pimpl_->bg_effect_helper_.enabled = true;
  need_redraw.emit();
}

void OverlayRenderer::SetOwner(nux::View* owner)
{
  pimpl_->bg_effect_helper_.owner= owner;
}

void OverlayRenderer::DisableBlur()
{
  pimpl_->bg_effect_helper_.blur_type = BLUR_NONE;
}

void OverlayRenderer::DrawFull(nux::GraphicsEngine& gfx_context, nux::Geometry content_geo, nux::Geometry absolute_geo, nux::Geometry geo, bool force_edges)
{
  pimpl_->Draw(gfx_context, content_geo, absolute_geo, geo, force_edges);
}

void OverlayRenderer::DrawInner(nux::GraphicsEngine& gfx_context, nux::Geometry content_geo, nux::Geometry absolute_geo, nux::Geometry geo)
{
  pimpl_->DrawContent(gfx_context, content_geo, absolute_geo, geo);
}

void OverlayRenderer::DrawInnerCleanup(nux::GraphicsEngine& gfx_context, nux::Geometry content_geo, nux::Geometry absolute_geo, nux::Geometry geo)
{
  pimpl_->DrawContentCleanup(gfx_context, content_geo, absolute_geo, geo);
}
  
}


