// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 */

#ifndef UNITY_TESTWRAPPER_STANDALONE_WM_H
#define UNITY_TESTWRAPPER_STANDALONE_WM_H

#include "StandaloneWindowManager.h"

namespace unity
{
namespace testwrapper
{
struct StandaloneWM
{
  StandaloneWM()
    : WM(StandaloneWM::Get())
  {}

  static StandaloneWindowManager* Get()
  {
    return dynamic_cast<StandaloneWindowManager*>(&WindowManager::Default());
  }

  ~StandaloneWM()
  {
    WM->ResetStatus();
  }

  StandaloneWindowManager* operator->() const
  {
    return WM;
  }

  StandaloneWindowManager* WM;

private:
  StandaloneWM(StandaloneWM const&) = delete;
  StandaloneWM& operator=(StandaloneWM const&) = delete;
};

} // unity namespace
} // tests namespace

#endif // UNITY_TESTWRAPPER_STANDALONE_WM_H
