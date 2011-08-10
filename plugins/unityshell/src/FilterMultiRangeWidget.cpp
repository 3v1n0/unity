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
#include "Nux/Nux.h"

#include "FilterMultiRangeWidget.h"
#include "FilterMultiRangeButton.h"
#include "FilterBasicButton.h"

namespace unity {

  FilterMultiRange::FilterMultiRange (NUX_FILE_LINE_DECL)
      : FilterExpanderLabel ("Multi-range", NUX_FILE_LINE_PARAM)
      , all_selected (false) {
    InitTheme();

    all_button_ = new FilterBasicButton("Any", NUX_TRACKER_LOCATION);
    all_button_->Activated.connect(sigc::mem_fun(this, &FilterMultiRange::OnAllActivated));
    all_button_->Reference();

    layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
    layout_->Reference();

    SetRightHandView(all_button_);
    SetContents(layout_);

    // for testing only
    //~ FilterMultiRangeButton* button = new FilterMultiRangeButton ("10", NUX_TRACKER_LOCATION);
    //~ layout_->AddView (button, 1);
     //~ buttons_.push_back (button);
//~
    //~ button = new FilterMultiRangeButton ("10", NUX_TRACKER_LOCATION);
    //~ buttons_.push_back (button);
    //~ layout_->AddView (button, 1);
//~
    //~ button = new FilterMultiRangeButton ("10", NUX_TRACKER_LOCATION);
    //~ layout_->AddView (button, 1);buttons_.push_back (button);
//~
    //~ button = new FilterMultiRangeButton ("10", NUX_TRACKER_LOCATION);
    //~ layout_->AddView (button, 1);buttons_.push_back (button);
//~
    //~ button = new FilterMultiRangeButton ("10", NUX_TRACKER_LOCATION);
    //~ layout_->AddView (button, 1);buttons_.push_back (button);
//~
      //~ button = new FilterMultiRangeButton ("10", NUX_TRACKER_LOCATION);
    //~ layout_->AddView (button, 1);buttons_.push_back (button);
//~
      //~ button = new FilterMultiRangeButton ("10", NUX_TRACKER_LOCATION);
    //~ layout_->AddView (button, 1);buttons_.push_back (button);
//~
      //~ button = new FilterMultiRangeButton ("10", NUX_TRACKER_LOCATION);
    //~ layout_->AddView (button, 1);buttons_.push_back (button);
//~
        //~ button = new FilterMultiRangeButton ("10", NUX_TRACKER_LOCATION);
    //~ layout_->AddView (button, 1);buttons_.push_back (button);
//~
        //~ button = new FilterMultiRangeButton ("10", NUX_TRACKER_LOCATION);
    //~ layout_->AddView (button, 1);buttons_.push_back (button);
//~
        //~ button = new FilterMultiRangeButton ("10", NUX_TRACKER_LOCATION);
    //~ layout_->AddView (button, 1);buttons_.push_back (button);
//~
    //~ OnActiveChanged(false);
  }

  FilterMultiRange::~FilterMultiRange() {
    all_button_->UnReference();
    layout_->UnReference();
  }

  void FilterMultiRange::SetFilter(dash::Filter::Ptr filter)
  {
    filter_ = std::static_pointer_cast<dash::MultiRangeFilter>(filter);

    filter_->option_added.connect (sigc::mem_fun (this, &FilterMultiRange::OnOptionAdded));
    filter_->option_removed.connect(sigc::mem_fun (this, &FilterMultiRange::OnOptionRemoved));

    // finally - make sure we are up-todate with our filter list
    dash::MultiRangeFilter::Options::iterator it;
    dash::MultiRangeFilter::Options options = filter_->options;
    for (it = options.begin(); it < options.end(); it++)
      OnOptionAdded(*it);
  }

  void FilterMultiRange::OnActiveChanged(bool value)
  {
    // go through all the buttons, and set the state :(

    std::vector<FilterMultiRangeButton*>::iterator it;
    int start = 2000;
    int end = 0;
    int index = 0;
    for ( it=buttons_.begin() ; it < buttons_.end(); it++ )
    {
      FilterMultiRangeButton* button = (*it);
      dash::FilterOption::Ptr filter = button->GetFilter();
      if (filter != NULL)
      {
        if (filter->active)
          if (index < start)
            start = index;
          if (index > end)
            end = index;
      }
      index++;
    }

    index = 0;
    for ( it=buttons_.begin() ; it < buttons_.end(); it++ )
    {
      FilterMultiRangeButton* button = (*it);

      if (index == start && index == end)
        button->SetHasArrow(1);
      else if (index == start)
        button->SetHasArrow(0);
      else if (index == end)
        button->SetHasArrow(2);
      else
        button->SetHasArrow(-1);

      if (index == 0)
        button->SetVisualSide(0);
      else if (index == (int)buttons_.size() - 1)
        button->SetVisualSide(2);
      else
        button->SetVisualSide(1);

      index++;
    }
  }

  void FilterMultiRange::OnOptionAdded(dash::FilterOption::Ptr new_filter)
  {
    FilterMultiRangeButton* button = new FilterMultiRangeButton (NUX_TRACKER_LOCATION);
    button->SetFilter (new_filter);
    layout_->AddView (button, 1);
    buttons_.push_back (button);
    new_filter->active.changed.connect(sigc::mem_fun (this, &FilterMultiRange::OnActiveChanged));
    OnActiveChanged(false);
  }

  void FilterMultiRange::OnOptionRemoved(dash::FilterOption::Ptr removed_filter)
  {
    std::vector<FilterMultiRangeButton*>::iterator it;
    FilterMultiRangeButton* found_filter = NULL;
    for ( it=buttons_.begin() ; it < buttons_.end(); it++ )
    {
      if ((*it)->GetFilter() == removed_filter)
      {
        found_filter = *it;
        break;
      }
    }

    if (found_filter)
    {
      layout_->RemoveChildObject(*it);
      buttons_.erase (it);
    }

    OnActiveChanged(false);
  }

  std::string FilterMultiRange::GetFilterType ()
  {
    return "FilterMultiRange";
  }

  void FilterMultiRange::InitTheme()
  {
    //FIXME - build theme here - store images, cache them, fun fun fun
  }

  void FilterMultiRange::OnAllActivated(nux::View *view)
  {
    if (filter_)
      filter_->Clear();
  }

  long int FilterMultiRange::ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo) {
    return GetLayout()->ProcessEvent(ievent, TraverseInfo, ProcessEventInfo);
  }

  void FilterMultiRange::Draw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::Geometry geo = GetGeometry();

    GfxContext.PushClippingRectangle(geo);
    nux::GetPainter().PaintBackground(GfxContext, geo);
    GfxContext.PopClippingRectangle();
  }

  void FilterMultiRange::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw) {
    GfxContext.PushClippingRectangle(GetGeometry());

    GetLayout()->ProcessDraw(GfxContext, force_draw);

    GfxContext.PopClippingRectangle();
  }

  void FilterMultiRange::PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::View::PostDraw(GfxContext, force_draw);
  }

};
