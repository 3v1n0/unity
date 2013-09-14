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

#include <Nux/Nux.h>
#include <Nux/VLayout.h>
#include <NuxCore/Logger.h>

#include "unity-shared/DashStyle.h"
#include "unity-shared/GraphicsUtils.h"
#include "FilterBar.h"
#include "FilterExpanderLabel.h"
#include "FilterFactory.h"

namespace unity
{
namespace dash
{
DECLARE_LOGGER(logger, "unity.dash.filterbar");

NUX_IMPLEMENT_OBJECT_TYPE(FilterBar);

FilterBar::FilterBar(NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
{
  Init();
}

FilterBar::~FilterBar()
{
}

void FilterBar::Init()
{
  dash::Style& style = dash::Style::Instance();

  nux::LinearLayout* layout = new nux::VLayout(NUX_TRACKER_LOCATION);
  layout->SetTopAndBottomPadding(style.GetFilterBarTopPadding() - style.GetFilterHighlightPadding());
  layout->SetSpaceBetweenChildren(style.GetSpaceBetweenFilterWidgets() - style.GetFilterHighlightPadding());
  SetLayout(layout);
}

void FilterBar::SetFilters(Filters::Ptr const& filters)
{
  filters_ = filters;
}

void FilterBar::AddFilter(Filter::Ptr const& filter)
{
  if (filter_map_.count(filter) > 0)
  {
    LOG_WARN(logger) << "Attempting to add a filter that has already been added";
    return;
  }

  FilterExpanderLabel* filter_view = factory_.WidgetForFilter(filter);
  AddChild(filter_view);
  filter_map_[filter] = filter_view;
  GetLayout()->AddView(filter_view, 0, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL);
}

void FilterBar::RemoveFilter(Filter::Ptr const& filter)
{
  for (auto iter: filter_map_)
  {
    if (iter.first->id == filter->id)
    {
      FilterExpanderLabel* filter_view = iter.second;
      RemoveChild(filter_view);
      filter_map_.erase(filter_map_.find(iter.first));
      GetLayout()->RemoveChildObject(filter_view);
      break;
    }
  }
}

void FilterBar::ClearFilters()
{
  for (auto iter: filter_map_)
  {
    FilterExpanderLabel* filter_view = iter.second;
    RemoveChild(filter_view);
    GetLayout()->RemoveChildObject(filter_view);
  }
  filter_map_.clear();
}

void FilterBar::Draw(nux::GraphicsEngine& graphics_engine, bool force_draw)
{

}

void FilterBar::DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  graphics_engine.PushClippingRectangle(GetGeometry());

  if (!IsFullRedraw() && RedirectedAncestor())
  {
    for (auto iter: filter_map_)
    {
      FilterExpanderLabel* filter_view = iter.second;
      if (filter_view && filter_view->IsVisible() && filter_view->IsRedrawNeeded())
        graphics::ClearGeometry(filter_view->GetGeometry());  
    }
  }

  GetLayout()->ProcessDraw(graphics_engine, force_draw);
  graphics_engine.PopClippingRectangle();
}

//
// Key navigation
//

bool FilterBar::AcceptKeyNavFocus()
{
  return false;
}

//
// Introspection
//
std::string FilterBar::GetName() const
{
  return "FilterBar";
}

void FilterBar::AddProperties(GVariantBuilder* builder)
{
  variant::BuilderWrapper(builder)
    .add("x", GetAbsoluteX())
    .add("y", GetAbsoluteY())
    .add("width", GetAbsoluteWidth())
    .add("height", GetAbsoluteHeight());
}

} // namespace dash
} // namespace unity
