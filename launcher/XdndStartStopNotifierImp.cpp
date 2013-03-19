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

#include "XdndStartStopNotifierImp.h"

#include <Nux/Nux.h>

#include "unity-shared/WindowManager.h"

namespace unity {

XdndStartStopNotifierImp::XdndStartStopNotifierImp()
  : display_(nux::GetGraphicsDisplay()->GetX11Display())
  , selection_(XInternAtom(display_, "XdndSelection", false))
  , dnd_in_progress_(false)
{
  WindowManager& wm = WindowManager::Default();
  wm.window_mapped.connect(sigc::hide(sigc::mem_fun(this, &XdndStartStopNotifierImp::DndTimeoutSetup)));
  wm.window_unmapped.connect(sigc::hide(sigc::mem_fun(this, &XdndStartStopNotifierImp::DndTimeoutSetup)));
 }

void XdndStartStopNotifierImp::DndTimeoutSetup()
{
  if (timeout_ && timeout_->IsRunning())
    return;

  auto cb_func = sigc::mem_fun(this, &XdndStartStopNotifierImp::OnTimeout);
  timeout_.reset(new glib::Timeout(200, cb_func));
}

bool XdndStartStopNotifierImp::OnTimeout()
{
  Window drag_owner = XGetSelectionOwner(display_, selection_);

  // evil hack because Qt does not release the selction owner on drag finished
  Window root_r, child_r;
  int root_x_r, root_y_r, win_x_r, win_y_r;
  unsigned int mask;
  XQueryPointer(display_, DefaultRootWindow(display_), &root_r, &child_r, &root_x_r, &root_y_r, &win_x_r, &win_y_r, &mask);

  if (drag_owner && (mask & (Button1Mask | Button2Mask | Button3Mask)))
  {
    if (!dnd_in_progress_)
    {
      started.emit();
      dnd_in_progress_ = true;
    }

    return true;
  }

  if (dnd_in_progress_)
  {
    finished.emit();
    dnd_in_progress_ = false;
  }

  return false;
}

}
