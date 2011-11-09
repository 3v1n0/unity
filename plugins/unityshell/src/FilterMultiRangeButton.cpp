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
#include "config.h"

#include <Nux/Nux.h>

#include "DashStyle.h"
#include "FilterMultiRangeButton.h"

namespace unity {
  FilterMultiRangeButton::FilterMultiRangeButton (const std::string label, NUX_FILE_LINE_DECL)
      : nux::ToggleButton(label, NUX_FILE_LINE_PARAM)
      , prelight_ (NULL)
      , active_ (NULL)
      , normal_ (NULL)
      , has_arrow_ (MULTI_RANGE_ARROW_NONE)
      , side_ (MULTI_RANGE_CENTER)
  {
    InitTheme();
    state_change.connect (sigc::mem_fun(this, &FilterMultiRangeButton::OnActivated));
  }

  FilterMultiRangeButton::FilterMultiRangeButton (NUX_FILE_LINE_DECL)
      : nux::ToggleButton(NUX_FILE_LINE_PARAM)
      , prelight_ (NULL)
      , active_ (NULL)
      , normal_ (NULL)
      , has_arrow_ (MULTI_RANGE_ARROW_NONE)
      , side_ (MULTI_RANGE_CENTER)
  {
    InitTheme();
    state_change.connect (sigc::mem_fun(this, &FilterMultiRangeButton::OnActivated));
  }

  FilterMultiRangeButton::~FilterMultiRangeButton()
  {
    delete prelight_;
    delete normal_;
    delete active_;
  }

  void FilterMultiRangeButton::OnActivated (nux::Area *area)
  {
    bool tmp_active = Active();
    if (filter_ != NULL)
      filter_->active = tmp_active;
  }

  void FilterMultiRangeButton::OnActiveChanged(bool value)
  {
    NeedRedraw();
  }

  void FilterMultiRangeButton::SetFilter (dash::FilterOption::Ptr filter)
  {
    filter_ = filter;
    bool tmp_active = filter_->active;
    SetActive(tmp_active);
  }

  dash::FilterOption::Ptr FilterMultiRangeButton::GetFilter()
  {
    return filter_;
  }

  void FilterMultiRangeButton::SetVisualSide (MultiRangeSide side)
  {
    side_ = side;
    prelight_->Invalidate(GetGeometry());
    active_->Invalidate(GetGeometry());
    normal_->Invalidate(GetGeometry());
  }

  void FilterMultiRangeButton::SetHasArrow (MultiRangeArrow value)
  {
    has_arrow_ = value;
    active_->Invalidate(GetGeometry());
    NeedRedraw();
  }

  long FilterMultiRangeButton::ComputeContentSize()
  {
    if (prelight_ == NULL)
    {
      InitTheme();
    }
    long ret = nux::ToggleButton::ComputeContentSize();
    if (cached_geometry_ != GetGeometry())
    {
      prelight_->Invalidate(GetGeometry());
      active_->Invalidate(GetGeometry());
      normal_->Invalidate(GetGeometry());
    }

    cached_geometry_ = GetGeometry();
    return ret;
  }

  void FilterMultiRangeButton::InitTheme()
  {
    if (prelight_ == NULL)
    {
      prelight_ = new nux::CairoWrapper(GetGeometry(), sigc::bind(sigc::mem_fun(this, &FilterMultiRangeButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_PRELIGHT));
      active_ = new nux::CairoWrapper(GetGeometry(), sigc::bind(sigc::mem_fun(this, &FilterMultiRangeButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_PRESSED));
      normal_ = new nux::CairoWrapper(GetGeometry(), sigc::bind(sigc::mem_fun(this, &FilterMultiRangeButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_NORMAL));
    }

    SetMinimumHeight(32);
  }

  void FilterMultiRangeButton::RedrawTheme (nux::Geometry const& geom, cairo_t *cr, nux::ButtonVisualState faked_state)
  {
    std::string name = "10";
    std::stringstream final;

    if (filter_)
    {
      name = filter_->name;
      final << "<small>" << name << "</small>";
    }

    dash::Arrow arrow;
    if (has_arrow_ == MULTI_RANGE_ARROW_NONE)
      arrow = dash::Arrow::NONE;
    else if (has_arrow_ == MULTI_RANGE_ARROW_LEFT)
      arrow = dash::Arrow::LEFT;
    else if (has_arrow_ == MULTI_RANGE_ARROW_BOTH)
      arrow = dash::Arrow::BOTH;
    else
      arrow = dash::Arrow::RIGHT;

    dash::Segment segment;
    if (side_ == MULTI_RANGE_SIDE_LEFT)
      segment = dash::Segment::LEFT;
    else if (side_ == MULTI_RANGE_CENTER)
      segment = dash::Segment::MIDDLE;
    else
      segment = dash::Segment::RIGHT;

    dash::Style::Instance().MultiRangeSegment(cr, faked_state, final.str(), arrow, segment);
    NeedRedraw();
  }

  void FilterMultiRangeButton::Draw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    gPainter.PaintBackground(GfxContext, GetGeometry());
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
    GfxContext.QRP_Color(GetGeometry().x,
                         GetGeometry().y,
                         GetGeometry().width,
                         GetGeometry().height,
                         col);

    nux::BaseTexture *texture = normal_->GetTexture();
    //FIXME - dashstyle does not give us a focused state yet, so ignore
    //~ if (state == nux::ButtonVisualState::NUX_VISUAL_STATE_PRELIGHT)
    //~ {
      //~ texture = prelight_->GetTexture();
    //~ }
    if (Active())
    {
      texture = active_->GetTexture();
    }

    GfxContext.QRP_1Tex(GetGeometry().x,
                        GetGeometry().y,
                        GetGeometry().width,
                        GetGeometry().height,
                        texture->GetDeviceTexture(),
                        texxform,
                        nux::Color(1.0f, 1.0f, 1.0f, 1.0f));
    GfxContext.GetRenderStates().SetBlend(alpha, src, dest);
  }

  void FilterMultiRangeButton::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw) {
    //nux::ToggleButton::DrawContent(GfxContext, force_draw);
  }

  void FilterMultiRangeButton::PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::ToggleButton::PostDraw(GfxContext, force_draw);
  }

}
