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

#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include <glib.h>
#include <sigc++/sigc++.h>

class WindowManager
{
public:

  static WindowManager * Default ();
  static void            SetDefault (WindowManager *manager);

  virtual bool IsWindowMaximized (guint32 xid) = 0;
  virtual bool IsWindowDecorated (guint32 xid) = 0;

  virtual void Restore (guint32 xid) = 0;
  virtual void Minimize (guint32 xid) = 0;
  virtual void Close (guint32 xid) = 0;

  // Signals
  sigc::signal<void, guint32> window_mapped;
  sigc::signal<void, guint32> window_unmapped;
  sigc::signal<void, guint32> window_maximized;
  sigc::signal<void, guint32> window_restored;
};

#endif // WINDOW_MANAGER_H
