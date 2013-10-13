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
namespace
{
  const std::string URI_TYPE = "text/uri-list";
}

XdndManagerImp::XdndManagerImp(XdndStartStopNotifier::Ptr const& xdnd_start_stop_notifier,
                               XdndCollectionWindow::Ptr const& xdnd_collection_window)
  : xdnd_start_stop_notifier_(xdnd_start_stop_notifier)
  , xdnd_collection_window_(xdnd_collection_window)
  , last_monitor_(-1)
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

  if (!dnd_data_.empty())
  {
    dnd_data_.clear();
    dnd_finished.emit();
  }
}

void XdndManagerImp::OnDndDataCollected(std::vector<std::string> const& mimes)
{
  if (!IsAValidDnd(mimes))
    return;

  auto& gp_display = nux::GetWindowThread()->GetGraphicsDisplay();
  glib::String data(gp_display.GetDndData(const_cast<char*>(URI_TYPE.c_str())));
  dnd_data_ = data.Str();

  if (dnd_data_.empty())
    return;

  auto uscreen = UScreen::GetDefault();
  last_monitor_ = uscreen->GetMonitorWithMouse();

  mouse_poller_timeout_.reset(new glib::Timeout(20, sigc::mem_fun(this, &XdndManagerImp::CheckMousePosition)));

  dnd_started.emit(dnd_data_, last_monitor_);
}

bool XdndManagerImp::IsAValidDnd(std::vector<std::string> const& mimes)
{
  auto end = std::end(mimes);
  auto it = std::find(std::begin(mimes), end, URI_TYPE);

  return it != end;
}

bool XdndManagerImp::CheckMousePosition()
{
  auto uscreen = UScreen::GetDefault();
  auto monitor = uscreen->GetMonitorWithMouse();

  if (!dnd_data_.empty() && monitor != last_monitor_)
  {
    last_monitor_ = monitor;
    monitor_changed.emit(dnd_data_, last_monitor_);
  }

  return true;
}

}

