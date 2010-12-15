/*
 * Copyright (C) 2010 Canonical Ltd
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include "WindowManager.h"

static WindowManager *window_manager = NULL;

class WindowManagerDummy : public WindowManager
{
  bool IsWindowMaximized (guint32 xid)
  {
    return false;
  }
};

WindowManager *
WindowManager::Default ()
{
  if (!window_manager)
    window_manager = new WindowManagerDummy ();

  return window_manager;
}

void
WindowManager::SetDefault (WindowManager *manager)
{
  window_manager = manager;
}
