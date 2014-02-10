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

#include "unity-shared/CairoTexture.h"
#include <unity-shared/PanelStyle.h>
#include <unity-shared/UnitySettings.h>

#include "Tooltip.h"

namespace unity
{
namespace
{
  const RawPixel ANCHOR_WIDTH       =  14_em;
  const RawPixel ANCHOR_HEIGHT      =  18_em;
  const RawPixel CORNER_RADIUS      =   4_em;
  const RawPixel PADDING            =  15_em;
  const RawPixel TEXT_PADDING       =   8_em;
  const RawPixel MINIMUM_TEXT_WIDTH = 100_em;
}

NUX_IMPLEMENT_OBJECT_TYPE(Tooltip);
Tooltip::Tooltip(int monitor) :
  _anchorX(0),
  _anchorY(0),
  _monitor(monitor),
  _cairo_text_has_changed(true),
  _cv(unity::Settings::Instance().em(monitor))
{
  _hlayout = new nux::HLayout(TEXT(""), NUX_TRACKER_LOCATION);
  _vlayout = new nux::VLayout(TEXT(""), NUX_TRACKER_LOCATION);

  _left_space = new nux::SpaceLayout(PADDING.CP(_cv) + ANCHOR_WIDTH.CP(_cv), PADDING.CP(_cv) + ANCHOR_WIDTH.CP(_cv), 1, 1000);
  _right_space = new nux::SpaceLayout(PADDING.CP(_cv) + CORNER_RADIUS.CP(_cv), PADDING.CP(_cv) + CORNER_RADIUS.CP(_cv), 1, 1000);

  _top_space = new nux::SpaceLayout(1, 1000, PADDING.CP(_cv), PADDING.CP(_cv));
  _bottom_space = new nux::SpaceLayout(1, 1000, PADDING.CP(_cv), PADDING.CP(_cv));

  _vlayout->AddLayout(_top_space, 0);

  _tooltip_text = new StaticCairoText(TEXT(""), NUX_TRACKER_LOCATION);
  _tooltip_text->SetTextAlignment(StaticCairoText::AlignState::NUX_ALIGN_CENTRE);
  _tooltip_text->SetTextVerticalAlignment(StaticCairoText::AlignState::NUX_ALIGN_CENTRE);
  _tooltip_text->SetMinimumWidth(MINIMUM_TEXT_WIDTH.CP(_cv));

  _tooltip_text->sigTextChanged.connect(sigc::mem_fun(this, &Tooltip::RecvCairoTextChanged));
  _tooltip_text->sigFontChanged.connect(sigc::mem_fun(this, &Tooltip::RecvCairoTextChanged));

  // getter and setter for the property
  text.SetSetterFunction([this](std::string newText)
    {
      if(_tooltip_text->GetText() == newText)
        return false;

      _tooltip_text->SetText(newText);
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

  int x = _anchorX - PADDING.CP(_cv);
  int y = _anchorY - ANCHOR_HEIGHT.CP(_cv) / 2 - CORNER_RADIUS.CP(_cv) - PADDING.CP(_cv);

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
  int text_min_width = MINIMUM_TEXT_WIDTH.CP(_cv);

  _tooltip_text->GetTextExtents(text_width, text_height);

  if (text_width + TEXT_PADDING.CP(_cv) * 2 > text_min_width)
  {
    text_min_width = text_width + TEXT_PADDING.CP(_cv) * 2;
  }

  _tooltip_text->SetMinimumWidth(text_min_width);
  _tooltip_text->SetMinimumHeight(text_height);

  if (text_height < ANCHOR_HEIGHT.CP(_cv))
  {
    _top_space->SetMinMaxSize(1, (ANCHOR_HEIGHT.CP(_cv) - text_height) / 2 + PADDING.CP(_cv) + CORNER_RADIUS.CP(_cv));
    _bottom_space->SetMinMaxSize(1, (ANCHOR_HEIGHT.CP(_cv) - text_height) / 2 + 1 + PADDING.CP(_cv) + CORNER_RADIUS.CP(_cv));
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
                 gint    width,
                 gint    height,
                 gfloat  hl_x,
                 gfloat  hl_y,
                 gfloat  hl_size,
                 gfloat* rgba_tint,
                 gfloat* rgba_hl,
                 gfloat* rgba_dot)
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
  cairo_rectangle(cr, 0.0f, 0.0f, (gdouble) width, (gdouble) height);

  // fill path of normal context with tint
  cairo_set_source_rgba(cr,
                        rgba_tint[0],
                        rgba_tint[1],
                        rgba_tint[2],
                        rgba_tint[3]);
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
                                    rgba_hl[0],
                                    rgba_hl[1],
                                    rgba_hl[2],
                                    rgba_hl[3]);
  cairo_pattern_add_color_stop_rgba(hl_pattern, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f);
  cairo_set_source(cr, hl_pattern);
  cairo_fill(cr);
  cairo_pattern_destroy(hl_pattern);
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
  nux::CairoGraphics dummy(CAIRO_FORMAT_A1, 1, 1);
  dummy.BlurSurface(blur_coeff, surf);
  compute_mask(cr);
  compute_outline(cr, line_width, rgba_line);
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

  int width = GetBaseWidth();
  int height = GetBaseHeight();

  int x = _anchorX - PADDING.CP(_cv);
  int y = _anchorY - height / 2;

  float blur_coef = 6.0f;

  SetBaseX(x);
  SetBaseY(y);

  nux::CairoGraphics cairo_bg(CAIRO_FORMAT_ARGB32, width, height);
  nux::CairoGraphics cairo_mask(CAIRO_FORMAT_ARGB32, width, height);
  nux::CairoGraphics cairo_outline(CAIRO_FORMAT_ARGB32, width, height);

  cairo_t* cr_bg      = cairo_bg.GetContext();
  cairo_t* cr_mask    = cairo_mask.GetContext();
  cairo_t* cr_outline = cairo_outline.GetContext();

  float   tint_color[4]    = {0.074f, 0.074f, 0.074f, 0.80f};
  float   hl_color[4]      = {1.0f, 1.0f, 1.0f, 0.8f};
  float   dot_color[4]     = {1.0f, 1.0f, 1.0f, 0.20f};
  float   shadow_color[4]  = {0.0f, 0.0f, 0.0f, 1.00f};
  float   outline_color[4] = {1.0f, 1.0f, 1.0f, 0.15f};
  float   mask_color[4]    = {1.0f, 1.0f, 1.0f, 1.00f};

  if (!HasBlurredBackground())
  {
    //If low gfx is detected then disable transparency because we're not bluring using our blur anymore.
    const float alpha_value = 1.0f;

    tint_color[3] = alpha_value;
    hl_color[3] = alpha_value;
    dot_color[3] = alpha_value;
  }

  tint_dot_hl(cr_bg,
              width,
              height,
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
    width,
    height,
    ANCHOR_WIDTH.CP(_cv),
    ANCHOR_HEIGHT.CP(_cv),
    -1,
    CORNER_RADIUS.CP(_cv),
    blur_coef,
    shadow_color,
    1.0f,
    PADDING.CP(_cv),
    outline_color);

  compute_full_mask(
    cr_mask,
    cairo_mask.GetSurface(),
    width,
    height,
    CORNER_RADIUS.CP(_cv), // radius,
    RawPixel(16).CP(_cv),  // shadow_radius,
    ANCHOR_WIDTH.CP(_cv),  // anchor_width,
    ANCHOR_HEIGHT.CP(_cv), // anchor_height,
    -1,                    // upper_size,
    true,                  // negative,
    false,                 // outline,
    1.0,                   // line_width,
    PADDING.CP(_cv),       // padding_size,
    mask_color);

  cairo_destroy(cr_bg);
  cairo_destroy(cr_outline);
  cairo_destroy(cr_mask);

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
