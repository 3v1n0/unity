/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>

 */

#ifndef TEST_USCREEN_MOCK_H
#define TEST_USCREEN_MOCK_H

#include "MultiMonitor.h"
#include "UScreen.h"

namespace unity
{

const unsigned MONITOR_WIDTH = 1024;
const unsigned MONITOR_HEIGHT = 768;

class MockUScreen : public UScreen
{
public:
  MockUScreen()
  {
    Reset(false);
  }

  ~MockUScreen()
  {
    if (default_screen_ == this)
      default_screen_ = nullptr;
  }

  void Reset(bool emit = true)
  {
    default_screen_ = this;
    primary_ = 0;
    monitors_ = {nux::Geometry(0, 0, MONITOR_WIDTH, MONITOR_HEIGHT)};

    changed.emit(primary_, monitors_);
  }

  void SetupFakeMultiMonitor(int primary = 0, bool emit_update = true)
  {
    SetPrimary(primary, false);
    monitors_.clear();

    for (int i = 0, total_width = 0; i < max_num_monitors; ++i)
    {
      monitors_.push_back(nux::Geometry(MONITOR_WIDTH, MONITOR_HEIGHT, total_width, 0));
      total_width += MONITOR_WIDTH;

      if (emit_update)
        changed.emit(GetPrimaryMonitor(), GetMonitors());
    }
  }

  void SetPrimary(int primary, bool emit = true)
  {
    if (primary_ != primary)
    {
      primary_ = primary;

      if (emit)
        changed.emit(primary_, monitors_);
    }
  }
};

}

#endif