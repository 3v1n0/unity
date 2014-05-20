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

#ifndef __UNITY_INPUT_MONITOR__
#define __UNITY_INPUT_MONITOR__

#include <X11/Xlib.h>
#include <sigc++/slot.h>
#include <memory>

namespace unity
{
namespace input
{
enum class Events : unsigned
{
  POINTER = (1 << 0),
  KEYS = (1 << 1),
  BARRIER = (1 << 2),
  INPUT = POINTER | KEYS,
  ALL = POINTER | KEYS | BARRIER
};

class Monitor : public sigc::trackable
{
public:
  typedef sigc::slot<void, XEvent const&> EventCallback;

  static Monitor& Get();

  Monitor();
  virtual ~Monitor();

  void RegisterClient(Events, EventCallback const&);
  void UnregisterClient(EventCallback const&);

private:
  Monitor(Monitor const&) = delete;
  Monitor& operator=(Monitor const&) = delete;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // input namespace
} // unity namespace

namespace std
{
template<>
struct hash<unity::input::Monitor::EventCallback>
{
  size_t operator()(unity::input::Monitor::EventCallback const& cb) const
  {
    if (cb.rep_)
      return std::hash<size_t>()(reinterpret_cast<size_t>(cb.rep_->call_));

    return std::hash<size_t>()(reinterpret_cast<size_t>(cb.rep_));
  }
};

template<>
struct equal_to<unity::input::Monitor::EventCallback>
{
  bool operator()(unity::input::Monitor::EventCallback const& lhs, unity::input::Monitor::EventCallback const& rhs) const
  {
    if (!lhs.rep_ || !rhs.rep_)
      return (lhs.rep_ == rhs.rep_);

    return (lhs.rep_->call_ == rhs.rep_->call_);
  }
};
} // std namespace

#endif // __UNITY_INPUT_MONITOR__
