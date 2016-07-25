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
#include <UnityCore/GLibWrapper.h>

#include "AnimationUtils.h"
#include "ApplicationManager.h"
#include "SearchBar.h"
#include "UnitySettings.h"
#include "WindowManager.h"
#include "RawPixel.h"

namespace unity
{
namespace spread
{
namespace
{
const unsigned FADE_DURATION = 100;
const unsigned DEFAULT_SEARCH_WAIT = 300;
const RawPixel OFFSET_X = 10_em;
const RawPixel OFFSET_Y = 15_em;
const RawPixel WIDTH = 620_em;

// For some reason std::to_lower or boost::to_lower_copy doesn't seem to handle well utf8
std::string casefold_copy(std::string const& str)
{
  return glib::String(g_utf8_casefold(str.c_str(), -1)).Str();
}
}

Filter::Filter()
  : fade_animator_(Settings::Instance().low_gfx() ? 0 : FADE_DURATION)
{
  auto& wm = WindowManager::Default();
  auto& settings = Settings::Instance();
  auto const& work_area = wm.GetWorkAreaGeometry(0);
  int monitor = wm.MonitorGeometryIn(work_area);
  int launcher_width = settings.LauncherSize(monitor);
  auto const& cv = settings.em(monitor);

  search_bar_ = SearchBar::Ptr(new SearchBar());
  search_bar_->SetMinimumWidth(WIDTH.CP(cv));
  search_bar_->SetMaximumWidth(WIDTH.CP(cv));
  search_bar_->scale = cv->DPIScale();
  search_bar_->live_search_wait = DEFAULT_SEARCH_WAIT;
  text.SetGetterFunction([this] { return search_bar_->search_string(); });
  text.SetSetterFunction([this] (std::string const& t) { search_bar_->search_string = t; return false; });
  debug::Introspectable::AddChild(search_bar_.GetPointer());

  auto layout = new nux::HLayout(NUX_TRACKER_LOCATION);
  layout->SetVerticalExternalMargin(0);
  layout->SetHorizontalExternalMargin(0);
  layout->AddView(search_bar_.GetPointer());

  view_window_ = new nux::BaseWindow(GetName().c_str());
  view_window_->SetLayout(layout);
  view_window_->SetBackgroundColor(nux::color::Transparent);
  view_window_->SetWindowSizeMatchLayout(true);
  view_window_->ShowWindow(true);
  view_window_->PushToFront();
  view_window_->SetOpacity(0.0f);
  view_window_->SetEnterFocusInputArea(search_bar_.GetPointer());
  view_window_->SetInputFocus();
  if (Settings::Instance().launcher_position() == LauncherPosition::LEFT)
    view_window_->SetXY(OFFSET_X.CP(cv) + std::max(work_area.x, launcher_width), OFFSET_Y.CP(cv) + work_area.y);
  else
    view_window_->SetXY(OFFSET_X.CP(cv) + work_area.x, OFFSET_Y.CP(cv) + work_area.y);
  fade_animator_.updated.connect([this] (double opacity) { view_window_->SetOpacity(opacity); });

  nux::GetWindowCompositor().SetKeyFocusArea(search_bar_->text_entry());

  search_bar_->search_changed.connect([this] (std::string const& search) {
    if (!Visible())
      animation::StartOrReverse(fade_animator_, animation::Direction::FORWARD);

    if (search.empty())
    {
      UpdateFilteredWindows();
      text.changed.emit(search);
      animation::StartOrReverse(fade_animator_, animation::Direction::BACKWARD);
    }
  });

  search_bar_->live_search_reached.connect([this] (std::string const& search) {
    if (!search.empty())
    {
      UpdateFilteredWindows();
      text.changed.emit(search);
      search_bar_->SetSearchFinished();
    }
  });

  Settings::Instance().low_gfx.changed.connect(sigc::track_obj([this] (bool low_gfx) {
    fade_animator_.SetDuration(low_gfx ? 0 : FADE_DURATION);
  }, *this));

  ApplicationManager::Default().window_opened.connect(sigc::hide(sigc::mem_fun(this, &Filter::OnWindowChanged)));
}

bool Filter::Visible() const
{
  return (view_window_->GetOpacity() != 0.0f);
}

nux::Geometry const& Filter::GetAbsoluteGeometry() const
{
  return view_window_->GetGeometry();
}

std::set<uint64_t> const& Filter::FilteredWindows() const
{
  return filtered_windows_;
}

void Filter::UpdateFilteredWindows()
{
  auto const& lower_search = casefold_copy(text());
  filtered_windows_.clear();
  title_connections_.Clear();

  if (lower_search.empty())
    return;

  auto update_cb = sigc::hide(sigc::mem_fun(this, &Filter::OnWindowChanged));

  for (auto const& app : ApplicationManager::Default().GetRunningApplications())
  {
    title_connections_.Add(app->title.changed.connect(update_cb));

    if (casefold_copy(app->title()).find(lower_search) != std::string::npos)
    {
      for (auto const& win : app->GetWindows())
        filtered_windows_.insert(win->window_id());
    }
  }

  for (auto const& win : ApplicationManager::Default().GetWindowsForMonitor(-1))
  {
    title_connections_.Add(win->title.changed.connect(update_cb));

    if (casefold_copy(win->title()).find(lower_search) != std::string::npos)
      filtered_windows_.insert(win->window_id());
  }
}

void Filter::OnWindowChanged()
{
  UpdateFilteredWindows();
  text.changed.emit(text);
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
    .add(GetAbsoluteGeometry())
    .add("visible", Visible());
}

} // namespace spread
} // namespace unity
