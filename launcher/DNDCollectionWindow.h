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

#ifndef DNDCOLLECTIONWINDOW_H
#define DNDCOLLECTIONWINDOW_H

#include <list>

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <sigc++/sigc++.h>

namespace unity {

/**
 * DNDCollectionWindow makes it possible to collect drag and drop (dnd) data as
 * soon as dnd starts and not when the mouse pointer enter the x window.
 **/

class DNDCollectionWindow : public nux::BaseWindow
{
NUX_DECLARE_OBJECT_TYPE(DNDCollectionWindow, nux::BaseWindow);

// Methods
public:
  DNDCollectionWindow();
  ~DNDCollectionWindow();

  void Collect();

private:
  void ProcessDndMove(int x, int y, std::list<char*> mimes);
  void OnWindowMoved(Window window_id);

// Members
public:
  nux::Property<Display*> display;

  sigc::signal<void, const std::list<char*>&> collected;

private:
  std::list<char*> mimes_;
};

} // namespace unity

#endif // DNDCOLLECTIONWINDOW_H
