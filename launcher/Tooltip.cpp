// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2012 Canonical Ltd
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
 *              Mirco Müller <mirco.mueller@canonical.com
 *              Andrea Cimitan <andrea.cimitan@canonical.com>
 *              Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#include <Nux/Nux.h>

#include <unity-shared/CairoTexture.h>
#include <unity-shared/RawPixel.h>
#include <unity-shared/UnitySettings.h>
#include "unity-shared/DecorationStyle.h"

#include "Tooltip.h"

namespace unity
{
namespace
{
  const RawPixel ANCHOR_WIDTH       =  14_em;
  const RawPixel ANCHOR_HEIGHT      =  18_em;
  const RawPixel CORNER_RADIUS      =   4_em;
  const RawPixel TEXT_PADDING       =   8_em;
  const RawPixel MINIMUM_TEXT_WIDTH = 100_em;
}

NUX_IMPLEMENT_OBJECT_TYPE(Tooltip);
Tooltip::Tooltip(int monitor) :
  CairoBaseWindow(monitor),
  _anchorX(0),
  _anchorY(0),
  _padding(decoration::Style::Get()->ActiveShadowRadius()),
  _cairo_text_has_changed(true)
{
  _hlayout = new nux::HLayout(TEXT(""), NUX_TRACKER_LOCATION);
  _vlayout = new nux::VLayout(TEXT(""), NUX_TRACKER_LOCATION);

  _left_space = new nux::SpaceLayout(_padding.CP(cv_) + ANCHOR_WIDTH.CP(cv_), _padding.CP(cv_) + ANCHOR_WIDTH.CP(cv_), 1, 1000);
  _right_space = new nux::SpaceLayout(_padding.CP(cv_) + CORNER_RADIUS.CP(cv_), _padding.CP(cv_) + CORNER_RADIUS.CP(cv_), 1, 1000);

  _top_space = new nux::SpaceLayout(1, 1000, _padding.CP(cv_), _padding.CP(cv_));
  _bottom_space = new nux::SpaceLayout(1, 1000, _padding.CP(cv_), _padding.CP(cv_));

  _vlayout->AddLayout(_top_space, 0);

  _tooltip_text = new StaticCairoText(TEXT(""), NUX_TRACKER_LOCATION);
  _tooltip_text->SetScale(cv_->DPIScale());
  _tooltip_text->SetTextAlignment(StaticCairoText::AlignState::NUX_ALIGN_CENTRE);
  _tooltip_text->SetTextVerticalAlignment(StaticCairoText::AlignState::NUX_ALIGN_CENTRE);
  _tooltip_text->SetMinimumWidth(MINIMUM_TEXT_WIDTH.CP(cv_));

  _tooltip_text->sigTextChanged.connect(sigc::mem_fun(this, &Tooltip::RecvCairoTextChanged));
  _tooltip_text->sigFontChanged.connect(sigc::mem_fun(this, &Tooltip::RecvCairoTextChanged));

  // getter and setter for the property
  text.SetSetterFunction([this](std::string const& newText)
    {
      if(_tooltip_text->GetText() == newText)
        return false;

      _tooltip_text->SetText(newText, true);
      return true;
    }
  );
  text.SetGetterFunction([this]()
    {
      return _tooltip_text->GetText();
    }
  );

  _vlayout->AddView(_tooltip_text.GetPointer(), 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  _vlayout->AddLayout(_bottom_space, 0);

  _hlayout->AddLayout(_left_space, 0);
  _hlayout->AddLayout(_vlayout, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  _hlayout->AddLayout(_right_space, 0);

  SetWindowSizeMatchLayout(true);
  SetLayout(_hlayout);
}

nux::Area* Tooltip::FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type)
{
  // No area under mouse to allow click through to entities below
  return nullptr;
}

void Tooltip::SetTooltipPosition(int tip_x, int tip_y)
{
  _anchorX = tip_x;
  _anchorY = tip_y;

  int x = _anchorX - _padding.CP(cv_);
  int y = _anchorY - ANCHOR_HEIGHT.CP(cv_) / 2 - CORNER_RADIUS.CP(cv_) - _padding.CP(cv_);

  SetBaseXY(x, y);
}

void Tooltip::ShowTooltipWithTipAt(int x, int y)
{
  SetTooltipPosition(x, y);
  Show();
}

void Tooltip::Draw(nux::GraphicsEngine& gfxContext, bool forceDraw)
{
  CairoBaseWindow::Draw(gfxContext, forceDraw);
  _tooltip_text->ProcessDraw(gfxContext, forceDraw);
}

void Tooltip::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{}

void Tooltip::PreLayoutManagement()
{
  int text_width;
  int text_height;
  int text_min_width = MINIMUM_TEXT_WIDTH.CP(cv_);

  _tooltip_text->GetTextExtents(text_width, text_height);

  if (text_width + TEXT_PADDING.CP(cv_) * 2 > text_min_width)
  {
    text_min_width = text_width + TEXT_PADDING.CP(cv_) * 2;
  }

  _tooltip_text->SetMinimumWidth(text_min_width);
  _tooltip_text->SetMinimumHeight(text_height);

  if (text_height < ANCHOR_HEIGHT.CP(cv_))
  {
    _top_space->SetMinMaxSize(1, (ANCHOR_HEIGHT.CP(cv_) - text_height) / 2 + _padding.CP(cv_) + CORNER_RADIUS.CP(cv_));
    _bottom_space->SetMinMaxSize(1, (ANCHOR_HEIGHT.CP(cv_) - text_height) / 2 + 1 + _padding.CP(cv_) + CORNER_RADIUS.CP(cv_));
  }

  CairoBaseWindow::PreLayoutManagement();
}

long Tooltip::PostLayoutManagement(long LayoutResult)
{
  long result = CairoBaseWindow::PostLayoutManagement(LayoutResult);
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
                 gfloat  width,
                 gfloat  height,
                 gfloat  hl_x,
                 gfloat  hl_y,
                 gfloat  hl_size,
                 nux::Color const& tint_color,
                 nux::Color const& hl_color,
                 nux::Color const& dot_color)
{
  cairo_pattern_t* hl_pattern = NULL;

  // clear normal context
  cairo_scale(cr, 1.0f, 1.0f);
  cairo_set_source_rgba(cr, 0.0f, 0.0f, 0.0f, 0.0f);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  // prepare drawing for normal context
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  // create path in normal context
  cairo_rectangle(cr, 0.0f, 0.0f, width, height);

  // fill path of normal context with tint
  cairo_set_source_rgba(cr,
                        tint_color.red,
                        tint_color.green,
                        tint_color.blue,
                        tint_color.alpha);
  cairo_fill_preserve(cr);

  // draw glow
  hl_pattern = cairo_pattern_create_radial(hl_x,
                                           hl_y - hl_size / 1.4f,
                                           0.0f,
                                           hl_x,
                                           hl_y - hl_size / 1.4f,
                                           hl_size);
  cairo_pattern_add_color_stop_rgba(hl_pattern,
                                    0.0f,
                                    hl_color.red,
                                    hl_color.green,
                                    hl_color.blue,
                                    hl_color.alpha);
  cairo_pattern_add_color_stop_rgba(hl_pattern, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f);
  cairo_set_source(cr, hl_pattern);
  cairo_fill(cr);
  cairo_pattern_destroy(hl_pattern);
}

void _setup(cairo_surface_t** surf,
            cairo_t**         cr,
            gboolean          outline,
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
                             gfloat   width,
                             gfloat   height,
                             gint     upper_size,
                             gfloat   radius,
                             guint    pad)
{

  //     0            1 2
  //     +------------+-+
  //    /               + 3
  //   /                |
  //  + 8               |
  //   \                |
  //    \               + 4
  //     +------------+-+
  //     7            6 5

  gfloat padding = pad;

  cairo_translate(cr, -0.5f, -0.5f);

  // create path
  cairo_move_to(cr, padding + anchor_width, padding); // Point 0
  cairo_line_to(cr, width - padding - radius, padding); // Point 1
  cairo_arc(cr,
            width  - padding - radius,
            padding + radius,
            radius,
            -90.0f * G_PI / 180.0f,
            0.0f * G_PI / 180.0f); // Point 3
  cairo_line_to(cr,
                (gdouble) width - padding,
                (gdouble) height - radius - padding); // Point 4
  cairo_arc(cr,
            (gdouble) width - padding - radius,
            (gdouble) height - padding - radius,
            radius,
            0.0f * G_PI / 180.0f,
            90.0f * G_PI / 180.0f); // Point 6
  cairo_line_to(cr,
                anchor_width + padding,
                (gdouble) height - padding); // Point 7

  cairo_line_to(cr,
                padding,
                (gdouble) height / 2.0f); // Point 8

  cairo_close_path(cr);
}

void compute_mask(cairo_t* cr)
{
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_fill_preserve(cr);
}

void compute_outline(cairo_t* cr,
                     gfloat   line_width,
                     nux::Color const& line_color)
{
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgba(cr,
                        line_color.red,
                        line_color.green,
                        line_color.blue,
                        line_color.alpha);
  cairo_set_line_width(cr, line_width);
  cairo_stroke(cr);
}

void _draw(cairo_t* cr,
           gboolean outline,
           gfloat   line_width,
           nux::Color const& color,
           gboolean negative,
           gboolean stroke)
{
  // prepare drawing
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

  // actually draw the mask
  if (outline)
  {
    cairo_set_line_width(cr, line_width);
    cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);
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
               nux::Color const& color,
               gboolean  negative,
               gboolean  stroke)
{
  // prepare drawing
  cairo_set_operator(*cr, CAIRO_OPERATOR_SOURCE);

  // actually draw the mask
  if (outline)
  {
    cairo_set_line_width(*cr, line_width);
    cairo_set_source_rgba(*cr, color.red, color.green, color.blue, color.alpha);
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
  gfloat  width,
  gfloat  height,
  gfloat  anchor_width,
  gfloat  anchor_height,
  gint    upper_size,
  gfloat  corner_radius,
  guint   blur_coeff,
  nux::Color const& shadow_color,
  gfloat  line_width,
  gint    padding_size,
  nux::Color const& line_color)
{
  _setup(&surf, &cr, TRUE, FALSE);
  _compute_full_mask_path(cr,
                          anchor_width,
                          anchor_height,
                          width,
                          height,
                          upper_size,
                          corner_radius,
                          padding_size);

  _draw(cr, TRUE, line_width, shadow_color, FALSE, FALSE);
  nux::CairoGraphics dummy(CAIRO_FORMAT_A1, 1, 1);
  dummy.BlurSurface(blur_coeff, surf);
  compute_mask(cr);
  compute_outline(cr, line_width, line_color);
}

void compute_full_mask(
  cairo_t* cr,
  cairo_surface_t* surf,
  gfloat   width,
  gfloat   height,
  gfloat   radius,
  gfloat   anchor_width,
  gfloat   anchor_height,
  gint     upper_size,
  gboolean negative,
  gboolean outline,
  gfloat   line_width,
  gint     padding_size,
  nux::Color const& color)
{
  _setup(&surf, &cr, outline, negative);
  _compute_full_mask_path(cr,
                          anchor_width,
                          anchor_height,
                          width,
                          height,
                          upper_size,
                          radius,
                          padding_size);
  _finalize(&cr, outline, line_width, color, negative, outline);
}

void Tooltip::UpdateTexture()
{
  if (_cairo_text_has_changed == false)
    return;

  int width = GetBaseWidth();
  int height = GetBaseHeight();

  int x = _anchorX - _padding.CP(cv_);
  int y = _anchorY - height / 2;
  SetXY(x, y);

  auto const& deco_style = decoration::Style::Get();
  float dpi_scale = cv_->DPIScale();
  float blur_coef = std::round(deco_style->ActiveShadowRadius() * dpi_scale / 2.0f);

  nux::CairoGraphics cairo_bg(CAIRO_FORMAT_ARGB32, width, height);
  nux::CairoGraphics cairo_mask(CAIRO_FORMAT_ARGB32, width, height);
  nux::CairoGraphics cairo_outline(CAIRO_FORMAT_ARGB32, width, height);

  cairo_surface_set_device_scale(cairo_bg.GetSurface(), dpi_scale, dpi_scale);
  cairo_surface_set_device_scale(cairo_mask.GetSurface(), dpi_scale, dpi_scale);
  cairo_surface_set_device_scale(cairo_outline.GetSurface(), dpi_scale, dpi_scale);

  cairo_t* cr_bg      = cairo_bg.GetInternalContext();
  cairo_t* cr_mask    = cairo_mask.GetInternalContext();
  cairo_t* cr_outline = cairo_outline.GetInternalContext();

  nux::Color tint_color(0.074f, 0.074f, 0.074f, 0.80f);
  nux::Color hl_color(1.0f, 1.0f, 1.0f, 0.8f);
  nux::Color dot_color(1.0f, 1.0f, 1.0f, 0.20f);
  nux::Color shadow_color(deco_style->ActiveShadowColor());
  nux::Color outline_color(1.0f, 1.0f, 1.0f, 0.15f);
  nux::Color mask_color(1.0f, 1.0f, 1.0f, 1.00f);

  if (!HasBlurredBackground())
  {
    //If low gfx is detected then disable transparency because we're not bluring using our blur anymore.
    tint_color.alpha = 1.0f;
    hl_color.alpha = 1.0f;
    dot_color.alpha = 1.0f;
  }

  tint_dot_hl(cr_bg,
              width / dpi_scale,
              height / dpi_scale,
              width / 2.0f,
              0,
              nux::Max<float>(width / 1.3f, height / 1.3f),
              tint_color,
              hl_color,
              dot_color);

  compute_full_outline_shadow
  (
    cr_outline,
    cairo_outline.GetSurface(),
    width / dpi_scale,
    height / dpi_scale,
    ANCHOR_WIDTH,
    ANCHOR_HEIGHT,
    -1,
    CORNER_RADIUS,
    blur_coef,
    shadow_color,
    1.0f,
    _padding,
    outline_color);

  compute_full_mask(
    cr_mask,
    cairo_mask.GetSurface(),
    width / dpi_scale,
    height / dpi_scale,
    CORNER_RADIUS,         // radius,
    ANCHOR_WIDTH,          // anchor_width,
    ANCHOR_HEIGHT,         // anchor_height,
    -1,                    // upper_size,
    true,                  // negative,
    false,                 // outline,
    1.0,                   // line_width,
    _padding,              // padding_size,
    mask_color);

  texture_bg_ = texture_ptr_from_cairo_graphics(cairo_bg);
  texture_mask_ = texture_ptr_from_cairo_graphics(cairo_mask);
  texture_outline_ = texture_ptr_from_cairo_graphics(cairo_outline);

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

// Introspection

std::string Tooltip::GetName() const
{
  return "ToolTip";
}

void Tooltip::AddProperties(debug::IntrospectionData& introspection)
{
  introspection
    .add("text", text)
    .add("active", IsVisible())
    .add(GetAbsoluteGeometry());
}

} // namespace nux
