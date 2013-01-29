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

#ifndef UNITYSHELL_FILTERRATINGSWIDGET_H
#define UNITYSHELL_FILTERRATINGSWIDGET_H

#include <Nux/Nux.h>
#include <Nux/GridHLayout.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include <UnityCore/RatingsFilter.h>

#include "FilterAllButton.h"
#include "FilterExpanderLabel.h"

namespace unity
{
namespace dash
{

class FilterBasicButton;
class FilterRatingsButton;

class FilterRatingsWidget : public FilterExpanderLabel
{
  NUX_DECLARE_OBJECT_TYPE(FilterRatingsWidget, FilterExpanderLabel);
public:
  FilterRatingsWidget(NUX_FILE_LINE_PROTO);
  virtual ~FilterRatingsWidget();

  void SetFilter(Filter::Ptr const& filter);
  std::string GetFilterType();

protected:
  void ClearRedirectedRenderChildArea();

private:
  FilterAllButton* all_button_;
  FilterRatingsButton* ratings_;
  RatingsFilter::Ptr filter_;
};

} // namespace dash
} // namespace unity

#endif // UNITYSHELL_FILTERRATINGSWIDGET_H
