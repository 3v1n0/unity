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
#include "DashStyle.h"
#include "CairoTexture.h"
#include "unity-shared/AnimationUtils.h"

namespace unity
{

namespace
{
  int const THUMB_WIDTH = 21;
  int const THUMB_HEIGHT = 68;
  int const THUMB_RADIUS = 3;
  int const ANIMATION_DURATION = 90;
}


VScrollBarOverlayWindow::VScrollBarOverlayWindow(nux::Geometry const& geo)
  : nux::BaseWindow("")
  , content_size_(geo)
  , content_offset_x_(0)
  , mouse_offset_y_(0)
  , current_state_(ThumbState::NONE)
  , current_action_(ThumbAction::NONE)
  , show_animator_(ANIMATION_DURATION)
{
  Area::SetGeometry(content_size_.x, content_size_.y, THUMB_WIDTH, content_size_.height);
  SetBackgroundColor(nux::color::Transparent);

  show_animator_.updated.connect(sigc::mem_fun(this, &BaseWindow::SetOpacity));
  show_animator_.finished.connect([this] {
    if (animation::GetDirection(show_animator_) == animation::Direction::BACKWARD)
      ShowWindow(false);
  });

  SetOpacity(0.0f);
  UpdateTexture();
}

void VScrollBarOverlayWindow::UpdateGeometry(nux::Geometry const& geo)
{
  if (content_size_.x != geo.x ||
      content_size_.y != geo.y ||
      content_size_.height != geo.height)
  {
    content_size_ = geo;
    UpdateMouseOffsetX();

    Area::SetGeometry(content_size_.x + content_offset_x_, content_size_.y, THUMB_WIDTH, content_size_.height);
  }
}

void VScrollBarOverlayWindow::SetThumbOffsetY(int y)
{
  int const new_offset = GetValidOffsetYValue(y);

  if (new_offset != mouse_offset_y_)
  {
    if (HasState(ThumbState::MOUSE_DOWN))
      MouseDragging();

    mouse_offset_y_ = new_offset;
    QueueDraw();
  }
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
  nux::Geometry const& geo = unity::UScreen::GetDefault()->GetMonitorGeometry(monitor);

  if (content_size_.x + THUMB_WIDTH > geo.x + geo.width)
    content_offset_x_ = geo.x + geo.width - (content_size_.x + THUMB_WIDTH);
  else
    content_offset_x_ = 0;
}

bool VScrollBarOverlayWindow::IsMouseInsideThumb(int y) const
{
  nux::Geometry const thumb(0, mouse_offset_y_, THUMB_WIDTH, THUMB_HEIGHT);
  return thumb.IsPointInside(0,y);
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
  AddState(ThumbState::MOUSE_DOWN);
  UpdateTexture();
}

void VScrollBarOverlayWindow::MouseUp()
{
  RemoveState(ThumbState::MOUSE_DOWN);
  current_action_ = ThumbAction::NONE;
  UpdateTexture();
  ShouldHide();
}

void VScrollBarOverlayWindow::MouseNear()
{
  AddState(ThumbState::MOUSE_NEAR);
  ShouldShow();
}

void VScrollBarOverlayWindow::MouseBeyond()
{
  RemoveState(ThumbState::MOUSE_NEAR);
  ShouldHide();
}

void VScrollBarOverlayWindow::MouseEnter()
{
  AddState(ThumbState::MOUSE_INSIDE);
  ShouldShow();
}

void VScrollBarOverlayWindow::MouseLeave()
{
  RemoveState(ThumbState::MOUSE_INSIDE);
  ShouldHide();
}

void VScrollBarOverlayWindow::ThumbInsideSlider()
{
  if (!HasState(ThumbState::INSIDE_SLIDER))
  {
    AddState(ThumbState::INSIDE_SLIDER);
    UpdateTexture();
  }
}

void VScrollBarOverlayWindow::ThumbOutsideSlider()
{
  if (HasState(ThumbState::INSIDE_SLIDER))
  {
    RemoveState(ThumbState::INSIDE_SLIDER);
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
    if (HasState(ThumbState::MOUSE_DOWN) ||
        HasState(ThumbState::MOUSE_NEAR))
    {
      ShowWindow(true);
      PushToFront();
      animation::StartOrReverse(show_animator_, animation::Direction::FORWARD);
    }
  }
}

void VScrollBarOverlayWindow::ShouldHide()
{
  if (IsVisible())
  {
    if (!(HasState(ThumbState::MOUSE_DOWN)) &&
        !(HasState(ThumbState::MOUSE_NEAR)) &&
        !(HasState(ThumbState::MOUSE_INSIDE)))
    {
      animation::StartOrReverse(show_animator_, animation::Direction::BACKWARD);
    }
  }
}

void VScrollBarOverlayWindow::ResetStates()
{
  current_state_ = ThumbState::NONE;
  current_action_ = ThumbAction::NONE;
  ShouldHide();
}

void VScrollBarOverlayWindow::AddState(ThumbState const& state)
{
  current_state_ |= state;
}

void VScrollBarOverlayWindow::RemoveState(ThumbState const& state)
{
  current_state_ &= ~(state);
}

bool VScrollBarOverlayWindow::HasState(ThumbState const& state) const
{
  return (current_state_ & state);
}

void VScrollBarOverlayWindow::Draw(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  if (!thumb_texture_)
    return;

  nux::Geometry base(0, mouse_offset_y_, THUMB_WIDTH, THUMB_HEIGHT);
  nux::TexCoordXForm texxform;

  graphics_engine.QRP_1Tex(base.x,
                           base.y,
                           base.width,
                           base.height,
                           thumb_texture_->GetDeviceTexture(),
                           texxform,
                           nux::color::White);
}

nux::color::RedGreenBlue ProduceColorShade(nux::color::RedGreenBlue const& rgb, float shade)
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

void PatternAddRGBStop(cairo_pattern_t* pat, nux::color::RedGreenBlue const& rgb, double stop, float alpha)
{
  cairo_pattern_add_color_stop_rgba (pat, stop, rgb.red, rgb.green, rgb.blue, alpha);
}

void SetSourceRGB(cairo_t* cr, nux::color::RedGreenBlue const& rgb, float alpha)
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

void DrawBothGrips(cairo_t* cr, nux::color::RedGreenBlue const& rgb, int width, int height)
{
  int const grip_width = 5;
  int const grip_height = 6;
  float const grip_y = 13.5;
  float const offset = 6.5;

  cairo_pattern_t* pat;
  pat = cairo_pattern_create_linear(0, 0, 0, height);

  PatternAddRGBStop(pat, rgb, 0.0, 0.0);
  PatternAddRGBStop(pat, rgb, 0.49, 0.5);
  PatternAddRGBStop(pat, rgb, 0.49, 0.5);
  PatternAddRGBStop(pat, rgb, 1.0, 0.0);

  cairo_set_source(cr, pat);
  cairo_pattern_destroy(pat);

  DrawGrip(cr, width/2 - offset, grip_y, grip_width, grip_height);
  DrawGrip(cr, width/2 - offset, height/2 + (grip_y - 10), grip_width, grip_height);

  cairo_fill(cr);
}

void DrawLineSeperator(cairo_t* cr, nux::color::RedGreenBlue const& top,
                       nux::color::RedGreenBlue const& bottom, int width, int height)
{
  int const offset = 1.5;

  // Top
  cairo_move_to(cr, offset, height/2);
  cairo_line_to(cr, width - offset, height/2);
  SetSourceRGB(cr, top, 0.36);
  cairo_stroke(cr);

  // Bottom
  cairo_move_to(cr, offset, 1 + height/2);
  cairo_line_to(cr, width - offset, 1 + height/2);
  SetSourceRGB(cr, bottom, 0.5);
  cairo_stroke(cr);
}


void DrawArrow (cairo_t* cr, nux::color::RedGreenBlue const& rgb, double x, double y, double width, double height)
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

void DrawBothArrows(cairo_t* cr, nux::color::RedGreenBlue const& rgb, int width, int height)
{
  int const arrow_width = 5;
  int const arrow_height = 3;
  float const trans_height = 8.5;
  float const offset_x = 0.5;

  // Top
  cairo_save(cr);
  cairo_translate(cr, width/2 + offset_x, trans_height);
  cairo_rotate(cr, G_PI);
  DrawArrow(cr, rgb, offset_x, 0, arrow_width, arrow_height);
  cairo_restore(cr);

  // Bottom
  cairo_save(cr);
  cairo_translate(cr, width/2 + offset_x, height - trans_height);
  cairo_rotate(cr, 0);
  DrawArrow(cr, rgb, -offset_x, 0, arrow_width, arrow_height);
  cairo_restore(cr);
}

void VScrollBarOverlayWindow::UpdateTexture()
{
  int width  = THUMB_WIDTH;
  int height = THUMB_HEIGHT;
  int radius = THUMB_RADIUS;

  float const aspect = 1.0f;
  float current_x = 0.0f;
  float current_y = 0.0f;

  cairo_t*            cr            = NULL;
  cairo_pattern_t*    pat           = NULL;

  nux::color::RedGreenBlue const& bg = nux::color::WhiteSmoke;
  nux::color::RedGreenBlue const& bg_selected = nux::color::White;
  nux::color::RedGreenBlue const& bg_active = nux::color::Gray;
  nux::color::RedGreenBlue const& arrow_color = nux::color::DarkSlateGray;

  nux::color::RedGreenBlue const& bg_arrow_up = ProduceColorShade(bg, 0.86);
  nux::color::RedGreenBlue const& bg_arrow_down = ProduceColorShade(bg, 1.1);
  nux::color::RedGreenBlue const& bg_shadow = ProduceColorShade(bg, 0.2);

  nux::color::RedGreenBlue const& bg_dark_line = ProduceColorShade(bg, 0.4);
  nux::color::RedGreenBlue const& bg_bright_line = ProduceColorShade(bg, 1.2);

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
  cairoGraphics.DrawRoundedRectangle(cr, aspect, current_x, current_y, radius, width, height);
  cairo_fill_preserve(cr);

  // Draw shaded background
  pat = cairo_pattern_create_linear(0, 0, 0, height);

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

  current_x += 0.5;
  current_y += 0.5;
  cairoGraphics.DrawRoundedRectangle(cr, aspect, current_x, current_y, radius - 1, width - 1, height - 1);

  if (HasState(ThumbState::INSIDE_SLIDER))
    SetSourceRGB(cr, bg_selected, 1.0);
  else
    SetSourceRGB(cr, bg_active, 0.9);

  cairo_stroke(cr);

  cairo_restore(cr);

  // Draw shade outline
  pat = cairo_pattern_create_linear(0, 0, 0, height);

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

  current_x += 0.5;
  current_y += 0.5;
  cairoGraphics.DrawRoundedRectangle(cr, aspect, current_x, current_y, radius, width- 2, height - 2);
  cairo_stroke(cr);

  current_x += 1.0;
  current_y += 1.0;
  cairoGraphics.DrawRoundedRectangle(cr, aspect, current_x, current_y, radius - 1, width - 4, height- 4);
  SetSourceRGB(cr, bg_bright_line, 0.6);
  cairo_stroke(cr);

  DrawBothGrips(cr, bg_dark_line, width, height);
  DrawLineSeperator(cr, bg_dark_line, bg_bright_line, width, height);
  DrawBothArrows(cr, arrow_color, width, height);

  thumb_texture_.Adopt(unity::texture_from_cairo_graphics(cairoGraphics));
  cairo_destroy(cr);

  QueueDraw();
}

} // namespace unity
