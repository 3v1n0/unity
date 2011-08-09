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
#include "FilterMultiRangeButton.h"

namespace unity {
  FilterMultiRangeButton::FilterMultiRangeButton (const std::string label, NUX_FILE_LINE_DECL)
      : nux::ToggleButton(label, NUX_FILE_LINE_PARAM) {
    active.changed.connect ([&] (bool is_active) {
      bool tmp_active = active;
      if (filter_ != NULL)
        filter_->active = tmp_active;
    });
  }

  FilterMultiRangeButton::FilterMultiRangeButton (NUX_FILE_LINE_DECL)
      : nux::ToggleButton(NUX_FILE_LINE_PARAM) {
    active.changed.connect ([&] (bool is_active) {
      bool tmp_active = active;
      if (filter_ != NULL)
        filter_->active = tmp_active;
    });
  }


  void FilterMultiRangeButton::SetFilter (dash::FilterOption::Ptr filter)
  {
    filter_ = filter;
    std::string tmp_label = filter->name;
    bool tmp_active = filter_->active;
    label = tmp_label;
    active = tmp_active;
  }

  dash::FilterOption::Ptr FilterMultiRangeButton::GetFilter()
  {
    return filter_;
  }

  long int FilterMultiRangeButton::ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo) {
    return nux::ToggleButton::ProcessEvent(ievent, TraverseInfo, ProcessEventInfo);
  }

  void FilterMultiRangeButton::Draw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    //FIXME - use the dash style to draw our own stuff
    nux::ToggleButton::Draw(GfxContext, force_draw);
  }

  void FilterMultiRangeButton::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::ToggleButton::DrawContent(GfxContext, force_draw);
  }

  void FilterMultiRangeButton::PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::ToggleButton::PostDraw(GfxContext, force_draw);
  }

}
