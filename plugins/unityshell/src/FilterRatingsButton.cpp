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

#include "DashStyle.h"
#include "FilterRatingsButton.h"

namespace unity
{
namespace dash
{

FilterRatingsButton::FilterRatingsButton(NUX_FILE_LINE_DECL)
  : nux::ToggleButton(NUX_FILE_LINE_PARAM)
{
  InitTheme();
  SetAcceptKeyNavFocusOnMouseDown(false);

  mouse_up.connect(sigc::mem_fun(this, &FilterRatingsButton::RecvMouseUp));
  mouse_drag.connect(sigc::mem_fun(this, &FilterRatingsButton::RecvMouseDrag));
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

void FilterRatingsButton::InitTheme()
{
  if (!active_empty_)
  {
    nux::Geometry geometry(GetGeometry());
    geometry.width /= 5;

    active_empty_.reset(new nux::CairoWrapper(geometry, sigc::bind(sigc::mem_fun(this, &FilterRatingsButton::RedrawTheme), 0, nux::ButtonVisualState::VISUAL_STATE_PRESSED)));
    normal_empty_.reset(new nux::CairoWrapper(geometry, sigc::bind(sigc::mem_fun(this, &FilterRatingsButton::RedrawTheme), 0, nux::ButtonVisualState::VISUAL_STATE_NORMAL)));
    prelight_empty_.reset(new nux::CairoWrapper(geometry, sigc::bind(sigc::mem_fun(this, &FilterRatingsButton::RedrawTheme), 0, nux::ButtonVisualState::VISUAL_STATE_PRELIGHT)));

    active_half_.reset(new nux::CairoWrapper(geometry, sigc::bind(sigc::mem_fun(this, &FilterRatingsButton::RedrawTheme), 1, nux::ButtonVisualState::VISUAL_STATE_PRESSED)));
    normal_half_.reset(new nux::CairoWrapper(geometry, sigc::bind(sigc::mem_fun(this, &FilterRatingsButton::RedrawTheme), 1, nux::ButtonVisualState::VISUAL_STATE_NORMAL)));
    prelight_half_.reset(new nux::CairoWrapper(geometry, sigc::bind(sigc::mem_fun(this, &FilterRatingsButton::RedrawTheme), 1, nux::ButtonVisualState::VISUAL_STATE_PRELIGHT)));

    active_full_.reset(new nux::CairoWrapper(geometry, sigc::bind(sigc::mem_fun(this, &FilterRatingsButton::RedrawTheme), 2, nux::ButtonVisualState::VISUAL_STATE_PRESSED)));
    normal_full_.reset(new nux::CairoWrapper(geometry, sigc::bind(sigc::mem_fun(this, &FilterRatingsButton::RedrawTheme), 2, nux::ButtonVisualState::VISUAL_STATE_NORMAL)));
    prelight_full_.reset(new nux::CairoWrapper(geometry, sigc::bind(sigc::mem_fun(this, &FilterRatingsButton::RedrawTheme), 2, nux::ButtonVisualState::VISUAL_STATE_PRELIGHT)));
  }
}

void FilterRatingsButton::RedrawTheme(nux::Geometry const& geom, cairo_t* cr, int type, nux::ButtonVisualState faked_state)
{
  Style& dash_style = Style::Instance();
  if (type == 0)
  {
    // empty
    dash_style.StarEmpty(cr, faked_state);
  }
  else if (type == 1)
  {
    // half
    dash_style.StarHalf(cr, faked_state);
  }
  else
  {
    // full
    dash_style.StarFull(cr, faked_state);
  }
}

long FilterRatingsButton::ComputeContentSize()
{
  long ret = nux::Button::ComputeContentSize();
  nux::Geometry geo(GetGeometry());

  if (cached_geometry_ != geo)
  {
    geo.width = 27;
    active_empty_->Invalidate(geo);
    normal_empty_->Invalidate(geo);
    prelight_empty_->Invalidate(geo);

    active_half_->Invalidate(geo);
    normal_half_->Invalidate(geo);
    prelight_half_->Invalidate(geo);

    active_full_->Invalidate(geo);
    normal_full_->Invalidate(geo);
    prelight_full_->Invalidate(geo);

    cached_geometry_ = geo;
  }

  return ret;
}

void FilterRatingsButton::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  int rating = 0;
  if (filter_ && filter_->filtering)
    rating = static_cast<int>(filter_->rating * 5);
  // FIXME: 9/26/2011
  // We should probably support an API for saying whether the ratings
  // should or shouldn't support half stars...but our only consumer at
  // the moment is the applications lens which according to design
  // (Bug #839759) shouldn't. So for now just force rounding.
  //    int total_half_stars = rating % 2;
  //    int total_full_stars = rating / 2;
  int total_full_stars = rating;
  int total_half_stars = 0;

  nux::Geometry const& geo = GetGeometry();
  nux::Geometry geo_star(geo);
  geo_star.width = 27;

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

  for (int index = 0; index < 5; index++)
  {
    nux::BaseTexture* texture = normal_empty_->GetTexture();
    if (index < total_full_stars)
    {
      if (GetVisualState() == nux::ButtonVisualState::VISUAL_STATE_NORMAL)
        texture = normal_full_->GetTexture();
      else if (GetVisualState() == nux::ButtonVisualState::VISUAL_STATE_PRELIGHT)
        texture = prelight_full_->GetTexture();
      else if (GetVisualState() == nux::ButtonVisualState::VISUAL_STATE_PRESSED)
        texture = active_full_->GetTexture();
    }
    else if (index < total_full_stars + total_half_stars)
    {
      if (GetVisualState() == nux::ButtonVisualState::VISUAL_STATE_NORMAL)
        texture = normal_half_->GetTexture();
      else if (GetVisualState() == nux::ButtonVisualState::VISUAL_STATE_PRELIGHT)
        texture = prelight_half_->GetTexture();
      else if (GetVisualState() == nux::ButtonVisualState::VISUAL_STATE_PRESSED)
        texture = active_half_->GetTexture();
    }
    else
    {
      if (GetVisualState() == nux::ButtonVisualState::VISUAL_STATE_NORMAL)
        texture = normal_empty_->GetTexture();
      else if (GetVisualState() == nux::ButtonVisualState::VISUAL_STATE_PRELIGHT)
        texture = prelight_empty_->GetTexture();
      else if (GetVisualState() == nux::ButtonVisualState::VISUAL_STATE_PRESSED)
        texture = active_empty_->GetTexture();
    }

    GfxContext.QRP_1Tex(geo_star.x,
                        geo_star.y,
                        geo_star.width,
                        geo_star.height,
                        texture->GetDeviceTexture(),
                        texxform,
                        nux::Color(1.0f, 1.0f, 1.0f, 1.0f));

    geo_star.x += geo_star.width + 10;

  }

  GfxContext.GetRenderStates().SetBlend(alpha, src, dest);

}

static void _UpdateRatingToMouse(RatingsFilter::Ptr filter, int x)
{
  int width = 180;
  float new_rating = (static_cast<float>(x) / width);

  // FIXME: change to 10 once we decide to support also half-stars
  new_rating = ceil(5 * new_rating) / 5;
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

} // namespace dash
} // namespace unity
