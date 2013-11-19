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

#include "FilterGenreButton.h"

namespace unity
{
namespace dash
{

NUX_IMPLEMENT_OBJECT_TYPE(FilterGenreButton);

FilterGenreButton::FilterGenreButton(std::string const& label, NUX_FILE_LINE_DECL)
  : FilterBasicButton(label, NUX_FILE_LINE_PARAM)
{
  InitTheme();

  state_change.connect([this](nux::Button* button)
  {
    if (filter_)
      filter_->active = Active();
  });
}

FilterGenreButton::FilterGenreButton(NUX_FILE_LINE_DECL)
  : FilterBasicButton(NUX_FILE_LINE_PARAM)
{
  InitTheme();

  state_change.connect([this](nux::Button* button)
  {
    if (filter_)
      filter_->active = Active();
  });
}


void FilterGenreButton::SetFilter(FilterOption::Ptr const& filter)
{
  filter_ = filter;

  SetActive(filter_->active);

  filter_->active.changed.connect([this](bool is_active)
  {
    SetActive(is_active);
  });
}

FilterOption::Ptr FilterGenreButton::GetFilter()
{
  return filter_;
}

} // namespace dash
} // namespace unity
