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

#ifndef INDICATOR_OBJECT_ENTRY_PROXY_H
#define INDICATOR_OBJECT_ENTRY_PROXY_H

#include <string>
#include <sigc++/signal.h>

#include <gdk/gdk.h>

class IndicatorObjectEntryProxy
{
public:

  virtual const char * GetLabel () = 0;
  virtual GdkPixbuf  * GetPixbuf () = 0;
  virtual void         SetActive (bool active) = 0;
  virtual bool         GetActive () = 0;
  virtual void         ShowMenu (int x, int y, guint32 timestamp) = 0;

  // Signals
  sigc::signal<void> Updated;

public:
  bool label_visible;
  bool label_sensitive;
  bool icon_visible;
  bool icon_sensitive;
};

#endif // INDICATOR_OBJECT_ENTRY_PROXY_H
