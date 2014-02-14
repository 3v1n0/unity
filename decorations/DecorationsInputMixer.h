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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef UNITY_DECORATION_INPUT_MIXER
#define UNITY_DECORATION_INPUT_MIXER

#include "DecorationsWidgets.h"

namespace unity
{
namespace decoration
{

class InputMixer
{
public:
  typedef std::shared_ptr<InputMixer> Ptr;
  InputMixer();

  void PushToFront(Item::Ptr const&);
  void PushToBack(Item::Ptr const&);
  void Remove(Item::Ptr const&);

  Item::List const& Items() const;
  Item::Ptr const& GetMouseOwner() const;

  void EnterEvent(CompPoint const&);
  void MotionEvent(CompPoint const&, Time);
  void LeaveEvent(CompPoint const&);
  void ButtonDownEvent(CompPoint const&, unsigned button, Time);
  void ButtonUpEvent(CompPoint const&, unsigned button, Time);
  void UngrabPointer();

private:
  InputMixer(InputMixer const&) = delete;
  InputMixer& operator=(InputMixer const&) = delete;

  void UpdateMouseOwner(CompPoint const&);
  void UnsetMouseOwner();
  Item::Ptr GetMatchingItem(CompPoint const&);
  Item::Ptr GetMatchingItemRecursive(Item::List const&, CompPoint const&);

  Item::List items_;
  Item::Ptr last_mouse_owner_;
  bool mouse_down_;
  bool recheck_owner_;
};

} // decoration namespace
} // unity namespace

#endif
