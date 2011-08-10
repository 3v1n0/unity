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
{
  SetBackgroundColor(nux::Color(0x00000000));
  SetBaseSize(screen->width(), screen->height());
  SetBaseXY(0, 0);
            
  EnableInputWindow(true, "DNDCollectionWindow");
  SetDndEnabled(false, true);
  ShowWindow(true);
}

DNDCollectionWindow::~DNDCollectionWindow()
{
  for (auto it : mimes_)
    g_free(it);
}

void DNDCollectionWindow::ProcessDndMove(int x, int y, std::list<char*> mimes)
{  
  // Hide the window as soon as possible
  EnableInputWindow(false, "DNDCollectionWindow", false, false);
  ShowWindow(false);
  
  // Duplicate the list
  for (auto it : mimes)
    mimes_.push_back(g_strdup(it));
  
  collected.emit(mimes_);
}
  
} // namespace unity
