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

#ifndef UNITY_LOCKSCREEN_ACCELERATORS
#define UNITY_LOCKSCREEN_ACCELERATORS

#include <memory>
#include <sigc++/signal.h>
#include <vector>


namespace unity
{
namespace lockscreen
{

enum class PressedState : unsigned int;

class Accelerator
{
public:
  typedef std::shared_ptr<Accelerator> Ptr;

  Accelerator(unsigned int keysym, unsigned int keycode, unsigned int modifiers);
  explicit Accelerator(std::string const& string);

  bool operator==(Accelerator const& accelerator) const;

  sigc::signal<void> activated;

private:
  bool KeyPressActivate();
  bool KeyReleaseActivate();

  bool HandleKeyPress(unsigned int keysym,
                      unsigned int modifiers,
                      PressedState pressed_state);
  bool HandleKeyRelease(unsigned int keysym,
                        unsigned int modifiers,
                        PressedState pressed_state);

  unsigned int keysym_;
  unsigned int keycode_;
  unsigned int modifiers_;

  bool active_;
  bool activated_;

  friend class Accelerators;
};

class Accelerators
{
public:
  typedef std::shared_ptr<Accelerators> Ptr;
  typedef std::vector<Accelerator::Ptr> Vector;

  Accelerators();

  Accelerators::Vector GetAccelerators() const;
  void Clear();

  void Add(Accelerator::Ptr const& accelerator);
  void Remove(Accelerator::Ptr const& accelerator);

  bool HandleKeyPress(unsigned int keysym,
                      unsigned int modifiers);
  bool HandleKeyRelease(unsigned int keysym,
                        unsigned int modifiers);

private:
  Accelerators::Vector accelerators_;

  PressedState pressed_state_;
};

} // lockscreen namespace
} // unity namespace

#endif // UNITY_LOCKSCREEN_ACCELERATORS
