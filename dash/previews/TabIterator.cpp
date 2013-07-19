// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *              Manuel de la Pena <manuel.delapena@canonical.com>
 *
 */

#include "TabIterator.h"

namespace unity
{
namespace dash
{
namespace previews
{

void TabIterator::Remove(nux::InputArea* area)
{
  areas_.remove(area);
}

void TabIterator::Prepend(nux::InputArea* area)
{
  Remove(area);
  areas_.push_front(area);
}

void TabIterator::Append(nux::InputArea* area)
{
  Remove(area);
  areas_.push_back(area);
}

void TabIterator::Insert(nux::InputArea* area, unsigned index)
{
  Remove(area);
  std::list<nux::InputArea*>::iterator it = areas_.begin();
  if (index < areas_.size())
  {
    std::advance(it, index);
    areas_.insert(it, area);
  }
  else
    areas_.push_back(area);
}

void TabIterator::InsertBefore(nux::InputArea* area, nux::InputArea* after)
{
  Remove(area);
  std::list<nux::InputArea*>::iterator it = std::find(areas_.begin(), areas_.end(),
    after);
  areas_.insert(it, area);
}

void TabIterator::InsertAfter(nux::InputArea* area, nux::InputArea* before)
{
  Remove(area);
  std::list<nux::InputArea*>::iterator it = std::find(areas_.begin(), areas_.end(),
    before);

  if (it != areas_.end())
    ++it;

  areas_.insert(it, area);
}

std::list<nux::InputArea*> const& TabIterator::GetTabAreas() const { return areas_; }

nux::InputArea* TabIterator::DefaultFocus() const
{
  if (areas_.empty())
    return nullptr;

  return *areas_.begin();
}

nux::InputArea* TabIterator::FindKeyFocusArea(unsigned int key_symbol,
                                              unsigned long x11_key_code,
                                              unsigned long special_keys_state)
{
  if (areas_.empty())
    return nullptr;

  nux::InputArea* current_focus_area = nux::GetWindowCompositor().GetKeyFocusArea();
  auto it = std::find(areas_.begin(), areas_.end(), current_focus_area);
  if (it != areas_.end())
    return current_focus_area;

  return *areas_.begin();
}

nux::Area* TabIterator::KeyNavIteration(nux::KeyNavDirection direction)
{
  if (areas_.empty())
    return nullptr;

  if (direction != nux::KEY_NAV_TAB_PREVIOUS && direction != nux::KEY_NAV_TAB_NEXT)
  {
    return nullptr;
  }

  nux::InputArea* current_focus_area = nux::GetWindowCompositor().GetKeyFocusArea();

  if (current_focus_area)
  {
    auto it = std::find(areas_.begin(), areas_.end(), current_focus_area);
    if (direction == nux::KEY_NAV_TAB_PREVIOUS)
    {
      if (it == areas_.begin())
        return *areas_.rbegin();
      else
      {
        it--;
        if (it == areas_.begin())
          return *areas_.rbegin();
        return *it;
      }
    }
    else if (direction == nux::KEY_NAV_TAB_NEXT)
    {
      if (it == areas_.end())
      {
        return *areas_.begin();
      }
      else
      {
        it++;
        if (it == areas_.end())
        {
          return *areas_.begin();
        }
        return *it;
      }
    }
  }
  else
  {
    if (direction == nux::KEY_NAV_TAB_PREVIOUS)
    {
      return *areas_.rbegin();
    }
    else if (direction == nux::KEY_NAV_TAB_NEXT)
    {
      return *areas_.begin();
    }
  }

  return nullptr;
}

} // previews

} // dash

} // unity

