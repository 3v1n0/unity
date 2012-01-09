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

namespace unity
{
namespace dash
{
  
FilterMultiRangeButton::FilterMultiRangeButton(std::string const& label, NUX_FILE_LINE_DECL)
  : nux::ToggleButton(label, NUX_FILE_LINE_PARAM)
  , has_arrow_(MultiRangeArrow::NONE)
  , side_(MultiRangeSide::CENTER)
{
  InitTheme();
  state_change.connect(sigc::mem_fun(this, &FilterMultiRangeButton::OnActivated));
}

FilterMultiRangeButton::FilterMultiRangeButton(NUX_FILE_LINE_DECL)
  : nux::ToggleButton(NUX_FILE_LINE_PARAM)
  , has_arrow_(MultiRangeArrow::NONE)
  , side_(MultiRangeSide::CENTER)
{
  InitTheme();
  state_change.connect(sigc::mem_fun(this, &FilterMultiRangeButton::OnActivated));
}

FilterMultiRangeButton::~FilterMultiRangeButton()
{
}

void FilterMultiRangeButton::OnActivated(nux::Area* area)
{
  if (filter_)
    filter_->active = Active();
}

void FilterMultiRangeButton::OnActiveChanged(bool value)
{
  NeedRedraw();
}

void FilterMultiRangeButton::SetFilter(FilterOption::Ptr const& filter)
{
  filter_ = filter;
  SetActive(filter_->active);
}

FilterOption::Ptr FilterMultiRangeButton::GetFilter()
{
  return filter_;
}

void FilterMultiRangeButton::SetVisualSide(MultiRangeSide side)
{
  nux::Geometry const& geo = GetGeometry();

  side_ = side;
  prelight_->Invalidate(geo);
  active_->Invalidate(geo);
  normal_->Invalidate(geo);
}

void FilterMultiRangeButton::SetHasArrow(MultiRangeArrow value)
{
  has_arrow_ = value;
  active_->Invalidate(GetGeometry());
  NeedRedraw();
}

long FilterMultiRangeButton::ComputeContentSize()
{
  if (!active_)
  {
    InitTheme();
  }
  long ret = nux::ToggleButton::ComputeContentSize();
  nux::Geometry const& geo = GetGeometry();
  if (cached_geometry_ != geo)
  {
    prelight_->Invalidate(geo);
    active_->Invalidate(geo);
    normal_->Invalidate(geo);
    cached_geometry_ = geo;
  }

  return ret;
}

void FilterMultiRangeButton::InitTheme()
{
  if (!active_)
  {
    nux::Geometry const& geo = GetGeometry();
    active_.reset(new nux::CairoWrapper(geo, sigc::bind(sigc::mem_fun(this, &FilterMultiRangeButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_PRESSED)));
    normal_.reset(new nux::CairoWrapper(geo, sigc::bind(sigc::mem_fun(this, &FilterMultiRangeButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_NORMAL)));
    prelight_.reset(new nux::CairoWrapper(geo, sigc::bind(sigc::mem_fun(this, &FilterMultiRangeButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_PRELIGHT)));
  }

  SetMinimumHeight(32);
}

void FilterMultiRangeButton::RedrawTheme(nux::Geometry const& geom, cairo_t* cr, nux::ButtonVisualState faked_state)
{
  std::string name("10");
  std::stringstream final;

  if (filter_)
  {
    name = filter_->name;
    final << "<small>" << name << "</small>";
  }

  Arrow arrow;
  if (has_arrow_ == MultiRangeArrow::NONE)
    arrow = Arrow::NONE;
  else if (has_arrow_ == MultiRangeArrow::LEFT)
    arrow = Arrow::LEFT;
  else if (has_arrow_ == MultiRangeArrow::BOTH)
    arrow = Arrow::BOTH;
  else
    arrow = Arrow::RIGHT;

  Segment segment;
  if (side_ == MultiRangeSide::LEFT)
    segment = Segment::LEFT;
  else if (side_ == MultiRangeSide::CENTER)
    segment = Segment::MIDDLE;
  else
    segment = Segment::RIGHT;

  Style::Instance().MultiRangeSegment(cr, faked_state, final.str(), arrow, segment);
  NeedRedraw();
}

void FilterMultiRangeButton::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();
  
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

  nux::BaseTexture* texture = normal_->GetTexture();
  //FIXME - dashstyle does not give us a focused state yet, so ignore
  //~ if (state == nux::ButtonVisualState::NUX_VISUAL_STATE_PRELIGHT)
  //~ {
    //~ texture = prelight_->GetTexture();
  //~ }
  if (Active())
  {
    texture = active_->GetTexture();
  }

  GfxContext.QRP_1Tex(geo.x,
                      geo.y,
                      geo.width,
                      geo.height,
                      texture->GetDeviceTexture(),
                      texxform,
                      nux::Color(1.0f, 1.0f, 1.0f, 1.0f));
  GfxContext.GetRenderStates().SetBlend(alpha, src, dest);
}

} // namespace dash
} // namespace unity

