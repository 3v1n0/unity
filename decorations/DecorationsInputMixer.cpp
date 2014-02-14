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

#include <NuxCore/Logger.h>
#include "DecorationsInputMixer.h"

namespace unity
{
namespace decoration
{
namespace
{
DECLARE_LOGGER(logger, "unity.decorations.inputmixer");
}

InputMixer::InputMixer()
  : mouse_down_(false)
  , recheck_owner_(false)
{}

void InputMixer::PushToFront(Item::Ptr const& item)
{
  if (!item)
    return;

  auto it = std::find(items_.begin(), items_.end(), item);
  if (it != items_.end())
    items_.erase(it);

  items_.push_front(item);
}

void InputMixer::PushToBack(Item::Ptr const& item)
{
  if (!item)
    return;

  auto it = std::find(items_.begin(), items_.end(), item);
  if (it != items_.end())
    items_.erase(it);

  items_.push_back(item);
}

void InputMixer::Remove(Item::Ptr const& item)
{
  if (item == last_mouse_owner_)
    UnsetMouseOwner();

  auto it = std::find(items_.begin(), items_.end(), item);
  if (it != items_.end())
    items_.erase(it);
}

Item::List const& InputMixer::Items() const
{
  return items_;
}

void InputMixer::ForceMouseOwnerCheck()
{
  if (!mouse_down_)
  {
    UpdateMouseOwner(CompPoint(pointerX, pointerY));
  }
  else
  {
    recheck_owner_ = true;
  }
}

Item::Ptr InputMixer::GetMatchingItemRecursive(Item::List const& items, CompPoint const& point)
{
  for (auto const& item : items)
  {
    if (item && item->visible() && item->Geometry().contains(point))
    {
      if (!item->IsContainer())
        return item->sensitive() ? item : nullptr;

      auto const& container = std::static_pointer_cast<BasicContainer>(item);
      auto const& child = GetMatchingItemRecursive(container->Items(), point);

      if (child)
        return child;
    }
  }

  return nullptr;
}

Item::Ptr InputMixer::GetMatchingItem(CompPoint const& point)
{
  return GetMatchingItemRecursive(items_, point);
}

void InputMixer::UpdateMouseOwner(CompPoint const& point)
{
  if (Item::Ptr const& item = GetMatchingItem(point))
  {
    if (item != last_mouse_owner_)
    {
      UnsetMouseOwner();
      last_mouse_owner_ = item;
      item->mouse_owner = true;
    }
  }
  else
  {
    UnsetMouseOwner();
  }
}

void InputMixer::UnsetMouseOwner()
{
  if (!last_mouse_owner_)
    return;

  last_mouse_owner_->mouse_owner = false;
  last_mouse_owner_ = nullptr;
}

void InputMixer::UngrabPointer()
{
  mouse_down_ = false;
  UnsetMouseOwner();
}

Item::Ptr const& InputMixer::GetMouseOwner() const
{
  return last_mouse_owner_;
};

void InputMixer::EnterEvent(CompPoint const& point)
{
  if (!mouse_down_)
    UpdateMouseOwner(point);
}

void InputMixer::LeaveEvent(CompPoint const& point)
{
  if (!mouse_down_)
    UnsetMouseOwner();
}

void InputMixer::MotionEvent(CompPoint const& point, Time timestamp)
{
  if (!mouse_down_)
    UpdateMouseOwner(point);

  if (last_mouse_owner_)
    last_mouse_owner_->MotionEvent(point, timestamp);
}

void InputMixer::ButtonDownEvent(CompPoint const& point, unsigned button, Time timestamp)
{
  mouse_down_ = true;

  if (last_mouse_owner_)
    last_mouse_owner_->ButtonDownEvent(point, button, timestamp);
}

void InputMixer::ButtonUpEvent(CompPoint const& point, unsigned button, Time timestamp)
{
  mouse_down_ = false;

  if (last_mouse_owner_)
  {
    // This event might cause the LastMouseOwner to be deleted, so we protect using a weak_ptr
    Item::WeakPtr weak_last_mouse_owner(last_mouse_owner_);
    last_mouse_owner_->ButtonUpEvent(point, button, timestamp);

    if (weak_last_mouse_owner && !last_mouse_owner_->Geometry().contains(point))
    {
      UpdateMouseOwner(point);
    }
    else if (recheck_owner_)
    {
      recheck_owner_ = false;
      UpdateMouseOwner(point);
    }
  }
}

} // decoration namespace
} // unity namespace
