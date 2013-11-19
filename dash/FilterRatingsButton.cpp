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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *
 */

#include <math.h>

#include <Nux/Nux.h>
#include <NuxCore/Logger.h>

#include "unity-shared/DashStyle.h"
#include "FilterRatingsButton.h"

namespace
{
const int star_size = 28;
const int star_gap  = 10;
const int num_stars = 5;
}

namespace unity
{
namespace dash
{
  
NUX_IMPLEMENT_OBJECT_TYPE(FilterRatingsButton);

FilterRatingsButton::FilterRatingsButton(NUX_FILE_LINE_DECL)
  : nux::ToggleButton(NUX_FILE_LINE_PARAM)
  , focused_star_(-1)
{
  SetAcceptKeyNavFocusOnMouseDown(false);
  SetAcceptKeyNavFocusOnMouseEnter(true);

  mouse_up.connect(sigc::mem_fun(this, &FilterRatingsButton::RecvMouseUp));
  mouse_move.connect(sigc::mem_fun(this, &FilterRatingsButton::RecvMouseMove));
  mouse_drag.connect(sigc::mem_fun(this, &FilterRatingsButton::RecvMouseDrag));

  key_nav_focus_change.connect([this](nux::Area* area, bool has_focus, nux::KeyNavDirection direction)
  {
    if (has_focus)
      focused_star_ = 0;
    else if (!has_focus)
      focused_star_ = -1;

    QueueDraw();
  });

  key_nav_focus_activate.connect([this](nux::Area*) { filter_->rating = static_cast<float>(focused_star_+1)/num_stars; });
  key_down.connect(sigc::mem_fun(this, &FilterRatingsButton::OnKeyDown));
}

FilterRatingsButton::~FilterRatingsButton()
{
}

void FilterRatingsButton::SetFilter(Filter::Ptr const& filter)
{
  filter_ = std::static_pointer_cast<RatingsFilter>(filter);
  filter_->rating.changed.connect(sigc::mem_fun(this, &FilterRatingsButton::OnRatingsChanged));
  NeedRedraw();
}

std::string FilterRatingsButton::GetFilterType()
{
  return "FilterRatingsButton";
}

void FilterRatingsButton::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  int rating = 0;
  if (filter_ && filter_->filtering)
    rating = static_cast<int>(filter_->rating * num_stars);
  // FIXME: 9/26/2011
  // We should probably support an API for saying whether the ratings
  // should or shouldn't support half stars...but our only consumer at
  // the moment is the applications scope which according to design
  // (Bug #839759) shouldn't. So for now just force rounding.
  //    int total_half_stars = rating % 2;
  //    int total_full_stars = rating / 2;
  int total_full_stars = rating;

  nux::Geometry const& geo = GetGeometry();
  nux::Geometry geo_star(geo);
  geo_star.width = star_size;

  gPainter.PaintBackground(GfxContext, geo);
  // set up our texture mode
  nux::TexCoordXForm texxform;
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

  // clear what is behind us
  unsigned int alpha = 0, src = 0, dest = 0;

  GfxContext.GetRenderStates().GetBlend(alpha, src, dest);
  GfxContext.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  nux::Color col = nux::color::Black;
  col.alpha = 0;
  GfxContext.QRP_Color(geo.x,
                       geo.y,
                       geo.width,
                       geo.height,
                       col);

  for (int index = 0; index < num_stars; ++index)
  {
    Style& style = Style::Instance();
    nux::BaseTexture* texture = style.GetStarSelectedIcon();
    if (index < total_full_stars)
    {
      if (GetVisualState() == nux::ButtonVisualState::VISUAL_STATE_NORMAL)
        texture = style.GetStarSelectedIcon();
      else if (GetVisualState() == nux::ButtonVisualState::VISUAL_STATE_PRELIGHT)
        texture = style.GetStarSelectedIcon();
      else if (GetVisualState() == nux::ButtonVisualState::VISUAL_STATE_PRESSED)
        texture = style.GetStarSelectedIcon();
    }
    else
    {
      if (GetVisualState() == nux::ButtonVisualState::VISUAL_STATE_NORMAL)
        texture = style.GetStarDeselectedIcon();
      else if (GetVisualState() == nux::ButtonVisualState::VISUAL_STATE_PRELIGHT)
        texture = style.GetStarDeselectedIcon();
      else if (GetVisualState() == nux::ButtonVisualState::VISUAL_STATE_PRESSED)
        texture = style.GetStarDeselectedIcon();
    }

    GfxContext.QRP_1Tex(geo_star.x,
                        geo_star.y,
                        geo_star.width,
                        geo_star.height,
                        texture->GetDeviceTexture(),
                        texxform,
                        nux::Color(1.0f, 1.0f, 1.0f, 1.0f));

    if (focused_star_ == index)
    {
      GfxContext.QRP_1Tex(geo_star.x,
                          geo_star.y,
                          geo_star.width,
                          geo_star.height,
                          style.GetStarHighlightIcon()->GetDeviceTexture(),
                          texxform,
                          nux::Color(1.0f, 1.0f, 1.0f, 0.5f));
    }

    geo_star.x += geo_star.width + star_gap;

  }

  GfxContext.GetRenderStates().SetBlend(alpha, src, dest);

}

static void _UpdateRatingToMouse(RatingsFilter::Ptr filter, int x)
{
  int width = 180;
  float new_rating = (static_cast<float>(x) / width);

  // FIXME: change to * 2 once we decide to support also half-stars
  new_rating = ceil((num_stars * 1) * new_rating) / (num_stars * 1);
  new_rating = (new_rating > 1) ? 1 : ((new_rating < 0) ? 0 : new_rating);

  if (filter)
    filter->rating = new_rating;
}

void FilterRatingsButton::RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  _UpdateRatingToMouse(filter_, x);
}

void FilterRatingsButton::RecvMouseDrag(int x, int y, int dx, int dy,
                                        unsigned long button_flags,
                                        unsigned long key_flags)
{
  _UpdateRatingToMouse(filter_, x);
}

void FilterRatingsButton::OnRatingsChanged(int rating)
{
  NeedRedraw();
}

void FilterRatingsButton::RecvMouseMove(int x, int y, int dx, int dy,
                                        unsigned long button_flags,
                                        unsigned long key_flags)
{
  int width = 180;
  focused_star_ = std::max(0, std::min(static_cast<int>(ceil((static_cast<float>(x) / width) * num_stars) - 1), num_stars - 1));

  if (!HasKeyFocus())
    nux::GetWindowCompositor().SetKeyFocusArea(this);

  QueueDraw();
}


bool FilterRatingsButton::InspectKeyEvent(unsigned int eventType, unsigned int keysym, const char* character)
{
  nux::KeyNavDirection direction = nux::KEY_NAV_NONE;

  switch (keysym)
  {
    case NUX_VK_LEFT:
      direction = nux::KeyNavDirection::KEY_NAV_LEFT;
      break;
    case NUX_VK_RIGHT:
      direction = nux::KeyNavDirection::KEY_NAV_RIGHT;
      break;
    default:
      direction = nux::KeyNavDirection::KEY_NAV_NONE;
      break;
  }

  if (direction == nux::KeyNavDirection::KEY_NAV_NONE)
    return false;
  else if (direction == nux::KEY_NAV_LEFT && (focused_star_ <= 0))
    return false;
  else if (direction == nux::KEY_NAV_RIGHT && (focused_star_ >= num_stars - 1))
    return false;
  else
   return true;
}


void FilterRatingsButton::OnKeyDown(unsigned long event_type, unsigned long event_keysym,
                                    unsigned long event_state, const TCHAR* character,
                                    unsigned short key_repeat_count)
{
  switch (event_keysym)
  {
    case NUX_VK_LEFT:
      --focused_star_;
      break;
    case NUX_VK_RIGHT:
      ++focused_star_;
      break;
    default:
      return;
  }

  QueueDraw();
}

bool FilterRatingsButton::AcceptKeyNavFocus()
{
  return true;
}

} // namespace dash
} // namespace unity
