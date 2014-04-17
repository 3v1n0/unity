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

namespace unity
{
namespace lockscreen
{

enum
{
  LeftShiftPressed    = 0x01,
  LeftControlPressed  = 0x02,
  LeftAltPressed      = 0x04,
  LeftSuperPressed    = 0x08,
  RightShiftPressed   = 0x10,
  RightControlPressed = 0x20,
  RightAltPressed     = 0x40,
  RightSuperPressed   = 0x80
};

class Accelerator
{
public:
  Accelerator(unsigned int keysym, unsigned int keycode, unsigned int modifiers);
  explicit Accelerator(std::string const& string);

  bool operator==(Accelerator const& accelerator) const;

  sigc::signal<void> Activate;

private:
  bool HandleKeyPress(unsigned int keysym,
                      unsigned int modifiers,
                      unsigned int press_state);
  bool HandleKeyRelease(unsigned int keysym,
                        unsigned int modifiers,
                        unsigned int press_state);

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

  Accelerators();

  void Clear();

  void Add(Accelerator const& accelerator);
  void Remove(Accelerator const& accelerator);

  bool HandleKeyPress(unsigned int keysym,
                      unsigned int modifiers);
  bool HandleKeyRelease(unsigned int keysym,
                        unsigned int modifiers);

private:
  std::list<Accelerator> accelerators_;

  unsigned int press_state_;
};

} // lockscreen namespace
} // unity namespace

#endif // UNITY_LOCKSCREEN_ACCELERATORS
