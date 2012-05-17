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
#include "FilterBar.h"
#include "FilterExpanderLabel.h"
#include "FilterFactory.h"

namespace unity
{
namespace dash
{
namespace
{

nux::logging::Logger logger("unity.dash.filterbar");

}

NUX_IMPLEMENT_OBJECT_TYPE(FilterBar);

FilterBar::FilterBar(NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
{
  // TODO - does the filterbar associate itself with a model of some sort?
  // does libunity provide a Lens.Filters model or something that we can update on?
  // don't want to associate a Filterbar with just a lens model, its a filter bar not a
  // lens parser
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
  GetLayout()->AddView(filter_view, 0, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);

  UpdateDrawSeparators();
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

  UpdateDrawSeparators();
}

void FilterBar::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();

  GfxContext.PushClippingRectangle(geo);
  nux::GetPainter().PaintBackground(GfxContext, geo);
  GfxContext.PopClippingRectangle();
}

void FilterBar::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  GfxContext.PushClippingRectangle(GetGeometry());
  GetLayout()->ProcessDraw(GfxContext, force_draw);

  GfxContext.PopClippingRectangle();
}

void FilterBar::UpdateDrawSeparators()
{
  std::list<Area*> children = GetLayout()->GetChildren();
  std::list<Area*>::reverse_iterator rit;
  bool found_one = false;

  for (rit = children.rbegin(); rit != children.rend(); ++rit)
  {
    FilterExpanderLabel* widget = dynamic_cast<FilterExpanderLabel*>(*rit);

    if (!widget)
      continue;

    widget->draw_separator = found_one;
    found_one = true;
  }
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
