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
#include <Nux/Layout.h>

#include "unity-shared/DashStyle.h"
#include "FilterMultiRangeButton.h"

namespace unity
{
namespace dash
{

namespace
{
const int kFontSizePx = 10;

const int kLayoutPadLeftRight = 4;
const int kLayoutPadtopBottom = 2;
}

NUX_IMPLEMENT_OBJECT_TYPE(FilterMultiRangeButton);

FilterMultiRangeButton::FilterMultiRangeButton(NUX_FILE_LINE_DECL)
  : nux::ToggleButton(NUX_FILE_LINE_PARAM)
  , theme_init_(false)
  , has_arrow_(MultiRangeArrow::NONE)
  , side_(MultiRangeSide::CENTER)
{
  Init();
}

FilterMultiRangeButton::~FilterMultiRangeButton()
{
}

void FilterMultiRangeButton::Init()
{
  InitTheme();
  // Controlled by parent widget
  SetAcceptKeyNavFocusOnMouseDown(false);
  SetAcceptKeyNavFocusOnMouseEnter(false);

  state_change.connect(sigc::mem_fun(this, &FilterMultiRangeButton::OnActivated));
  key_nav_focus_change.connect([this](nux::Area*, bool, nux::KeyNavDirection) { QueueDraw(); });
  key_nav_focus_activate.connect([this](nux::Area* area) { Active() ? Deactivate() : Activate(); });
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
  if (side_ == side)
    return;
  side_ = side;

  QueueDraw();
}

void FilterMultiRangeButton::SetHasArrow(MultiRangeArrow value)
{
  if (has_arrow_ == value)
    return;
  has_arrow_ = value;

  QueueDraw();
}

long FilterMultiRangeButton::ComputeContentSize()
{
  long ret = nux::ToggleButton::ComputeContentSize();
  nux::Geometry const& geo = GetGeometry();
  if (theme_init_ && cached_geometry_ != geo)
  {
    cached_geometry_ = geo;

    std::vector<MultiRangeSide> sides = {MultiRangeSide::LEFT, MultiRangeSide::RIGHT, MultiRangeSide::CENTER};
    std::vector<MultiRangeArrow> arrows = {MultiRangeArrow::LEFT, MultiRangeArrow::RIGHT, MultiRangeArrow::BOTH, MultiRangeArrow::NONE};

    auto func_invalidate = [geo](std::pair<const MapKey, NuxCairoPtr>& pair)
    {
      pair.second->Invalidate(geo);
    };

    for_each (prelight_.begin(), prelight_.end(), func_invalidate);
    for_each (active_.begin(), active_.end(), func_invalidate);
    for_each (normal_.begin(), normal_.end(), func_invalidate);
    for_each (focus_.begin(), focus_.end(), func_invalidate);
  }

  return ret;
}

void FilterMultiRangeButton::InitTheme()
{
  if (!active_[MapKey(MultiRangeArrow::LEFT, MultiRangeSide::LEFT)])
  {
    nux::Geometry const& geo = GetGeometry();

    std::vector<MultiRangeSide> sides = {MultiRangeSide::LEFT, MultiRangeSide::RIGHT, MultiRangeSide::CENTER};
    std::vector<MultiRangeArrow> arrows = {MultiRangeArrow::LEFT, MultiRangeArrow::RIGHT, MultiRangeArrow::BOTH, MultiRangeArrow::NONE};

    for (auto arrow : arrows)
    {
      for (auto side : sides)
      {
        active_[MapKey(arrow, side)].reset(new nux::CairoWrapper(geo, sigc::bind(sigc::mem_fun(this, &FilterMultiRangeButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_PRESSED, arrow, side)));
        normal_[MapKey(arrow, side)].reset(new nux::CairoWrapper(geo, sigc::bind(sigc::mem_fun(this, &FilterMultiRangeButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_NORMAL, arrow, side)));
        prelight_[MapKey(arrow, side)].reset(new nux::CairoWrapper(geo, sigc::bind(sigc::mem_fun(this, &FilterMultiRangeButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_PRELIGHT, arrow, side)));
        focus_[MapKey(arrow, side)].reset(new nux::CairoWrapper(geo, sigc::bind(sigc::mem_fun(this, &FilterMultiRangeButton::RedrawFocusOverlay), arrow, side)));
      }
    }
  }

  SetMinimumHeight(dash::Style::Instance().GetFilterButtonHeight() + 3);
  theme_init_ = true;
}

void FilterMultiRangeButton::RedrawTheme(nux::Geometry const& geom,
                                         cairo_t* cr,
                                         nux::ButtonVisualState faked_state,
                                         MultiRangeArrow faked_arrow,
                                         MultiRangeSide faked_side)
{
  std::string name("10");

  if (filter_)
  {
    name = filter_->name;
  }

  Arrow arrow;
  if (faked_arrow == MultiRangeArrow::NONE)
    arrow = Arrow::NONE;
  else if (faked_arrow == MultiRangeArrow::LEFT)
    arrow = Arrow::LEFT;
  else if (faked_arrow == MultiRangeArrow::BOTH)
    arrow = Arrow::BOTH;
  else
    arrow = Arrow::RIGHT;

  Segment segment;
  if (faked_side == MultiRangeSide::LEFT)
    segment = Segment::LEFT;
  else if (faked_side == MultiRangeSide::CENTER)
    segment = Segment::MIDDLE;
  else
    segment = Segment::RIGHT;

  Style::Instance().MultiRangeSegment(cr, faked_state, name, kFontSizePx, arrow, segment);
  NeedRedraw();
}

void FilterMultiRangeButton::RedrawFocusOverlay(nux::Geometry const& geom,
                                                cairo_t* cr,
                                                MultiRangeArrow faked_arrow,
                                                MultiRangeSide faked_side)
{
  Arrow arrow;
  if (faked_arrow == MultiRangeArrow::NONE)
    arrow = Arrow::NONE;
  else if (faked_arrow == MultiRangeArrow::LEFT)
    arrow = Arrow::LEFT;
  else if (faked_arrow == MultiRangeArrow::BOTH)
    arrow = Arrow::BOTH;
  else
    arrow = Arrow::RIGHT;

  Segment segment;
  if (faked_side == MultiRangeSide::LEFT)
    segment = Segment::LEFT;
  else if (faked_side == MultiRangeSide::CENTER)
    segment = Segment::MIDDLE;
  else
    segment = Segment::RIGHT;

  Style::Instance().MultiRangeFocusOverlay(cr, arrow, segment);
  QueueDraw();
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

  nux::BaseTexture* texture = normal_[MapKey(has_arrow_, side_)]->GetTexture();
  if (GetVisualState() == nux::ButtonVisualState::VISUAL_STATE_PRELIGHT)
  {
    texture = prelight_[MapKey(has_arrow_, side_)]->GetTexture();
  }

  if (Active())
  {
    texture = active_[MapKey(has_arrow_, side_)]->GetTexture();
  }

  if (HasKeyFocus())
  {
    GfxContext.QRP_1Tex(geo.x,
                        geo.y,
                        geo.width,
                        geo.height,
                        focus_[MapKey(has_arrow_, side_)]->GetTexture()->GetDeviceTexture(),
                        texxform,
                        nux::color::White);
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
