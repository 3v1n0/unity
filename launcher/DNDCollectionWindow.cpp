// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2011 Canonical Ltd
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
* Authored by: Andrea Azzarone <azzaronea@gmail.com>
*/

#include "DNDCollectionWindow.h"

#include "unity-shared/WindowManager.h"

namespace unity {

NUX_IMPLEMENT_OBJECT_TYPE(DNDCollectionWindow);

DNDCollectionWindow::DNDCollectionWindow()
  : nux::BaseWindow("")
  , display(NULL)
{
  // Make it invisible...
  SetBackgroundColor(nux::Color(0x00000000));
  SetOpacity(0.0f);
  // ... and as big as the whole screen.
  WindowManager& wm = WindowManager::Default();
  SetGeometry(wm.GetScreenGeometry());

  ShowWindow(true);
  PushToBack();
  // Hack to create the X Window as soon as possible.
  EnableInputWindow(true, "DNDCollectionWindow");
  EnableInputWindow(false, "DNDCollectionWindow");
  SetDndEnabled(false, true);

  wm.window_moved.connect(sigc::mem_fun(this, &DNDCollectionWindow::OnWindowMoved));
}

DNDCollectionWindow::~DNDCollectionWindow()
{
  for (auto it : mimes_)
    g_free(it);
}

/**
 * EnableInputWindow doesn't show the window immediately.
 * Since nux::EnableInputWindow uses XMoveResizeWindow the best way to know if
 * the X Window is really on/off screen is receiving WindowManager::window_moved
 * signal. Please don't hate me!
 **/
void DNDCollectionWindow::OnWindowMoved(Window window_id)
{
  if (window_id == GetInputWindowId() && display() != NULL)
  {
    // Create a fake mouse move because sometimes an extra one is required.
    XWarpPointer(display(), None, None, 0, 0, 0, 0, 0, 0);
    XFlush(display());
  }
}

void DNDCollectionWindow::Collect()
{
  // Using PushToFront we're sure that the window is shown over the panel window,
  // the launcher window and the dash window. Don't forget to call PushToBack as
  // soon as possible.
  PushToFront();
  EnableInputWindow(true, "DndCollectionWindow");
}

void DNDCollectionWindow::ProcessDndMove(int x, int y, std::list<char*> mimes)
{
  // Hide the window as soon as possible.
  PushToBack();
  EnableInputWindow(false, "DNDCollectionWindow");
  
  // Free mimes_ before fill it again.                       
  for (auto it : mimes_)
    g_free(it);
  mimes_.clear();

  // Duplicate the list.
  for (auto it : mimes)
    mimes_.push_back(g_strdup(it));
  
  // Emit the collected signal.
  collected.emit(mimes_);
}
  
} // namespace unity
