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
 * Authored by: Jay Taoko <jay.taoko@canonical.com>
 * Authored by: Mirco Müller <mirco.mueller@canonical.com
 */

#include <Nux/Nux.h>
#include <Nux/VLayout.h>
#include <Nux/HLayout.h>
#include <Nux/WindowThread.h>
#include <Nux/WindowCompositor.h>
#include <Nux/BaseWindow.h>
#include <Nux/Button.h>
#include <NuxGraphics/GraphicsEngine.h>
#include <Nux/TextureArea.h>
#include <NuxImage/CairoGraphics.h>

#include "CairoTexture.h"
#include "QuicklistMenuItem.h"
#include "ubus-server.h"
#include "UBusMessages.h"

#include "Tooltip.h"

using unity::texture_from_cairo_graphics;

namespace nux
{
NUX_IMPLEMENT_OBJECT_TYPE(Tooltip);

Tooltip::Tooltip()
{
  _texture_bg = 0;
  _texture_mask = 0;
  _texture_outline = 0;
  _cairo_text_has_changed = true;

  _anchorX   = 0;
  _anchorY   = 0;
  _labelText = TEXT("Unity");

  _anchor_width   = 10;
  _anchor_height  = 18;
  _corner_radius  = 4;
  _padding        = 15;
  _top_size       = 4;

  _hlayout         = new nux::HLayout(TEXT(""), NUX_TRACKER_LOCATION);
  _vlayout         = new nux::VLayout(TEXT(""), NUX_TRACKER_LOCATION);

  _left_space = new nux::SpaceLayout(_padding + _anchor_width + _corner_radius, _padding + _anchor_width + _corner_radius, 1, 1000);
  _right_space = new nux::SpaceLayout(_padding + _corner_radius, _padding + _corner_radius, 1, 1000);

  _top_space = new nux::SpaceLayout(1, 1000, _padding + _corner_radius, _padding + _corner_radius);
  _bottom_space = new nux::SpaceLayout(1, 1000, _padding + _corner_radius, _padding + _corner_radius);

  _vlayout->AddLayout(_top_space, 0);

  _tooltip_text = new nux::StaticCairoText(_labelText.GetTCharPtr(), NUX_TRACKER_LOCATION);
  _tooltip_text->sigTextChanged.connect(sigc::mem_fun(this, &Tooltip::RecvCairoTextChanged));
  _tooltip_text->sigFontChanged.connect(sigc::mem_fun(this, &Tooltip::RecvCairoTextChanged));
  _tooltip_text->Reference();

  _vlayout->AddView(_tooltip_text, 1, eCenter, eFull);

  _vlayout->AddLayout(_bottom_space, 0);

  _hlayout->AddLayout(_left_space, 0);
  _hlayout->AddLayout(_vlayout, 1, eCenter, eFull);
  _hlayout->AddLayout(_right_space, 0);

  SetWindowSizeMatchLayout(true);
  SetLayout(_hlayout);

}

Tooltip::~Tooltip()
{
  _tooltip_text->UnReference();

  if (_texture_bg) _texture_bg->UnReference();
  if (_texture_mask) _texture_mask->UnReference();
  if (_texture_outline) _texture_outline->UnReference();
}

Area* Tooltip::FindAreaUnderMouse(const Point& mouse_position, NuxEventType event_type)
{
  // No area under mouse to allow click through to entities below
  return 0;
}

void Tooltip::ShowTooltipWithTipAt(int anchor_tip_x, int anchor_tip_y)
{
  _anchorX = anchor_tip_x;
  _anchorY = anchor_tip_y;

  int x = _anchorX - _padding;
  int y = anchor_tip_y - _anchor_height / 2 - _top_size - _corner_radius - _padding;

  SetBaseX(x);
  SetBaseY(y);

  PushToFront();

  ShowWindow(true);
  UBusServer* ubus = ubus_server_get_default();
  ubus_server_send_message(ubus, UBUS_TOOLTIP_SHOWN, NULL);
}

void Tooltip::Draw(GraphicsEngine& gfxContext, bool forceDraw)
{
  Geometry base = GetGeometry();

  // the elements position inside the window are referenced to top-left window
  // corner. So bring base to (0, 0).
  base.SetX(0);
  base.SetY(0);
  gfxContext.PushClippingRectangle(base);

  nux::GetWindowThread()->GetGraphicsDisplay().GetGraphicsEngine()->GetRenderStates().SetBlend(false);

  TexCoordXForm texxform_bg;
  texxform_bg.SetWrap(TEXWRAP_CLAMP, TEXWRAP_CLAMP);
  texxform_bg.SetTexCoordType(TexCoordXForm::OFFSET_COORD);

  TexCoordXForm texxform_mask;
  texxform_mask.SetWrap(TEXWRAP_CLAMP, TEXWRAP_CLAMP);
  texxform_mask.SetTexCoordType(TexCoordXForm::OFFSET_COORD);


  gfxContext.QRP_2TexMod(base.x,
                         base.y,
                         base.width,
                         base.height,
                         _texture_bg->GetDeviceTexture(),
                         texxform_bg,
                         Color(1.0f, 1.0f, 1.0f, 1.0f),
                         _texture_mask->GetDeviceTexture(),
                         texxform_mask,
                         Color(1.0f, 1.0f, 1.0f, 1.0f));


  TexCoordXForm texxform;
  texxform.SetWrap(TEXWRAP_CLAMP, TEXWRAP_CLAMP);
  texxform.SetTexCoordType(TexCoordXForm::OFFSET_COORD);

  nux::GetWindowThread()->GetGraphicsDisplay().GetGraphicsEngine()->GetRenderStates().SetBlend(true);
  nux::GetWindowThread()->GetGraphicsDisplay().GetGraphicsEngine()->GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);
  gfxContext.QRP_1Tex(base.x,
                      base.y,
                      base.width,
                      base.height,
                      _texture_outline->GetDeviceTexture(),
                      texxform,
                      Color(1.0f, 1.0f, 1.0f, 1.0f));

  nux::GetWindowThread()->GetGraphicsDisplay().GetGraphicsEngine()->GetRenderStates().SetBlend(false);

  _tooltip_text->ProcessDraw(gfxContext, forceDraw);

  gfxContext.PopClippingRectangle();
}

void Tooltip::DrawContent(GraphicsEngine& GfxContext, bool force_draw)
{

}

void Tooltip::PreLayoutManagement()
{
  int MaxItemWidth = 0;
  int TotalItemHeight = 0;
  int  textWidth  = 0;
  int  textHeight = 0;

  _tooltip_text->GetTextExtents(textWidth, textHeight);

  if (textWidth > MaxItemWidth)
    MaxItemWidth = textWidth;
  TotalItemHeight += textHeight;


  if (TotalItemHeight < _anchor_height)
  {
    _top_space->SetMinMaxSize(1, (_anchor_height - TotalItemHeight) / 2 + _padding + _corner_radius);
    _bottom_space->SetMinMaxSize(1, (_anchor_height - TotalItemHeight) / 2 + 1 + _padding + _corner_radius);
  }

  BaseWindow::PreLayoutManagement();
}

long Tooltip::PostLayoutManagement(long LayoutResult)
{
  long result = BaseWindow::PostLayoutManagement(LayoutResult);
  UpdateTexture();

  return result;
}

void Tooltip::RecvCairoTextChanged(StaticCairoText* cairo_text)
{
  _cairo_text_has_changed = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

void tint_dot_hl(cairo_t* cr,
                 gint    width,
                 gint    height,
                 gfloat  hl_x,
                 gfloat  hl_y,
                 gfloat  hl_size,
                 gfloat* rgba_tint,
                 gfloat* rgba_hl,
                 gfloat* rgba_dot)
{
  // clear normal context
  cairo_scale(cr, 1.0f, 1.0f);
  cairo_set_source_rgba(cr, 0.0f, 0.0f, 0.0f, 0.0f);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  // prepare drawing for normal context
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  // create path in normal context
  cairo_rectangle(cr, 0.0f, 0.0f, (gdouble) width, (gdouble) height);

  // fill path of normal context with tint
  cairo_set_source_rgba(cr,
                        rgba_tint[0],
                        rgba_tint[1],
                        rgba_tint[2],
                        rgba_tint[3]);
  cairo_fill(cr);
}

void _setup(cairo_surface_t** surf,
            cairo_t**         cr,
            gboolean          outline,
            gint              width,
            gint              height,
            gboolean          negative)
{
  // clear context
  cairo_scale(*cr, 1.0f, 1.0f);
  if (outline)
  {
    cairo_set_source_rgba(*cr, 0.0f, 0.0f, 0.0f, 0.0f);
    cairo_set_operator(*cr, CAIRO_OPERATOR_CLEAR);
  }
  else
  {
    cairo_set_operator(*cr, CAIRO_OPERATOR_OVER);
    if (negative)
      cairo_set_source_rgba(*cr, 0.0f, 0.0f, 0.0f, 0.0f);
    else
      cairo_set_source_rgba(*cr, 1.0f, 1.0f, 1.0f, 1.0f);
  }
  cairo_paint(*cr);
}

void _compute_full_mask_path(cairo_t* cr,
                             gfloat   anchor_width,
                             gfloat   anchor_height,
                             gint     width,
                             gint     height,
                             gint     upper_size,
                             gfloat   radius,
                             guint    pad)
{
  //     0  1        2  3
  //     +--+--------+--+
  //     |              |
  //     + 14           + 4
  //     |              |
  //     |              |
  //     |              |
  //     + 13           |
  //    /               |
  //   /                |
  //  + 12              |
  //   \                |
  //    \               |
  //  11 +              |
  //     |              |
  //     |              |
  //     |              |
  //  10 +              + 5
  //     |              |
  //     +--+--------+--+ 6
  //     9  8        7


  gfloat padding  = pad;
  int ZEROPOINT5 = 0.0f;

  gfloat HeightToAnchor = 0.0f;
  HeightToAnchor = ((gfloat) height - 2.0f * radius - anchor_height - 2 * padding) / 2.0f;
  if (HeightToAnchor < 0.0f)
  {
    g_warning("Anchor-height and corner-radius a higher than whole texture!");
    return;
  }

  if (upper_size >= 0)
  {
    if (upper_size > height - 2.0f * radius - anchor_height - 2 * padding)
    {
      //g_warning ("[_compute_full_mask_path] incorrect upper_size value");
      HeightToAnchor = 0;
    }
    else
    {
      HeightToAnchor = height - 2.0f * radius - anchor_height - 2 * padding - upper_size;
    }
  }
  else
  {
    HeightToAnchor = (height - 2.0f * radius - anchor_height - 2 * padding) / 2.0f;
  }

  cairo_translate(cr, -0.5f, -0.5f);

  // create path
  cairo_move_to(cr, padding + anchor_width + radius + ZEROPOINT5, padding + ZEROPOINT5);  // Point 1
  cairo_line_to(cr, width - padding - radius, padding + ZEROPOINT5);    // Point 2
  cairo_arc(cr,
            width  - padding - radius + ZEROPOINT5,
            padding + radius + ZEROPOINT5,
            radius,
            -90.0f * G_PI / 180.0f,
            0.0f * G_PI / 180.0f);   // Point 4
  cairo_line_to(cr,
                (gdouble) width - padding + ZEROPOINT5,
                (gdouble) height - radius - padding + ZEROPOINT5); // Point 5
  cairo_arc(cr,
            (gdouble) width - padding - radius + ZEROPOINT5,
            (gdouble) height - padding - radius + ZEROPOINT5,
            radius,
            0.0f * G_PI / 180.0f,
            90.0f * G_PI / 180.0f);  // Point 7
  cairo_line_to(cr,
                anchor_width + padding + radius + ZEROPOINT5,
                (gdouble) height - padding + ZEROPOINT5); // Point 8

  cairo_arc(cr,
            anchor_width + padding + radius + ZEROPOINT5,
            (gdouble) height - padding - radius,
            radius,
            90.0f * G_PI / 180.0f,
            180.0f * G_PI / 180.0f); // Point 10

  cairo_line_to(cr,
                padding + anchor_width + ZEROPOINT5,
                (gdouble) height - padding - radius - HeightToAnchor + ZEROPOINT5);   // Point 11
  cairo_line_to(cr,
                padding + ZEROPOINT5,
                (gdouble) height - padding - radius - HeightToAnchor - anchor_height / 2.0f + ZEROPOINT5); // Point 12
  cairo_line_to(cr,
                padding + anchor_width + ZEROPOINT5,
                (gdouble) height - padding - radius - HeightToAnchor - anchor_height + ZEROPOINT5);  // Point 13

  cairo_line_to(cr, padding + anchor_width + ZEROPOINT5, padding + radius  + ZEROPOINT5);   // Point 14
  cairo_arc(cr,
            padding + anchor_width + radius + ZEROPOINT5,
            padding + radius + ZEROPOINT5,
            radius,
            180.0f * G_PI / 180.0f,
            270.0f * G_PI / 180.0f);

  cairo_close_path(cr);
}

void compute_mask(cairo_t* cr)
{
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_fill_preserve(cr);
}

void compute_outline(cairo_t* cr,
                     gfloat   line_width,
                     gfloat*  rgba_line)
{
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgba(cr,
                        rgba_line[0],
                        rgba_line[1],
                        rgba_line[2],
                        rgba_line[3]);
  cairo_set_line_width(cr, line_width);
  cairo_stroke(cr);
}

void _draw(cairo_t* cr,
           gboolean outline,
           gfloat   line_width,
           gfloat*  rgba,
           gboolean negative,
           gboolean stroke)
{
  // prepare drawing
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

  // actually draw the mask
  if (outline)
  {
    cairo_set_line_width(cr, line_width);
    cairo_set_source_rgba(cr, rgba[0], rgba[1], rgba[2], rgba[3]);
  }
  else
  {
    if (negative)
      cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 1.0f);
    else
      cairo_set_source_rgba(cr, 0.0f, 0.0f, 0.0f, 0.0f);
  }

  // stroke or fill?
  if (stroke)
    cairo_stroke_preserve(cr);
  else
    cairo_fill_preserve(cr);
}

void _finalize(cairo_t** cr,
               gboolean  outline,
               gfloat    line_width,
               gfloat*   rgba,
               gboolean  negative,
               gboolean  stroke)
{
  // prepare drawing
  cairo_set_operator(*cr, CAIRO_OPERATOR_SOURCE);

  // actually draw the mask
  if (outline)
  {
    cairo_set_line_width(*cr, line_width);
    cairo_set_source_rgba(*cr, rgba[0], rgba[1], rgba[2], rgba[3]);
  }
  else
  {
    if (negative)
      cairo_set_source_rgba(*cr, 1.0f, 1.0f, 1.0f, 1.0f);
    else
      cairo_set_source_rgba(*cr, 0.0f, 0.0f, 0.0f, 0.0f);
  }

  // stroke or fill?
  if (stroke)
    cairo_stroke(*cr);
  else
    cairo_fill(*cr);
}

void
compute_full_outline_shadow(
  cairo_t* cr,
  cairo_surface_t* surf,
  gint    width,
  gint    height,
  gfloat  anchor_width,
  gfloat  anchor_height,
  gint    upper_size,
  gfloat  corner_radius,
  guint   blur_coeff,
  gfloat* rgba_shadow,
  gfloat  line_width,
  gint    padding_size,
  gfloat* rgba_line)
{
  _setup(&surf, &cr, TRUE, width, height, FALSE);
  _compute_full_mask_path(cr,
                          anchor_width,
                          anchor_height,
                          width,
                          height,
                          upper_size,
                          corner_radius,
                          padding_size);

  _draw(cr, TRUE, line_width, rgba_shadow, FALSE, FALSE);
  CairoGraphics* dummy = new CairoGraphics(CAIRO_FORMAT_A1, 1, 1);
  dummy->BlurSurface(blur_coeff, surf);
  delete dummy;
  compute_mask(cr);
}

void compute_full_mask(
  cairo_t* cr,
  cairo_surface_t* surf,
  gint     width,
  gint     height,
  gfloat   radius,
  guint    shadow_radius,
  gfloat   anchor_width,
  gfloat   anchor_height,
  gint     upper_size,
  gboolean negative,
  gboolean outline,
  gfloat   line_width,
  gint     padding_size,
  gfloat*  rgba)
{
  _setup(&surf, &cr, outline, width, height, negative);
  _compute_full_mask_path(cr,
                          anchor_width,
                          anchor_height,
                          width,
                          height,
                          upper_size,
                          radius,
                          padding_size);
  _finalize(&cr, outline, line_width, rgba, negative, outline);
}

void Tooltip::UpdateTexture()
{
  if (_cairo_text_has_changed == false)
    return;

  int height = GetBaseHeight();

  _top_size = 0;
  int x = _anchorX - _padding;
  int y = _anchorY - height / 2;

  SetBaseX(x);
  SetBaseY(y);

  float blur_coef         = 6.0f;

  CairoGraphics* cairo_bg       = new CairoGraphics(CAIRO_FORMAT_ARGB32, GetBaseWidth(), GetBaseHeight());
  CairoGraphics* cairo_mask     = new CairoGraphics(CAIRO_FORMAT_ARGB32, GetBaseWidth(), GetBaseHeight());
  CairoGraphics* cairo_outline  = new CairoGraphics(CAIRO_FORMAT_ARGB32, GetBaseWidth(), GetBaseHeight());

  cairo_t* cr_bg      = cairo_bg->GetContext();
  cairo_t* cr_mask    = cairo_mask->GetContext();
  cairo_t* cr_outline = cairo_outline->GetContext();

  float   tint_color[4]    = {0.074f, 0.074f, 0.074f, 0.80f};
  float   hl_color[4]      = {1.0f, 1.0f, 1.0f, 0.15f};
  float   dot_color[4]     = {1.0f, 1.0f, 1.0f, 0.20f};
  float   shadow_color[4]  = {0.0f, 0.0f, 0.0f, 1.00f};
  float   outline_color[4] = {1.0f, 1.0f, 1.0f, 0.75f};
  float   mask_color[4]    = {1.0f, 1.0f, 1.0f, 1.00f};

  tint_dot_hl(cr_bg,
              GetBaseWidth(),
              GetBaseHeight(),
              GetBaseWidth() / 2.0f,
              0,
              Max<float>(GetBaseWidth() / 1.3f, GetBaseHeight() / 1.3f),
              tint_color,
              hl_color,
              dot_color);

  compute_full_outline_shadow
  (
    cr_outline,
    cairo_outline->GetSurface(),
    GetBaseWidth(),
    GetBaseHeight(),
    _anchor_width,
    _anchor_height,
    -1,
    _corner_radius,
    blur_coef,
    shadow_color,
    1.0f,
    _padding,
    outline_color);

  compute_full_mask(
    cr_mask,
    cairo_mask->GetSurface(),
    GetBaseWidth(),
    GetBaseHeight(),
    _corner_radius, // radius,
    16,             // shadow_radius,
    _anchor_width,  // anchor_width,
    _anchor_height, // anchor_height,
    -1,             // upper_size,
    true,           // negative,
    false,          // outline,
    1.0,            // line_width,
    _padding,       // padding_size,
    mask_color);

  cairo_destroy(cr_bg);
  cairo_destroy(cr_outline);
  cairo_destroy(cr_mask);

  if (_texture_bg)
    _texture_bg->UnReference();
  _texture_bg = texture_from_cairo_graphics(*cairo_bg);

  if (_texture_mask)
    _texture_mask->UnReference();
  _texture_mask = texture_from_cairo_graphics(*cairo_mask);

  if (_texture_outline)
    _texture_outline->UnReference();
  _texture_outline = texture_from_cairo_graphics(*cairo_outline);

  delete cairo_bg;
  delete cairo_mask;
  delete cairo_outline;
  _cairo_text_has_changed = false;
}

void Tooltip::PositionChildLayout(float offsetX,
                                  float offsetY)
{
}

void Tooltip::LayoutWindowElements()
{
}

void Tooltip::NotifyConfigurationChange(int width,
                                        int height)
{
}

void Tooltip::SetText(NString text)
{
  if (_labelText == text)
    return;

  _labelText = text;
  _tooltip_text->SetText(_labelText);
  QueueRelayout();
}

// Introspection

std::string Tooltip::GetName() const
{
  return "ToolTip";
}

void Tooltip::AddProperties(GVariantBuilder* builder)
{
  g_variant_builder_add(builder, "{sv}", "text", g_variant_new_string(_labelText.GetTCharPtr()));
  g_variant_builder_add(builder, "{sv}", "x", g_variant_new_int32(GetBaseX()));
  g_variant_builder_add(builder, "{sv}", "y", g_variant_new_int32(GetBaseY()));
  g_variant_builder_add(builder, "{sv}", "width", g_variant_new_int32(GetBaseWidth()));
  g_variant_builder_add(builder, "{sv}", "height", g_variant_new_int32(GetBaseHeight()));
  g_variant_builder_add(builder, "{sv}", "active", g_variant_new_boolean(IsVisible()));
}

} // namespace nux
