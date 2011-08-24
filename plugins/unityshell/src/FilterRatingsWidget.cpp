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
#include <glib.h>
#include <glib/gi18n-lib.h>

#include "FilterGenreWidget.h"
#include "FilterGenreButton.h"
#include "FilterBasicButton.h"
#include "FilterRatingsButton.h"
#include "FilterRatingsWidget.h"


namespace unity {

  FilterRatingsWidget::FilterRatingsWidget (NUX_FILE_LINE_DECL)
      : FilterExpanderLabel (_("Rating"), NUX_FILE_LINE_PARAM)
  {
    any_button_ = new FilterBasicButton(_("Any"), NUX_TRACKER_LOCATION);
    any_button_->activated.connect(sigc::mem_fun(this, &FilterRatingsWidget::OnAnyButtonActivated));

    SetRightHandView(any_button_);

    nux::VLayout* layout = new nux::VLayout (NUX_TRACKER_LOCATION);
    ratings_ = new FilterRatingsButton (NUX_TRACKER_LOCATION);

    layout->AddView(ratings_);

    SetContents(layout);
  }

  FilterRatingsWidget::~FilterRatingsWidget() {
  }

  void FilterRatingsWidget::OnAnyButtonActivated(nux::View *view)
  {
    filter_->Clear();
  }

  void FilterRatingsWidget::SetFilter (dash::Filter::Ptr filter)
  {
    filter_ = std::static_pointer_cast<dash::RatingsFilter>(filter);
    ratings_->SetFilter(filter);
    SetLabel(filter_->name);
    NeedRedraw();
  }

  std::string FilterRatingsWidget::GetFilterType ()
  {
    return "FilterRatingsWidget";
  }


  long int FilterRatingsWidget::ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo) {
    return GetLayout()->ProcessEvent(ievent, TraverseInfo, ProcessEventInfo);
  }

  void FilterRatingsWidget::Draw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::Geometry geo = GetGeometry();

    GfxContext.PushClippingRectangle(geo);
    nux::GetPainter().PaintBackground(GfxContext, geo);
    GfxContext.PopClippingRectangle();
  }

  void FilterRatingsWidget::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw) {
    GfxContext.PushClippingRectangle(GetGeometry());

    GetLayout()->ProcessDraw(GfxContext, force_draw);

    GfxContext.PopClippingRectangle();
  }

  void FilterRatingsWidget::PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::View::PostDraw(GfxContext, force_draw);
  }

};
