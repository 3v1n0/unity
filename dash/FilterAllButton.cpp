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
 * Authored by: Andrea Azzarone <azzaronea@gmail.com>
 *
 */


#include <glib.h>
#include "config.h"
#include <glib/gi18n-lib.h>

#include "FilterAllButton.h"

namespace unity
{
namespace dash
{

NUX_IMPLEMENT_OBJECT_TYPE(FilterAllButton);

FilterAllButton::FilterAllButton(NUX_FILE_LINE_DECL)
 : FilterBasicButton(_("All"), NUX_FILE_LINE_PARAM)
{
  SetActive(true);
  SetInputEventSensitivity(false);

  state_change.connect(sigc::mem_fun(this, &FilterAllButton::OnStateChanged));
}

void FilterAllButton::SetFilter(Filter::Ptr const& filter)
{
  filter_ = filter;
  OnFilteringChanged(filter_->filtering); // Evil hack ;)

  filtering_connection_ = filter_->filtering.changed.connect(sigc::mem_fun(this, &FilterAllButton::OnFilteringChanged));
}

void FilterAllButton::OnStateChanged(nux::View* view)
{
  if (filter_ and Active())
    filter_->Clear();
  QueueDraw();
}

void FilterAllButton::OnFilteringChanged(bool filtering)
{
  SetActive(!filtering);
  SetInputEventSensitivity(filtering);
}

} // namespace dash
} // namespace unity
