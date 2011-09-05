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
#include <NuxCore/Logger.h>

#include "FilterRatingsButton.h"
#include "config.h"
#include "DashStyle.h"
#include "Nux/Nux.h"

namespace {
  nux::logging::Logger logger("unity.dash.FilterRatingsButton");
}

namespace unity {

  FilterRatingsButton::FilterRatingsButton (NUX_FILE_LINE_DECL)
      : nux::Button (NUX_FILE_LINE_PARAM)
      , prelight_empty_ (NULL)
      , active_empty_ (NULL)
      , normal_empty_ (NULL)
      , prelight_half_ (NULL)
      , active_half_ (NULL)
      , normal_half_ (NULL)
      , prelight_full_ (NULL)
      , active_full_ (NULL)
      , normal_full_ (NULL)
  {
    InitTheme();

    mouse_down.connect (sigc::mem_fun (this, &FilterRatingsButton::RecvMouseDown) );
  }

  FilterRatingsButton::~FilterRatingsButton() {
    delete prelight_empty_;
    delete active_empty_;
    delete normal_empty_;
    delete prelight_half_;
    delete active_half_;
    delete normal_half_;
    delete prelight_full_;
    delete active_full_ ;
    delete normal_full_;
  }

  void FilterRatingsButton::SetFilter(dash::Filter::Ptr filter)
  {
    filter_ = std::static_pointer_cast<dash::RatingsFilter>(filter);
    filter_->rating.changed.connect (sigc::mem_fun (this, &FilterRatingsButton::OnRatingsChanged));
    NeedRedraw();
  }

  std::string FilterRatingsButton::GetFilterType ()
  {
    return "FilterRatingsButton";
  }

  void FilterRatingsButton::InitTheme()
  {
    if (prelight_empty_ == NULL)
    {
      nux::Geometry geometry = GetGeometry();
      geometry.width /= 5;
      prelight_empty_ = new nux::CairoWrapper(geometry, sigc::bind(sigc::mem_fun(this, &FilterRatingsButton::RedrawTheme), 0, nux::State::NUX_STATE_PRELIGHT));
      active_empty_ = new nux::CairoWrapper(geometry, sigc::bind(sigc::mem_fun(this, &FilterRatingsButton::RedrawTheme), 0, nux::State::NUX_STATE_ACTIVE));
      normal_empty_ = new nux::CairoWrapper(geometry, sigc::bind(sigc::mem_fun(this, &FilterRatingsButton::RedrawTheme), 0, nux::State::NUX_STATE_NORMAL));

      prelight_half_ = new nux::CairoWrapper(geometry, sigc::bind(sigc::mem_fun(this, &FilterRatingsButton::RedrawTheme), 1, nux::State::NUX_STATE_PRELIGHT));
      active_half_ = new nux::CairoWrapper(geometry, sigc::bind(sigc::mem_fun(this, &FilterRatingsButton::RedrawTheme), 1, nux::State::NUX_STATE_ACTIVE));
      normal_half_ = new nux::CairoWrapper(geometry, sigc::bind(sigc::mem_fun(this, &FilterRatingsButton::RedrawTheme), 1, nux::State::NUX_STATE_NORMAL));

      prelight_full_ = new nux::CairoWrapper(geometry, sigc::bind(sigc::mem_fun(this, &FilterRatingsButton::RedrawTheme), 2, nux::State::NUX_STATE_PRELIGHT));
      active_full_ = new nux::CairoWrapper(geometry, sigc::bind(sigc::mem_fun(this, &FilterRatingsButton::RedrawTheme), 2, nux::State::NUX_STATE_ACTIVE));
      normal_full_ = new nux::CairoWrapper(geometry, sigc::bind(sigc::mem_fun(this, &FilterRatingsButton::RedrawTheme), 2, nux::State::NUX_STATE_NORMAL));
    }
  }

  void FilterRatingsButton::RedrawTheme (nux::Geometry const& geom, cairo_t *cr, int type, nux::State faked_state)
  {
    DashStyle *dash_style = DashStyle::GetDefault();
    if (type == 0)
    {
      // empty
      dash_style->StarEmpty (cr, faked_state);
    }
    else if (type == 1)
    {
      // half
      dash_style->StarHalf (cr, faked_state);
    }
    else
    {
      // full
      dash_style->StarFull (cr, faked_state);
    }
  }

  long FilterRatingsButton::ComputeLayout2 ()
  {
    long ret = nux::Button::ComputeLayout2();
    if (cached_geometry_ != GetGeometry())
    {
      nux::Geometry geometry = GetGeometry();
      geometry.width /= 5;
      prelight_empty_->Invalidate(geometry);
      active_empty_->Invalidate(geometry);
      normal_empty_->Invalidate(geometry);

      prelight_half_->Invalidate(geometry);
      active_half_->Invalidate(geometry);
      normal_half_->Invalidate(geometry);

      prelight_full_->Invalidate(geometry);
      active_full_->Invalidate(geometry);
      normal_full_->Invalidate(geometry);
    }

    cached_geometry_ = GetGeometry();
    return ret;
  }


  long int FilterRatingsButton::ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo) {
    return nux::Button::ProcessEvent(ievent, TraverseInfo, ProcessEventInfo);
  }

  void FilterRatingsButton::Draw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    int rating = 5;
    if (filter_ != NULL)
      rating = filter_->rating * 10;
    int total_full_stars = rating / 2;
    int total_half_stars = rating % 2;

    nux::Geometry geometry = GetGeometry ();
    geometry.width = geometry.width / 5;

    gPainter.PaintBackground(GfxContext, GetGeometry());
    // set up our texture mode
    nux::TexCoordXForm texxform;
    texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
    texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

    // clear what is behind us
    nux::t_u32 alpha = 0, src = 0, dest = 0;

    GfxContext.GetRenderStates().GetBlend(alpha, src, dest);
    GfxContext.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    nux::Color col = nux::color::Black;
    col.alpha = 0;
    GfxContext.QRP_Color(GetGeometry().x,
                         GetGeometry().y,
                         GetGeometry().width,
                         GetGeometry().height,
                         col);

    for (int index = 0; index < 5; index++)
    {
      nux::BaseTexture *texture = normal_empty_->GetTexture();

      if (index < total_full_stars) {
        if (state = nux::State::NUX_STATE_NORMAL)
          texture = normal_full_->GetTexture();
        else if (state == nux::State::NUX_STATE_PRELIGHT)
          texture = prelight_full_->GetTexture();
        else if (state == nux::State::NUX_STATE_ACTIVE)
          texture = active_full_->GetTexture();
      }
      else if (index < total_full_stars + total_half_stars) {
        if (state = nux::State::NUX_STATE_NORMAL)
          texture = normal_half_->GetTexture();
        else if (state == nux::State::NUX_STATE_PRELIGHT)
          texture = prelight_half_->GetTexture();
        else if (state == nux::State::NUX_STATE_ACTIVE)
          texture = active_half_->GetTexture();
      }
      else {
        if (state = nux::State::NUX_STATE_NORMAL)
          texture = normal_empty_->GetTexture();
        else if (state == nux::State::NUX_STATE_PRELIGHT)
          texture = prelight_empty_->GetTexture();
        else if (state == nux::State::NUX_STATE_ACTIVE)
          texture = active_empty_->GetTexture();
      }

      GfxContext.QRP_1Tex(geometry.x,
                          geometry.y,
                          geometry.width,
                          geometry.height,
                          texture->GetDeviceTexture(),
                          texxform,
                          nux::Color(1.0f, 1.0f, 1.0f, 1.0f));

      geometry.x += geometry.width;

    }

    GfxContext.GetRenderStates().SetBlend(alpha, src, dest);

  }

  void FilterRatingsButton::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::Button::DrawContent(GfxContext, force_draw);
  }

  void FilterRatingsButton::PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::Button::PostDraw(GfxContext, force_draw);
  }

  void FilterRatingsButton::RecvMouseDown (int x, int y, unsigned long button_flags, unsigned long key_flags) {
    int width = GetGeometry().width;
    float new_rating = (static_cast<float>(x) / width) + 0.10f;
    if (filter_ != NULL)
      filter_->rating = new_rating;
  }

  void FilterRatingsButton::OnRatingsChanged (int rating) {
    NeedRedraw();
  }

};
