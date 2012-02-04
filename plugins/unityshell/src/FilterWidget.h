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
#ifndef UNITYSHELL_FILTERWIDGET_H
#define UNITYSHELL_FILTERWIDGET_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <UnityCore/Filter.h>

namespace unity
{
namespace dash
{

class FilterWidget : public nux::View
{
  NUX_DECLARE_OBJECT_TYPE(FilterWidget, nux::View);
public:
  FilterWidget(NUX_FILE_LINE_PROTO);
  virtual ~FilterWidget() {};

  virtual void SetFilter(Filter::Ptr const& filter) = 0;
  virtual std::string GetFilterType() = 0;
};

} // namespace dash
} // namespace unity

#endif //UNITYSHELL_FILTERWIDGET_H
