// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2014 Canonical Ltd
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 3 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* Authored by: Marco Trevisan <marco@ubuntu.com>
*/

#include "SpreadFilter.h"

#include <Nux/HLayout.h>
#include "AnimationUtils.h"
#include "SearchBar.h"
#include "WindowManager.h"

namespace unity
{
namespace spread
{
namespace
{
const unsigned FADE_DURATION = 100;
const unsigned DEFAULT_SEARCH_WAIT = 300;
const nux::Point OFFSET(10, 15);
const nux::Size SIZE(620, 42);
}

Filter::Filter()
  : fade_animator_(FADE_DURATION)
{
  search_bar_ = SearchBar::Ptr(new SearchBar());
  search_bar_->SetMinMaxSize(SIZE.width, SIZE.height);
  search_bar_->live_search_wait = DEFAULT_SEARCH_WAIT;
  text.SetGetterFunction([this] { return search_bar_->search_string(); });
  text.SetSetterFunction([this] (std::string const& t) { search_bar_->search_string = t; return false; });
  debug::Introspectable::AddChild(search_bar_.GetPointer());

  auto layout = new nux::HLayout(NUX_TRACKER_LOCATION);
  layout->SetVerticalExternalMargin(0);
  layout->SetHorizontalExternalMargin(0);
  layout->AddView(search_bar_.GetPointer());

  auto const& work_area = WindowManager::Default().GetWorkAreaGeometry(0);
  view_window_ = new nux::BaseWindow(GetName().c_str());
  view_window_->SetLayout(layout);
  view_window_->SetBackgroundColor(nux::color::Transparent);
  view_window_->SetWindowSizeMatchLayout(true);
  view_window_->ShowWindow(true);
  view_window_->PushToFront();
  view_window_->SetOpacity(0.0f);
  view_window_->SetEnterFocusInputArea(search_bar_.GetPointer());
  view_window_->SetInputFocus();
  view_window_->SetXY(OFFSET.x + work_area.x, OFFSET.y + work_area.y);
  fade_animator_.updated.connect([this] (double opacity) { view_window_->SetOpacity(opacity); });

  nux::GetWindowCompositor().SetKeyFocusArea(search_bar_->text_entry());

  search_bar_->search_changed.connect([this] (std::string const& search) {
    if (!Visible())
      animation::StartOrReverse(fade_animator_, animation::Direction::FORWARD);

    if (search.empty())
    {
      text.changed.emit(search);
      animation::StartOrReverse(fade_animator_, animation::Direction::BACKWARD);
    }
  });

  search_bar_->live_search_reached.connect([this] (std::string const& search) {
    if (search.empty())
      return;

    text.changed.emit(search);
    search_bar_->SetSearchFinished();
  });
}

Filter::~Filter()
{
  nux::GetWindowCompositor().SetKeyFocusArea(nullptr);
  nux::GetWindowThread()->RemoveObjectFromLayoutQueue(view_window_.GetPointer());
}

bool Filter::Visible() const
{
  return (view_window_->GetOpacity() != 0.0f);
}

nux::Geometry const& Filter::GetAbsoluteGeometry() const
{
  return view_window_->GetGeometry();
}

//
// Introspection
//
std::string Filter::GetName() const
{
  return "SpreadFilter";
}

void Filter::AddProperties(debug::IntrospectionData& introspection)
{
  introspection
    .add("visible", Visible());
}

} // namespace spread
} // namespace unity
