// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2012 Canonical Ltd
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
* Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
*/

#include "XdndManagerImp.h"

#include "unity-shared/UScreen.h"

namespace unity {

XdndManagerImp::XdndManagerImp(XdndStartStopNotifier::Ptr const& xdnd_start_stop_notifier, 
                               XdndCollectionWindow::Ptr const& xdnd_collection_window)
  : xdnd_start_stop_notifier_(xdnd_start_stop_notifier)
  , xdnd_collection_window_(xdnd_collection_window)
  , last_monitor_(-1)
  , valid_dnd_in_progress_(false)
{
  xdnd_start_stop_notifier_->started.connect(sigc::mem_fun(this, &XdndManagerImp::OnDndStarted));
  xdnd_start_stop_notifier_->finished.connect(sigc::mem_fun(this, &XdndManagerImp::OnDndFinished));

  xdnd_collection_window_->collected.connect(sigc::mem_fun(this, &XdndManagerImp::OnDndDataCollected));
}

void XdndManagerImp::OnDndStarted()
{
  xdnd_collection_window_->Collect();
}

void XdndManagerImp::OnDndFinished()
{
  xdnd_collection_window_->Deactivate();
  mouse_poller_timeout_.reset();

  if (valid_dnd_in_progress_)
  {
    valid_dnd_in_progress_ = false;
    dnd_finished.emit();
  }
}

void XdndManagerImp::OnDndDataCollected(std::vector<std::string> const& mimes)
{
  if (!IsAValidDnd(mimes))
    return;

  valid_dnd_in_progress_ = true;

  auto& gp_display = nux::GetWindowThread()->GetGraphicsDisplay();
  char target[] = "text/uri-list";
  glib::String data(gp_display.GetDndData(target));

  auto uscreen = UScreen::GetDefault();
  last_monitor_ = uscreen->GetMonitorWithMouse();

  mouse_poller_timeout_.reset(new glib::Timeout(20, sigc::mem_fun(this, &XdndManagerImp::CheckMousePosition)));

  dnd_started.emit(data.Str(), last_monitor_);
}

bool XdndManagerImp::IsAValidDnd(std::vector<std::string> const& mimes)
{
  auto end = std::end(mimes);
  auto it = std::find(std::begin(mimes), end, "text/uri-list");

  return it != end;
}

bool XdndManagerImp::CheckMousePosition()
{
  auto uscreen = UScreen::GetDefault();
  auto monitor = uscreen->GetMonitorWithMouse();

  if (valid_dnd_in_progress_ && monitor != last_monitor_)
  {
    last_monitor_ = monitor;
    monitor_changed.emit(last_monitor_);
  }

  return true;
}

}

