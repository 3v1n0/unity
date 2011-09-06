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

#include "Nux/Nux.h"
#include "Nux/VLayout.h"
#include "FilterBar.h"
#include "FilterFactory.h"

namespace unity
{

  FilterBar::FilterBar(NUX_FILE_LINE_DECL)
      : View (NUX_FILE_LINE_PARAM) {
    // TODO - does the filterbar associate itself with a model of some sort?
    // does libunity provide a Lens.Filters model or something that we can update on?
    // don't want to associate a Filterbar with just a lens model, its a filter bar not a
    // lens parser
    Init ();
  }

  FilterBar::~FilterBar() {

  }

  void FilterBar::Init () {
    nux::Layout *layout = new nux::VLayout (NUX_TRACKER_LOCATION);
    layout->SetVerticalInternalMargin(12);
    SetLayout (layout);
  }

  void FilterBar::SetFilters (dash::Filters::Ptr filters)
  {
    filters_ = filters;
  }

  void FilterBar::AddFilter (dash::Filter::Ptr filter) {
    if (filter_map_.count (filter) > 0) {
      g_warning ("Attempting to add a filter that has already been added");
      return;
    }

    nux::View *filter_view = factory_.WidgetForFilter (filter);
    filter_map_[filter] = filter_view;
    GetLayout()->AddView(filter_view, 0, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);
  }

  void FilterBar::RemoveFilter (dash::Filter::Ptr filter) {
    for (auto iter: filter_map_)
    {
      if (iter.first->id == filter->id)
      {
        nux::View *filter_view = iter.second;
        filter_map_.erase (filter_map_.find(iter.first));
        GetLayout()->RemoveChildObject (filter_view);
        filter_view->UnReference();
        break;
      }
    }
  }

  void FilterBar::ClearFilters () {
    std::map<dash::Filter::Ptr, nux::View *>::iterator it;
    for (it=filter_map_.begin() ; it != filter_map_.end(); it++)
      GetLayout()->RemoveChildObject ((*it).second);

    filter_map_.clear();
  }


  long int FilterBar::ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo) {
    return GetLayout()->ProcessEvent(ievent, TraverseInfo, ProcessEventInfo);
  }

  void FilterBar::Draw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::Geometry geo = GetGeometry();

    GfxContext.PushClippingRectangle(geo);
    nux::GetPainter().PaintBackground(GfxContext, geo);
    GfxContext.PopClippingRectangle();
  }

  void FilterBar::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw) {
    GfxContext.PushClippingRectangle(GetGeometry());

    GetLayout()->ProcessDraw(GfxContext, force_draw);

    GfxContext.PopClippingRectangle();
  }

  void FilterBar::PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::View::PostDraw(GfxContext, force_draw);
  }


}

