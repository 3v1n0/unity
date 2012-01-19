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

#include "FilterBar.h"
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
  nux::LinearLayout* layout = new nux::VLayout(NUX_TRACKER_LOCATION);
  layout->SetSpaceBetweenChildren(12);
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

  nux::View* filter_view = factory_.WidgetForFilter(filter);
  filter_map_[filter] = filter_view;
  GetLayout()->AddView(filter_view, 0, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);
}

void FilterBar::RemoveFilter(Filter::Ptr const& filter)
{
  for (auto iter: filter_map_)
  {
    if (iter.first->id == filter->id)
    {
      nux::View* filter_view = iter.second;
      filter_map_.erase(filter_map_.find(iter.first));
      GetLayout()->RemoveChildObject(filter_view);
      break;
    }
  }
}

void FilterBar::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();

  GfxContext.PushClippingRectangle(geo);
  nux::GetPainter().PaintBackground(GfxContext, geo);

  nux::Color col(1.0f, 1.0f, 1.0f, 0.15f);
  int i = 0;
  int num_filters = filter_map_.size() - 1;

  for (auto iter: filter_map_)
  {
    if (i != num_filters)
    {
      nux::View* filter_view = iter.second;
      nux::Geometry const& geom = filter_view->GetGeometry();
      GfxContext.GetRenderStates().SetBlend(true);
      nux::GetPainter().Draw2DLine(GfxContext,
                                   geom.x, geom.y + geom.height - 1,
                                   geom.x + geom.width, geom.y + geom.height - 1,
                                   col,
                                   col);
      GfxContext.GetRenderStates().SetBlend(false);
    }
    i++;
  }

  GfxContext.PopClippingRectangle();
}

void FilterBar::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  GfxContext.PushClippingRectangle(GetGeometry());
  GetLayout()->ProcessDraw(GfxContext, force_draw);
  GfxContext.PopClippingRectangle();
}


} // namespace dash
} // namespace unity

