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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *              Ken VanDine <ken.vandine@canonical.com>
 *
 */

#include "unity-shared/DashStyle.h"
#include "unity-shared/PreviewStyle.h"
#include <NuxCore/Logger.h>
#include <Nux/Layout.h>

#include "SocialPreviewContent.h"

namespace unity
{
namespace dash
{
namespace previews
{
DECLARE_LOGGER(logger, "unity.dash.previews.social.content");

namespace
{
const int BUBBLE_WIDTH = 300;
const int BUBBLE_HEIGHT = 250;
const int TAIL_HEIGHT = 50;
const int TAIL_POS_FROM_RIGHT = 60;
}

inline nux::Geometry GetBubbleGeometry(nux::Geometry const& geo)
{
  int width = MIN(BUBBLE_WIDTH, geo.width);
  int height = MIN(BUBBLE_HEIGHT + TAIL_HEIGHT, geo.height);

  return nux::Geometry(geo.x + (geo.width - width)/2, geo.y + (geo.height - height)/2, width, height);
}

NUX_IMPLEMENT_OBJECT_TYPE(SocialPreviewContent);

SocialPreviewContent::SocialPreviewContent(std::string const& text, NUX_FILE_LINE_DECL)
: View(NUX_FILE_LINE_PARAM)
{
  SetupViews();
  if (text.length() > 0)
    SetText(text);
}

SocialPreviewContent::~SocialPreviewContent()
{
}

void SocialPreviewContent::SetText(std::string const& text)
{
  std::stringstream ss;
  ss << "<b>&#x201C;</b> ";
  ss << text;
  ss << " <b>&#x201E;</b>";
  text_->SetText(ss.str());
  UpdateBaloonTexture();
}

void SocialPreviewContent::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{

  nux::Geometry const& geo = GetGeometry();

  gPainter.PaintBackground(gfx_engine, geo);
  // set up our texture mode
  nux::TexCoordXForm texxform;
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

  // clear what is behind us
  unsigned int alpha = 0, src = 0, dest = 0;
  gfx_engine.GetRenderStates().GetBlend(alpha, src, dest);
  gfx_engine.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  nux::ObjectPtr<nux::IOpenGLBaseTexture> tex = cr_bubble_->GetTexture()->GetDeviceTexture();

  nux::Geometry geo_bubble(GetBubbleGeometry(geo));

  gfx_engine.QRP_1Tex(geo_bubble.x,
                      geo_bubble.y,
                      tex->GetWidth(),
                      tex->GetHeight(),
                      tex,
                      texxform,
                      nux::Color(1.0f, 1.0f, 1.0f, 1.0f));

  gfx_engine.GetRenderStates().SetBlend(alpha, src, dest);

  if (GetCompositionLayout())
  {
    gPainter.PushPaintLayerStack();
    {
      nux::Geometry clip_geo = geo;

      gfx_engine.PushClippingRectangle(clip_geo);
      gPainter.PushPaintLayerStack();
      GetCompositionLayout()->ProcessDraw(gfx_engine, force_draw);
      gPainter.PopPaintLayerStack();
      gfx_engine.PopClippingRectangle();
    }
    gPainter.PopPaintLayerStack();
  }
}

void SocialPreviewContent::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
}

void SocialPreviewContent::SetupViews()
{
  dash::previews::Style const& style = dash::previews::Style::Instance();

  auto on_mouse_down = [&](int x, int y, unsigned long button_flags, unsigned long key_flags) { this->preview_container_.OnMouseDown(x, y, button_flags, key_flags); };

  text_ = new StaticCairoText("", false, NUX_TRACKER_LOCATION);
  text_->SetLines(-8);
  text_->SetFont(style.content_font());
  text_->SetLineSpacing(5);
  text_->SetTextEllipsize(StaticCairoText::NUX_ELLIPSIZE_MIDDLE);
  text_->mouse_click.connect(on_mouse_down);

  nux::Layout* layout = new nux::Layout();
  layout->AddView(text_.GetPointer(), 1);

  mouse_click.connect(on_mouse_down);

  SetLayout(layout);

  cr_bubble_.reset(new nux::CairoWrapper(GetGeometry(), sigc::bind(sigc::mem_fun(this, &SocialPreviewContent::RedrawBubble), nux::ButtonVisualState::VISUAL_STATE_PRELIGHT)));
}

void SocialPreviewContent::UpdateBaloonTexture()
{
  nux::Geometry const& geo = GetGeometry();

  nux::Geometry geo_cr(GetBubbleGeometry(geo));

  int max_width = std::max(0, (int)(geo_cr.width - 2*(geo_cr.width*0.1)));
  int max_height = std::max(0, (int)((geo_cr.height - TAIL_HEIGHT) - 2*((geo_cr.height - TAIL_HEIGHT)*0.1)));

  // this will update the texture with the actual size of the text.
  text_->SetMaximumHeight(max_height);
  text_->SetMaximumWidth(max_width);
  nux::Geometry const& geo_text = text_->GetGeometry();

  // center text
  text_->SetBaseX(geo_cr.x + geo_cr.width/2 - geo_text.width/2);
  text_->SetBaseY(geo_cr.y + geo_cr.height/2 - geo_text.height/2 - TAIL_HEIGHT/2);

  if (geo_cr.width > 0 && geo_cr.height > 0)
  {
    cr_bubble_->Invalidate(geo_cr);
  }
}

void SocialPreviewContent::RedrawBubble(nux::Geometry const& geom, cairo_t* cr, nux::ButtonVisualState faked_state)
{
  double width = std::max(0, cairo_image_surface_get_width(cairo_get_target(cr)));
  double height = std::max(0, cairo_image_surface_get_height(cairo_get_target(cr)) - TAIL_HEIGHT);

  double tailPosition = width - TAIL_POS_FROM_RIGHT - TAIL_HEIGHT;
  if (width > 0 && height > 0)
  {
    double line_width = 6.0;
    double radius = 28.0;
    double x = 0.0;
    double y = 0.0;

    DrawBubble(cr, line_width, radius, x, y, width, height, tailPosition, TAIL_HEIGHT);
  }
}

inline double _align(double val, bool odd=true)
{
  double fract = val - (int) val;

  if (odd)
  {
    // for strokes with an odd line-width
    if (fract != 0.5f)
      return (double) ((int) val + 0.5f);
    else
      return val;
  }
  else
  {
    // for strokes with an even line-width
    if (fract != 0.0f)
      return (double) ((int) val);
    else
      return val;
  }
}

void SocialPreviewContent::DrawBubble(cairo_t* cr,
                   double line_width,
                   double   radius,
                   double   x,
                   double   y,
                   double   width,
                   double   height,
                   double   tailPosition,
                   double   tailWidth)
{
  // sanity check
  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS &&
      cairo_surface_get_type(cairo_get_target(cr)) != CAIRO_SURFACE_TYPE_IMAGE)
    return;

  cairo_set_line_width(cr, line_width);

  bool odd = line_width != double((int)line_width);

  // top-left, right of the corner
  cairo_move_to(cr, _align (x + radius, odd), _align (y, odd));

  // top-right, left of the corner
  cairo_line_to(cr, _align(x + width - radius, odd), _align(y, odd));

  // top-right, below the corner
  cairo_arc(cr,
            _align(x + width - radius, odd),
            _align(y + radius, odd),
            radius,
            -90.0f * G_PI / 180.0f,
            0.0f * G_PI / 180.0f);

  // bottom-right, above the corner
  cairo_line_to(cr, _align(x + width, odd), _align(y + height - radius, odd));

  // bottom-right, left of the corner
  cairo_arc(cr,
            _align(x + width - radius, odd),
            _align(y + height - radius, odd),
            radius,
            0.0f * G_PI / 180.0f,
            90.0f * G_PI / 180.0f);

  if (tailWidth > 0.0 && tailPosition > 0 && tailPosition <= (x + width - tailWidth - radius))
  {
    // tail-right, tail top
    cairo_line_to(cr, _align(tailPosition + tailWidth, odd), _align(y + height, odd));

    // tail-right, tail bottom
    cairo_line_to(cr, _align(tailPosition + tailWidth, odd), _align(y + height + tailWidth, odd));

    // tail-right, tail bottom
    cairo_line_to(cr, _align(tailPosition, odd), _align(y + height, odd));
  }

  // bottom-left, right of the corner
  cairo_line_to(cr, _align(x + radius, odd), _align(y + height, odd));    

  // bottom-left, above the corner
  cairo_arc(cr,
            _align(x + radius, odd),
            _align(y + height - radius, odd),
            radius,
            90.0f * G_PI / 180.0f,
            180.0f * G_PI / 180.0f);

  // top-left, right of the corner
  cairo_arc(cr,
            _align(x + radius, odd),
            _align(y + radius, odd),
            radius,
            180.0f * G_PI / 180.0f,
            270.0f * G_PI / 180.0f);

  nux::Color color_fill(1.0, 1.0, 1.0, 0.2);
  cairo_set_source_rgba(cr, color_fill.red, color_fill.green, color_fill.blue, color_fill.alpha);
  cairo_fill_preserve(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_DEST_OUT);
  nux::Color color_stroke(1.0, 1.0, 1.0, 0.5);
  cairo_set_source_rgba(cr, color_stroke.red, color_stroke.green, color_stroke.blue, color_stroke.alpha);
  cairo_stroke(cr);
}

void SocialPreviewContent::PreLayoutManagement()
{
  UpdateBaloonTexture();

  View::PreLayoutManagement();
}

std::string SocialPreviewContent::GetName() const
{
  return "SocialPreviewContent";
}

void SocialPreviewContent::AddProperties(GVariantBuilder* builder)
{
}

}
}
}
