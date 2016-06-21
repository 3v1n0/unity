// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2014 Canonical Ltd
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
 * Authored by: William Hua <william.hua@canonical.com>
 */

#ifndef UNITY_LOCKSCREEN_ACCELERATOR_CONTROLLER
#define UNITY_LOCKSCREEN_ACCELERATOR_CONTROLLER

#include "unity-shared/KeyGrabber.h"
#include "LockScreenAccelerators.h"

namespace unity
{
namespace lockscreen
{

class AcceleratorController : public sigc::trackable
{
public:
  typedef std::shared_ptr<AcceleratorController> Ptr;

  AcceleratorController(key::Grabber::Ptr const&);

  Accelerators::Ptr const& GetAccelerators() const;

private:
  void AddAction(CompAction const&);
  void RemoveAction(CompAction const&);
  void OnActionActivated(CompAction&);

  std::vector<std::pair<CompAction, Accelerator::Ptr>> actions_accelerators_;
  Accelerators::Ptr accelerators_;
};

} // lockscreen namespace
} // unity namespace

#endif // UNITY_LOCKSCREEN_ACCELERATOR_CONTROLLER
