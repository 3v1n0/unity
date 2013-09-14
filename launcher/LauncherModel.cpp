// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2012 Canonical Ltd
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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "LauncherModel.h"
#include "AbstractLauncherIcon.h"

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Variant.h>

namespace unity
{
namespace launcher
{

LauncherModel::LauncherModel()
  : selection_(0)
{}

std::string LauncherModel::GetName() const
{
  return "LauncherModel";
}

void LauncherModel::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper(builder)
  .add("selection", selection_);
}

unity::debug::Introspectable::IntrospectableList LauncherModel::GetIntrospectableChildren()
{
  introspection_results_.clear();

  for (auto icon : _inner)
    if (!icon->removed)
      introspection_results_.push_back(icon.GetPointer());

  return introspection_results_;
}

bool LauncherModel::IconShouldShelf(AbstractLauncherIcon::Ptr const& icon) const
{
  return icon->position() == AbstractLauncherIcon::Position::END;
}

bool LauncherModel::CompareIcons(AbstractLauncherIcon::Ptr const& first, AbstractLauncherIcon::Ptr const& second)
{
  if (first->position() < second->position())
    return true;
  else if (first->position() > second->position())
    return false;

  return first->SortPriority() < second->SortPriority();
}

void LauncherModel::PopulatePart(iterator begin, iterator end)
{
  AbstractLauncherIcon::Ptr prev_icon;

  for (auto it = begin; it != end; ++it)
  {
    auto const& icon = *it;
    _inner.push_back(icon);

    if (prev_icon)
    {
      // Ensuring that the current icon has higher priority than previous one
      if (icon->SortPriority() < prev_icon->SortPriority())
      {
        int new_priority = prev_icon->SortPriority() + 1;
        icon->SetSortPriority(new_priority);
      }
    }

    prev_icon = icon;
  }
}

bool LauncherModel::Populate()
{
  Base copy = _inner;
  _inner.clear();
  PopulatePart(main_begin(), main_end());
  PopulatePart(shelf_begin(), shelf_end());

  return copy.size() == _inner.size() && !std::equal(begin(), end(), copy.begin());
}

void LauncherModel::AddIcon(AbstractLauncherIcon::Ptr const& icon)
{
  if (!icon || std::find(begin(), end(), icon) != end())
    return;

  if (IconShouldShelf(icon))
    _inner_shelf.push_back(icon);
  else
    _inner_main.push_back(icon);

  Sort();

  icon_added.emit(icon);
  icon->on_icon_removed_connection = icon->remove.connect(sigc::mem_fun(this, &LauncherModel::OnIconRemove));
}

void LauncherModel::RemoveIcon(AbstractLauncherIcon::Ptr const& icon)
{
  size_t size;

  _inner_shelf.erase(std::remove(_inner_shelf.begin(), _inner_shelf.end(), icon), _inner_shelf.end());
  _inner_main.erase(std::remove(_inner_main.begin(), _inner_main.end(), icon), _inner_main.end());

  size = _inner.size();
  _inner.erase(std::remove(_inner.begin(), _inner.end(), icon), _inner.end());

  if (size != _inner.size())
  {
    icon_removed.emit(icon);
  }
}

void LauncherModel::OnIconRemove(AbstractLauncherIcon::Ptr const& icon)
{
  icon->removed = true;

  timeouts_.AddTimeout(1000, [this, icon] {
    RemoveIcon(icon);
    return false;
  });
}

void LauncherModel::Save()
{
  saved.emit();
}

void LauncherModel::Sort()
{
  std::stable_sort(_inner_shelf.begin(), _inner_shelf.end(), &LauncherModel::CompareIcons);
  std::stable_sort(_inner_main.begin(), _inner_main.end(), &LauncherModel::CompareIcons);

  if (Populate())
    order_changed.emit();
}

bool LauncherModel::IconHasSister(AbstractLauncherIcon::Ptr const& icon) const
{
  if (!icon)
    return false;

  auto const& container = IconShouldShelf(icon) ? _inner_shelf : _inner_main;

  for (auto const& icon_it : container)
  {
    if (icon_it != icon && icon_it->GetIconType() == icon->GetIconType())
      return true;
  }

  return false;
}

void LauncherModel::ReorderAfter(AbstractLauncherIcon::Ptr const& icon, AbstractLauncherIcon::Ptr const& other)
{
  if (icon == other || icon.IsNull() || other.IsNull())
    return;

  if (icon->position() != other->position())
    return;

  icon->SetSortPriority(other->SortPriority() + 1);

  for (auto it = std::next(std::find(begin(), end(), other)); it != end(); ++it)
  {
    auto const& icon_it = *it;

    if (icon_it == icon)
      continue;

    // Increasing the priority of the icons next to the other one
    int new_priority = icon_it->SortPriority() + 2;
    icon_it->SetSortPriority(new_priority);
  }

  Sort();
}

void LauncherModel::ReorderBefore(AbstractLauncherIcon::Ptr const& icon, AbstractLauncherIcon::Ptr const& other, bool animate)
{
  if (icon == other || icon.IsNull() || other.IsNull())
    return;

  if (icon->position() != other->position())
    return;

  bool found_target = false;
  bool center = false;

  for (auto const& icon_it : _inner)
  {
    if (icon_it == icon)
    {
      center = !center;
      continue;
    }

    int old_priority = icon_it->SortPriority();
    int new_priority = old_priority + (found_target ? 1 : -1);

    // We need to reduce the priority of all the icons previous to 'other'
    if (icon_it != other && !found_target && other->SortPriority() == old_priority)
      new_priority -= 1;

    icon_it->SetSortPriority(new_priority);

    if (icon_it == other)
    {
      if (animate && center)
        icon_it->SaveCenter();

      center = !center;
      new_priority += -1;
      icon->SetSortPriority(new_priority);

      if (animate && center)
        icon_it->SaveCenter();

      found_target = true;
    }
    else
    {
      if (animate && center)
        icon_it->SaveCenter();
    }
  }

  Sort();
}

void LauncherModel::ReorderSmart(AbstractLauncherIcon::Ptr const& icon, AbstractLauncherIcon::Ptr const& other, bool animate)
{
  if (icon == other || icon.IsNull() || other.IsNull())
    return;

  if (icon->position() != other->position())
    return;

  bool found_icon = false;
  bool found_target = false;
  bool center = false;

  for (auto const& icon_it : _inner)
  {
    if (icon_it == icon)
    {
      found_icon = true;
      center = !center;
      continue;
    }

    int old_priority = icon_it->SortPriority();
    int new_priority = old_priority + (found_target ? 1 : -1);

    // We need to reduce the priority of all the icons previous to 'other'
    if (icon_it != other && !found_target && other->SortPriority() == old_priority)
      new_priority -= 1;

    icon_it->SetSortPriority(new_priority);

    if (icon_it == other)
    {
      if (animate && center)
        icon_it->SaveCenter();

      center = !center;
      new_priority += found_icon ? 1 : -1;
      icon->SetSortPriority(new_priority);

      if (animate && center)
        icon_it->SaveCenter();

      found_target = true;
    }
    else
    {
      if (animate && center)
        icon_it->SaveCenter();
    }
  }

  Sort();
}

int
LauncherModel::Size() const
{
  return _inner.size();
}

AbstractLauncherIcon::Ptr const& LauncherModel::Selection() const
{
  return _inner[selection_];
}

int LauncherModel::SelectionIndex() const
{
  return selection_;
}

void LauncherModel::SetSelection(int selection)
{
  int new_selection = std::min<int>(Size() - 1, std::max<int> (0, selection));

  if (new_selection == selection_)
    return;

  selection_ = new_selection;
  selection_changed.emit(Selection());
}

void LauncherModel::SelectNext()
{
  int temp = selection_;

  temp++;
  while (temp != selection_)
  {
    if (temp >= Size())
      temp = 0;

    if (_inner[temp]->IsVisible())
    {
      selection_ = temp;
      selection_changed.emit(Selection());
      break;
    }
    temp++;
  }
}

void LauncherModel::SelectPrevious()
{
  int temp = selection_;

  temp--;
  while (temp != selection_)
  {
    if (temp < 0)
      temp = Size() - 1;

    if (_inner[temp]->IsVisible())
    {
      selection_ = temp;
      selection_changed.emit(Selection());
      break;
    }
    temp--;
  }
}

AbstractLauncherIcon::Ptr LauncherModel::GetClosestIcon(AbstractLauncherIcon::Ptr const& icon, bool& is_before) const
{
  AbstractLauncherIcon::Ptr prev, next;
  bool found_target = false;

  for (auto const& current : _inner)
  {
    if (current->position() != icon->position())
      continue;

    if (!found_target)
    {
      if (current == icon)
      {
        found_target = true;

        if (prev)
          break;
      }
      else
      {
        prev = current;
      }
    }
    else
    {
      next = current;
      break;
    }
  }

  is_before = next.IsNull();

  return is_before ? prev : next;
}

int LauncherModel::IconIndex(AbstractLauncherIcon::Ptr const& target) const
{
  int pos = 0;
  bool found = false;

  for (auto const& icon : _inner)
  {
    if (icon == target)
    {
      found = true;
      break;
    }

    ++pos;
  }

  return found ? pos : -1;
}

/* iterators */

LauncherModel::iterator LauncherModel::begin()
{
  return _inner.begin();
}

LauncherModel::iterator LauncherModel::end()
{
  return _inner.end();
}

LauncherModel::iterator LauncherModel::at(int index)
{
  LauncherModel::iterator it;
  int i;

  // start currently selected icon
  for (it = _inner.begin(), i = 0; it != _inner.end(); ++it, i++)
  {
    if (i == index)
      return it;
  }

  return (LauncherModel::iterator)NULL;
}

LauncherModel::reverse_iterator LauncherModel::rbegin()
{
  return _inner.rbegin();
}

LauncherModel::reverse_iterator LauncherModel::rend()
{
  return _inner.rend();
}

LauncherModel::iterator LauncherModel::main_begin()
{
  return _inner_main.begin();
}

LauncherModel::iterator LauncherModel::main_end()
{
  return _inner_main.end();
}

LauncherModel::reverse_iterator LauncherModel::main_rbegin()
{
  return _inner_main.rbegin();
}

LauncherModel::reverse_iterator LauncherModel::main_rend()
{
  return _inner_main.rend();
}

LauncherModel::iterator LauncherModel::shelf_begin()
{
  return _inner_shelf.begin();
}

LauncherModel::iterator LauncherModel::shelf_end()
{
  return _inner_shelf.end();
}

LauncherModel::reverse_iterator LauncherModel::shelf_rbegin()
{
  return _inner_shelf.rbegin();
}

LauncherModel::reverse_iterator LauncherModel::shelf_rend()
{
  return _inner_shelf.rend();
}

} // namespace launcher
} // namespace unity
