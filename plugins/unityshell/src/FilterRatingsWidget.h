// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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



#ifndef FILTERRATINGSWIDGET_H
#define FILTERRATINGSWIDGET_H

#include <Nux/Nux.h>
#include <Nux/GridHLayout.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include <UnityCore/RatingsFilter.h>

#include "FilterWidget.h"
#include "FilterExpanderLabel.h"

namespace unity {
  class FilterBasicButton;
  class FilterRatingsButton;

  class FilterRatingsWidget : public FilterExpanderLabel, public FilterWidget
  {
    NUX_DECLARE_OBJECT_TYPE(FilterRatingsWidget, FilterExpanderLabel);
  public:
    FilterRatingsWidget (NUX_FILE_LINE_PROTO);
    virtual ~FilterRatingsWidget();

    void SetFilter (dash::Filter::Ptr filter);
    std::string GetFilterType ();

    nux::Property<int> rating;

  protected:
    virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw);

    void OnRatingsRatingChanged(const int& new_rating);
    void OnFilterRatingChanged(float new_rating);
    void OnAnyButtonActivated(nux::View *view);

    FilterBasicButton *any_button_;
    FilterRatingsButton *ratings_;
    dash::RatingsFilter::Ptr filter_;

  private:
    float last_rating_;
  };

}

#endif // FILTERRATINGSWIDGET_H
