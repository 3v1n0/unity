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

namespace unity {
  
NUX_IMPLEMENT_OBJECT_TYPE(DNDCollectionWindow);

DNDCollectionWindow::DNDCollectionWindow(CompScreen* screen)
  : nux::BaseWindow("")
  , screen_(screen)
  , force_mouse_move_handle_(0)
{
  SetBackgroundColor(nux::Color(0x00000000));
  SetBaseSize(screen_->width(), screen->height());
  SetBaseXY(0, 0);
  
  ShowWindow(true);
  PushToBack();
  // Hack
  EnableInputWindow(true, "DNDCollectionWindow");
  EnableInputWindow(false, "DNDCollectionWindow");
  SetDndEnabled(false, true);
}

DNDCollectionWindow::~DNDCollectionWindow()
{
  for (auto it : mimes_)
    g_free(it);
    
  if (force_mouse_move_handle_)
  {
    g_source_remove(force_mouse_move_handle_);
    force_mouse_move_handle_ = 0;
  }
}

gboolean DNDCollectionWindow::ForceMouseMoveTimeout(gpointer data)
{
  DNDCollectionWindow* self = (DNDCollectionWindow*) data;  
  self->force_mouse_move_handle_ = 0;
  
  XWarpPointer (self->screen_->dpy(), None, None, 0, 0, 0, 0, 0, 0);
  XFlush(self->screen_->dpy());
  
  return FALSE;
}

void DNDCollectionWindow::Collect()
{
  PushToFront();
  EnableInputWindow(true, "DndCollectionWindow");
  
  if (!force_mouse_move_handle_)
    g_timeout_add(10, &ForceMouseMoveTimeout, this);
}

void DNDCollectionWindow::ProcessDndMove(int x, int y, std::list<char*> mimes)
{
  if (force_mouse_move_handle_)
  {
    g_source_remove(force_mouse_move_handle_);
    force_mouse_move_handle_ = 0;
  }
    
  // Hide the window as soon as possible
  PushToBack();
  EnableInputWindow(false, "DNDCollectionWindow");
  XSync(screen_->dpy(), False);

  XWarpPointer(screen_->dpy(), None, None, 0, 0, 0, 0, 0, 0);
    
  for (auto it : mimes_)
    g_free(it);
  mimes_.clear();

  // Duplicate the list
  for (auto it : mimes)
    mimes_.push_back(g_strdup(it));
  
  collected.emit(mimes_);
}
  
} // namespace unity
