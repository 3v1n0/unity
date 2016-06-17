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
* Authored by: Marco Trevisan <marco.trevisan@canonical.com>
*/

#ifndef __UNITY_KEY_GRABBER__
#define __UNITY_KEY_GRABBER__

#include <core/core.h>
#include <sigc++/signal.h>

namespace unity
{
namespace key
{
class Grabber
{
public:
  typedef std::shared_ptr<Grabber> Ptr;
  virtual ~Grabber() = default;

  virtual uint32_t AddAction(CompAction const&) = 0;
  virtual bool RemoveAction(CompAction const&) = 0;
  virtual bool RemoveAction(uint32_t id) = 0;

  sigc::signal<void, CompAction const&> action_added;
  sigc::signal<void, CompAction const&> action_removed;

  virtual CompAction::Vector& GetActions() = 0;
};

} // namespace key
} // namespace unity

#endif // __UNITY_KEY_GRABBER__
