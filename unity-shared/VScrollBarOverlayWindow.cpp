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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include <Nux/Nux.h>
#include <NuxGraphics/CairoGraphics.h>

#include "VScrollBarOverlayWindow.h"
#include "UScreen.h"
#include "UBusMessages.h"
#include "DashStyle.h"
#include "CairoTexture.h"

const int THUMB_WIDTH = 21;
const int THUMB_HEIGHT = 68;
const int THUMB_RADIUS = 3;


VScrollBarOverlayWindow::VScrollBarOverlayWindow(const nux::Geometry& geo)
  : nux::BaseWindow("")
  , content_size_(geo)
  , content_offset_x_(0)
  , mouse_offset_y_(0)
  , mouse_down_(false)
  , mouse_near_(false)
  , inside_slider_(false)
  , current_action_(ThumbAction::NONE)
{
  Area::SetGeometry(content_size_.x, content_size_.y, THUMB_WIDTH, content_size_.height);
  SetBackgroundColor(nux::color::Transparent);

  UpdateTexture();

  _ubus_manager.RegisterInterest(UBUS_OVERLAY_HIDDEN, sigc::mem_fun(this, &VScrollBarOverlayWindow::OnOverlayHidden));
}

VScrollBarOverlayWindow::~VScrollBarOverlayWindow()
{
  if(thumb_texture_)
    thumb_texture_.Release();
}

void VScrollBarOverlayWindow::UpdateGeometry(const nux::Geometry& geo)
{
  content_size_ = geo;
  UpdateMouseOffsetX();
  Area::SetGeometry(content_size_.x + content_offset_x_, content_size_.y, THUMB_WIDTH, content_size_.height);
}

void VScrollBarOverlayWindow::SetThumbOffsetY(int y)
{
  if (mouse_down_)
    MouseDragging();

  mouse_offset_y_ = GetValidOffsetYValue(y);
  QueueDraw();
}

int VScrollBarOverlayWindow::GetValidOffsetYValue(int new_offset) const
{
  if (new_offset < 0)
    return 0;
  else if (new_offset > content_size_.height - THUMB_HEIGHT)
    return content_size_.height - THUMB_HEIGHT;

  return new_offset;
}

void VScrollBarOverlayWindow::UpdateMouseOffsetX()
{
  int monitor = unity::UScreen::GetDefault()->GetMonitorWithMouse();
  const nux::Geometry& geo = unity::UScreen::GetDefault()->GetMonitorGeometry(monitor);

  if (content_size_.x + THUMB_WIDTH > geo.width)
    content_offset_x_ = geo.width - (content_size_.x + THUMB_WIDTH) + 1;
  else
    content_offset_x_ = 0;
}

bool VScrollBarOverlayWindow::IsMouseInsideThumb(int y) const
{
  const nux::Geometry thumb(0, mouse_offset_y_, THUMB_WIDTH, THUMB_HEIGHT);
  if (thumb.IsPointInside(0,y))
    return true;

  return false;
}

bool VScrollBarOverlayWindow::IsMouseBeingDragged() const
{
  return current_action_ == ThumbAction::DRAGGING;
}

int VScrollBarOverlayWindow::GetThumbHeight() const
{
  return THUMB_HEIGHT;
}

int VScrollBarOverlayWindow::GetThumbOffsetY() const
{
  return mouse_offset_y_;
}

nux::Geometry VScrollBarOverlayWindow::GetThumbGeometry() const
{
  return nux::Geometry(content_size_.x + content_offset_x_,
                       content_size_.y + mouse_offset_y_,
                       THUMB_WIDTH, THUMB_HEIGHT);
}

void VScrollBarOverlayWindow::MouseDown()
{
  mouse_down_ = true;
  UpdateTexture();
}

void VScrollBarOverlayWindow::MouseUp()
{
  mouse_down_ = false;
  current_action_ = ThumbAction::NONE;
  UpdateTexture();
  ShouldHide();
}

void VScrollBarOverlayWindow::MouseNear()
{
  mouse_near_ = true;
  ShouldShow();
}

void VScrollBarOverlayWindow::MouseBeyond()
{
  mouse_near_ = false;
  ShouldHide();
}

void VScrollBarOverlayWindow::ThumbInsideSlider()
{
  if (!inside_slider_)
  {
    inside_slider_ = true;
    UpdateTexture();
  }
}

void VScrollBarOverlayWindow::ThumbOutsideSlider()
{
  if (inside_slider_)
  {
    inside_slider_ = false;
    UpdateTexture();
  }
}

void VScrollBarOverlayWindow::PageUpAction()
{
  current_action_ = ThumbAction::PAGE_UP;
  UpdateTexture();
}

void VScrollBarOverlayWindow::PageDownAction()
{
  current_action_ = ThumbAction::PAGE_DOWN;
  UpdateTexture();
}

void VScrollBarOverlayWindow::MouseDragging()
{
  if (current_action_ != ThumbAction::DRAGGING)
  {
    current_action_ = ThumbAction::DRAGGING;
    UpdateTexture();
  }
}

void VScrollBarOverlayWindow::ShouldShow()
{
  if (!IsVisible())
  {
    if (mouse_down_ || mouse_near_)
    {
      ShowWindow(true);
      PushToFront();
      QueueDraw();
    }
  }
}

void VScrollBarOverlayWindow::ShouldHide()
{
  if (IsVisible())
  {
    if (!mouse_down_ && !mouse_near_)
    {
      ShowWindow(false);
      QueueDraw();
    }
  }
}

void VScrollBarOverlayWindow::OnOverlayHidden(GVariant* data)
{
  ResetStates();
}

void VScrollBarOverlayWindow::ResetStates()
{
  mouse_down_ = false;
  mouse_near_ = false;
  current_action_ = ThumbAction::NONE;
  ShouldHide();
}

void VScrollBarOverlayWindow::DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  if (!thumb_texture_)
    return;

  const nux::Color& color = nux::color::White;
  nux::Geometry base(0, mouse_offset_y_, THUMB_WIDTH, THUMB_HEIGHT);

  nux::TexCoordXForm texxform;

  graphics_engine.QRP_1Tex(base.x,
                      base.y,
                      base.width,
                      base.height,
                      thumb_texture_->GetDeviceTexture(),
                      texxform,
                      color);
}

nux::color::RedGreenBlue ProduceColorShade(const nux::color::RedGreenBlue& rgb, float shade)
{
  if (shade == 1.0f)
    return rgb;

  nux::color::HueLightnessSaturation hls(rgb);

  hls.lightness *= shade;
  if (hls.lightness > 1.0f)
    hls.lightness = 1.0f;
  else if (hls.lightness < 0.0f)
    hls.lightness = 0.0f;

  hls.saturation *= shade;
  if (hls.saturation > 1.0f)
    hls.saturation = 1.0f;
  else if (hls.saturation < 0.0f)
    hls.saturation = 0.0f;

  nux::color::RedGreenBlue rgb_shade(hls);

  return rgb_shade;
}

void PatternAddRGBStop(cairo_pattern_t* pat, const nux::color::RedGreenBlue& rgb, double stop, float alpha)
{
  cairo_pattern_add_color_stop_rgba (pat, stop, rgb.red, rgb.green, rgb.blue, alpha);
}

void SetSourceRGB(cairo_t* cr, const nux::color::RedGreenBlue& rgb, float alpha)
{
  cairo_set_source_rgba(cr, rgb.red, rgb.green, rgb.blue, alpha);
}

void DrawGrip (cairo_t* cr, double x, double y, int nx, int ny)
{
  gint lx, ly;

  for (ly = 0; ly < ny; ly++)
  {
    for (lx = 0; lx < nx; lx++)
    {
      gint sx = lx * 3;
      gint sy = ly * 3;

      cairo_rectangle (cr, x + sx, y + sy, 1, 1);
    }
  }
}

void DrawBothGrips(cairo_t* cr, const nux::color::RedGreenBlue& rgb, int width, int height)
{
  cairo_pattern_t* pat;

  pat = cairo_pattern_create_linear(0, 0, 0, height);

  PatternAddRGBStop(pat, rgb, 0.0, 0.0);
  PatternAddRGBStop(pat, rgb, 0.49, 0.5);
  PatternAddRGBStop(pat, rgb, 0.49, 0.5);
  PatternAddRGBStop(pat, rgb, 1.0, 0.0);

  cairo_set_source(cr, pat);
  cairo_pattern_destroy(pat);

  DrawGrip(cr, width/2 - 6.5, 13.5, 5, 6);
  DrawGrip(cr, width/2 - 6.5, height/2 + 3.5, 5, 6);

  cairo_fill(cr);
}

void DrawLineSeperator(cairo_t* cr, const nux::color::RedGreenBlue& top, 
                       const nux::color::RedGreenBlue& bottom, int width, int height)
{
  // Top
  cairo_move_to(cr, 1.5, height/2);
  cairo_line_to(cr, width - 1.5, height/2);
  SetSourceRGB(cr, top, 0.36);
  cairo_stroke(cr);

  // Bottom
  cairo_move_to(cr, 1.5, 1 + height/2);
  cairo_line_to(cr, width - 1.5, 1 + height/2);
  SetSourceRGB(cr, bottom, 0.5);
  cairo_stroke(cr);
}


void DrawArrow (cairo_t* cr, const nux::color::RedGreenBlue& rgb, double x, double y, double width, double height)
{
  cairo_save (cr);

  cairo_translate (cr, x, y);
  cairo_move_to (cr, -width / 2, -height / 2);
  cairo_line_to (cr, 0, height / 2);
  cairo_line_to (cr, width / 2, -height / 2);
  cairo_close_path (cr);

  SetSourceRGB(cr, rgb, 0.75);
  cairo_fill_preserve (cr);

  SetSourceRGB(cr, rgb, 1.0);
  cairo_stroke (cr);

  cairo_restore (cr);
}

void DrawBothArrows(cairo_t* cr, int width, int height)
{
  const nux::color::RedGreenBlue arrow_color(0.3f, 0.3f, 0.3f);

  // Top
  cairo_save(cr);
  cairo_translate(cr, width/2 + 0.5, 8.5);
  cairo_rotate(cr, G_PI);
  DrawArrow(cr, arrow_color, 0.5, 0, 5, 3);
  cairo_restore(cr);

  // Bottom
  cairo_save(cr);
  cairo_translate(cr, width/2 + 0.5, height - 8.5);
  cairo_rotate(cr, 0);
  DrawArrow(cr, arrow_color, -0.5, 0, 5, 3);
  cairo_restore(cr);
}

void VScrollBarOverlayWindow::UpdateTexture()
{
  int width  = THUMB_WIDTH;
  int height = THUMB_HEIGHT;
  int radius = THUMB_RADIUS;

  cairo_t*            cr            = NULL;
  cairo_pattern_t*    pat           = NULL;

  const nux::color::RedGreenBlue bg(0.91f, 0.90f, 0.90f);
  const nux::color::RedGreenBlue bg_selected(0.95f, 0.95f, 0.95f);
  const nux::color::RedGreenBlue bg_active(0.55f, 0.55f, 0.55f);

  nux::CairoGraphics cairoGraphics(CAIRO_FORMAT_ARGB32, width, height);
  cr = cairoGraphics.GetContext();

  cairo_save(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  cairo_save(cr);

  cairo_translate (cr, 0.5, 0.5);
  width--;
  height--;

  cairo_set_line_width (cr, 1.0);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_save(cr);

  // Draw backgound
  SetSourceRGB(cr, bg, 1.0);
  cairoGraphics.DrawRoundedRectangle(cr, 1.0f, 0.0, 0.0, radius, width, height);
  cairo_fill_preserve(cr);

  // Draw shaded background
  pat = cairo_pattern_create_linear(0, 0, 0, height);

  const nux::color::RedGreenBlue& bg_arrow_up = ProduceColorShade(bg, 0.86);
  const nux::color::RedGreenBlue& bg_arrow_down = ProduceColorShade(bg, 1.1);

  PatternAddRGBStop(pat, bg_arrow_up, 0.0, 0.8);
  PatternAddRGBStop(pat, bg_arrow_down, 1.0, 0.8);

  cairo_set_source(cr, pat);
  cairo_pattern_destroy(pat);

  if (current_action_ == ThumbAction::DRAGGING)
  {
    cairo_fill_preserve(cr);
    SetSourceRGB(cr, bg, 0.8);
    cairo_fill(cr);
  }
  else
  {
    cairo_fill(cr);
  }

  // Draw Page Up/Down Action
  if (current_action_ == ThumbAction::PAGE_UP ||
      current_action_ == ThumbAction::PAGE_DOWN)
  {
    if (current_action_ == ThumbAction::PAGE_UP)
      cairo_rectangle(cr, 0, 0, width, height/2);
    else
      cairo_rectangle(cr, 0, height/2, width, height/2);

    SetSourceRGB(cr, bg, 0.8);
    cairo_fill(cr);
  }

  cairo_save(cr);

  // Draw Outline
  cairo_set_line_width (cr, 2.0);
  cairoGraphics.DrawRoundedRectangle(cr, 1.0f, 0.5, 0.5, radius - 1, width - 1, height - 1);

  if (inside_slider_)
    SetSourceRGB(cr, bg_selected, 1.0);
  else
    SetSourceRGB(cr, bg_active, 0.9);

  cairo_stroke(cr);

  cairo_restore(cr);

  // Draw shade outline
  pat = cairo_pattern_create_linear(0, 0, 0, height);

  const nux::color::RedGreenBlue& bg_shadow = ProduceColorShade(bg, 0.2);
  PatternAddRGBStop(pat, bg_shadow, 0.5, 0.06);

  switch(current_action_)
  {
    case ThumbAction::NONE:
      PatternAddRGBStop(pat, bg_shadow, 0.0, 0.22);
      PatternAddRGBStop(pat, bg_shadow, 1.0, 0.22);
      break;
    case ThumbAction::DRAGGING:
      PatternAddRGBStop(pat, bg_shadow, 0.0, 0.2);
      PatternAddRGBStop(pat, bg_shadow, 1.0, 0.2);
      break;
    case ThumbAction::PAGE_UP:
      PatternAddRGBStop(pat, bg_shadow, 0.0, 0.1);
      PatternAddRGBStop(pat, bg_shadow, 1.0, 0.22);
      break;
    case ThumbAction::PAGE_DOWN:
      PatternAddRGBStop(pat, bg_shadow, 0.0, 0.22);
      PatternAddRGBStop(pat, bg_shadow, 1.0, 0.1);
      break;
    default:
      break;
  }

  cairo_set_source(cr, pat);
  cairo_pattern_destroy(pat);

  cairoGraphics.DrawRoundedRectangle(cr, 1.0f, 1.0, 1.0, radius, width- 2, height - 2);
  cairo_stroke(cr);

  const nux::color::RedGreenBlue& bg_dark_line = ProduceColorShade(bg, 0.4);
  const nux::color::RedGreenBlue& bg_bright_line = ProduceColorShade(bg, 1.2);

  cairoGraphics.DrawRoundedRectangle(cr, 1.0f, 2.0, 2.0, radius - 1, width - 4, height- 4);
  SetSourceRGB(cr, bg_bright_line, 0.6);
  cairo_stroke(cr);

  DrawBothGrips(cr, bg_dark_line, width, height);
  DrawLineSeperator(cr, bg_dark_line, bg_bright_line, width, height);
  DrawBothArrows(cr, width, height);
  
  thumb_texture_.Adopt(unity::texture_from_cairo_graphics(cairoGraphics));

  cairo_destroy(cr);

  QueueDraw();
}
