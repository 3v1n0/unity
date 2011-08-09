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



#ifndef FILTERMULTIRANGE_H
#define FILTERMULTIRANGE_H

#include <Nux/Nux.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include <UnityCore/MultiRangeFilter.h>
#include "FilterWidget.h"
#include "FilterBasicButton.h"
#include "FilterExpanderLabel.h"

namespace unity {
  class FilterMultiRangeButton;

  class FilterMultiRange : public FilterExpanderLabel, public unity::FilterWidget {
  public:
    FilterMultiRange (NUX_FILE_LINE_PROTO);
    virtual ~FilterMultiRange();

    void SetFilter (dash::Filter::Ptr filter);
    std::string GetFilterType ();

    nux::Property<bool> all_selected;

  protected:
    virtual long int ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo);
    virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw);

    void InitTheme ();

  private:
    void OnAllActivated(nux::View* view);
    void OnOptionAdded(dash::FilterOption::Ptr new_filter);
    void OnOptionRemoved(dash::FilterOption::Ptr removed_filter);

    nux::HLayout *layout_;
    FilterBasicButton* all_button_;

    std::vector<FilterMultiRangeButton*> buttons_;
    dash::MultiRangeFilter::Ptr filter_;
  };

}

#endif // FILTERMULTIRANGE_H
