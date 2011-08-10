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

#include "FilterRatingsButton.h"
#include "config.h"
#include "DashStyle.h"
#include "Nux/Nux.h"




namespace unity {

  FilterRatingsButton::FilterRatingsButton (NUX_FILE_LINE_DECL)
      : nux::Button (NUX_FILE_LINE_PARAM)
  {
    InitTheme();

    OnMouseDown.connect (sigc::mem_fun (this, &FilterRatingsButton::RecvMouseDown) );
  }

  FilterRatingsButton::~FilterRatingsButton() {

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
    if (prelight_ == NULL)
    {
      prelight_ = new nux::CairoWrapper(GetGeometry(), sigc::bind(sigc::mem_fun(this, &FilterRatingsButton::RedrawTheme), nux::State::NUX_STATE_PRELIGHT));
      active_ = new nux::CairoWrapper(GetGeometry(), sigc::bind(sigc::mem_fun(this, &FilterRatingsButton::RedrawTheme), nux::State::NUX_STATE_ACTIVE));
      normal_ = new nux::CairoWrapper(GetGeometry(), sigc::bind(sigc::mem_fun(this, &FilterRatingsButton::RedrawTheme), nux::State::NUX_STATE_NORMAL));
    }
  }

  void FilterRatingsButton::RedrawTheme (nux::Geometry const& geom, cairo_t *cr, nux::State faked_state)
  {
    DashStyle *dash_style = DashStyle::GetDefault();
    dash_style->Button (cr, faked_state, label);
    cairo_surface_write_to_png (cairo_get_target (cr), "/tmp/button.png");
  }


  long int FilterRatingsButton::ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo) {
    return nux::Button::ProcessEvent(ievent, TraverseInfo, ProcessEventInfo);
  }

  void FilterRatingsButton::Draw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    //~ int rating = 0;
    //~ if (filter_ != NULL)
      //~ rating = filter_->rating * 10;
    //~ int total_full_stars = rating / 2;
    //~ int total_half_stars = rating % 2;

    nux::Geometry geometry = GetGeometry ();
    geometry.width = geometry.width / 5;
    for (int index = 0; index < 5; index++) {
      geometry.x = index * geometry.width;

      nux::AbstractPaintLayer *render_layer;
      //~ if (index < total_full_stars) {
        //~ render_layer = _full_normal;
      //~ }
      //~ else if (index < total_full_stars + total_half_stars) {
        //~ render_layer = _half_normal;
      //~ }
      //~ else {
        //~ render_layer = _empty_normal;
      //~ }

      render_layer->SetGeometry(geometry);
      render_layer->Renderlayer(GfxContext);

    }
  }

  void FilterRatingsButton::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::Button::DrawContent(GfxContext, force_draw);
  }

  void FilterRatingsButton::PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::Button::PostDraw(GfxContext, force_draw);
  }

  void FilterRatingsButton::RecvMouseDown (int x, int y, unsigned long button_flags, unsigned long key_flags) {
    int width = GetGeometry().width;
    if (filter_ != NULL)
      filter_->rating = ceil(x / (width / 10.0));
  }

  void FilterRatingsButton::OnRatingsChanged (int rating) {
    NeedRedraw();
  }

};
