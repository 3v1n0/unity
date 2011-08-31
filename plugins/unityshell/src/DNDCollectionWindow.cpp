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
#include "WindowManager.h"

namespace unity {
  
NUX_IMPLEMENT_OBJECT_TYPE(DNDCollectionWindow);

DNDCollectionWindow::DNDCollectionWindow()
  : nux::BaseWindow("")
  , display(NULL)
{
  SetBackgroundColor(nux::Color(0x00000000));
  SetGeometry(WindowManager::Default()->GetScreenGeometry());
  
  ShowWindow(true);
  PushToBack();
  // Hack
  EnableInputWindow(true, "DNDCollectionWindow");
  EnableInputWindow(false, "DNDCollectionWindow");
  SetDndEnabled(false, true);
  
  // Enable input window doesn not show the window immediately. Catching the
  // window_moved signal avoid using a timeout.
  WindowManager::Default()->window_moved.connect(sigc::mem_fun(this, &DNDCollectionWindow::OnWindowMoved));
}

DNDCollectionWindow::~DNDCollectionWindow()
{
  for (auto it : mimes_)
    g_free(it);
}

void DNDCollectionWindow::OnWindowMoved(guint32 xid)
{
  if (xid == GetInputWindowId() && display() != NULL)
  {
    XWarpPointer(display(), None, None, 0, 0, 0, 0, 0, 0);
    XFlush(display());
  }
}

void DNDCollectionWindow::Collect()
{
  PushToFront();
  EnableInputWindow(true, "DndCollectionWindow");
}

void DNDCollectionWindow::ProcessDndMove(int x, int y, std::list<char*> mimes)
{    
  // Hide the window as soon as possible
  PushToBack();
  EnableInputWindow(false, "DNDCollectionWindow");
                         
  for (auto it : mimes_)
    g_free(it);
  mimes_.clear();

  // Duplicate the list
  for (auto it : mimes)
    mimes_.push_back(g_strdup(it));
  
  collected.emit(mimes_);
}
  
} // namespace unity
