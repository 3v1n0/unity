// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonical Ltd
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
 */

#include "LauncherModel.h"
#include "AbstractLauncherIcon.h"

namespace unity
{
namespace launcher
{

typedef struct
{
  AbstractLauncherIcon* icon;
  LauncherModel* self;
} RemoveArg;

LauncherModel::LauncherModel()
{
  selection_ = 0;
}

LauncherModel::~LauncherModel()
{
  for (auto icon : _inner_shelf)
    icon->UnReference();

  for (auto icon : _inner_main)
    icon->UnReference();
}

bool LauncherModel::IconShouldShelf(AbstractLauncherIcon* icon)
{
  return icon->Type() == AbstractLauncherIcon::TYPE_TRASH;
}

bool LauncherModel::CompareIcons(AbstractLauncherIcon* first, AbstractLauncherIcon* second)
{
  if (first->Type() < second->Type())
    return true;
  else if (first->Type() > second->Type())
    return false;

  return first->SortPriority() < second->SortPriority();
}

bool
LauncherModel::Populate()
{
  Base copy = _inner;

  _inner.clear();

  iterator it, it2;

  int i = 0;
  for (it = main_begin(); it != main_end(); it++)
  {
    _inner.push_back(*it);
    (*it)->SetSortPriority(i++);
  }

  for (it = shelf_begin(); it != shelf_end(); it++)
  {
    _inner.push_back(*it);
    (*it)->SetSortPriority(i++);
  }

  return copy.size() == _inner.size() && !std::equal(begin(), end(), copy.begin());
}

void
LauncherModel::AddIcon(AbstractLauncherIcon* icon)
{
  icon->SinkReference();

  if (IconShouldShelf(icon))
    _inner_shelf.push_back(icon);
  else
    _inner_main.push_back(icon);

  Sort();

  icon_added.emit(icon);

  if (icon->on_icon_removed_connection.connected())
    icon->on_icon_removed_connection.disconnect();
  icon->on_icon_removed_connection = icon->remove.connect(sigc::mem_fun(this, &LauncherModel::OnIconRemove));
}

void
LauncherModel::RemoveIcon(AbstractLauncherIcon* icon)
{
  size_t size;

  _inner_shelf.erase(std::remove(_inner_shelf.begin(), _inner_shelf.end(), icon), _inner_shelf.end());
  _inner_main.erase(std::remove(_inner_main.begin(), _inner_main.end(), icon), _inner_main.end());

  size = _inner.size();
  _inner.erase(std::remove(_inner.begin(), _inner.end(), icon), _inner.end());

  if (size != _inner.size())
  {
    icon_removed.emit(icon);
    icon->UnReference();
  }
}

gboolean
LauncherModel::RemoveCallback(gpointer data)
{
  RemoveArg* arg = (RemoveArg*) data;

  arg->self->RemoveIcon(arg->icon);
  g_free(arg);

  return false;
}

void
LauncherModel::OnIconRemove(AbstractLauncherIcon* icon)
{
  RemoveArg* arg = (RemoveArg*) g_malloc0(sizeof(RemoveArg));
  arg->icon = icon;
  arg->self = this;

  g_timeout_add(1000, &LauncherModel::RemoveCallback, arg);
}

void
LauncherModel::Save()
{
  saved.emit();
}

void
LauncherModel::Sort()
{
  std::sort(_inner_shelf.begin(), _inner_shelf.end(), &LauncherModel::CompareIcons);
  std::sort(_inner_main.begin(), _inner_main.end(), &LauncherModel::CompareIcons);

  if (Populate())
    order_changed.emit();
}

bool
LauncherModel::IconHasSister(AbstractLauncherIcon* icon)
{
  iterator it;
  iterator end;

  if (icon && icon->Type() == AbstractLauncherIcon::TYPE_DEVICE)
    return true;

  if (IconShouldShelf(icon))
  {
    it = shelf_begin();
    end = shelf_end();
  }
  else
  {
    it = main_begin();
    end = main_end();
  }

  for (; it != end; ++it)
  {
    AbstractLauncherIcon* iter_icon = *it;
    if ((iter_icon  != icon)
        && iter_icon->Type() == icon->Type())
      return true;
  }

  return false;
}

void
LauncherModel::ReorderAfter(AbstractLauncherIcon* icon, AbstractLauncherIcon* other)
{
  if (icon == other)
    return;

  int i = 0;
  for (LauncherModel::iterator it = begin(); it != end(); ++it)
  {
    if ((*it) == icon)
      continue;

    if ((*it) == other)
    {
      (*it)->SetSortPriority(i);
      ++i;

      icon->SetSortPriority(i);
      ++i;
    }
    else
    {
      (*it)->SetSortPriority(i);
      ++i;
    }
  }

  Sort();
}

void
LauncherModel::ReorderBefore(AbstractLauncherIcon* icon, AbstractLauncherIcon* other, bool save)
{
  if (icon == other)
    return;

  int i = 0;
  int j = 0;
  for (auto icon_it : _inner)
  {
    if (icon_it == icon)
    {
      j++;
      continue;
    }

    if (icon_it == other)
    {
      icon->SetSortPriority(i);
      if (i != j && save)
        icon_it->SaveCenter();
      i++;

      icon_it->SetSortPriority(i);
      if (i != j && save)
        icon_it->SaveCenter();
      i++;
    }
    else
    {
      icon_it->SetSortPriority(i);
      if (i != j && save)
        icon_it->SaveCenter();
      i++;
    }
    j++;
  }

  Sort();
}

void
LauncherModel::ReorderSmart(AbstractLauncherIcon* icon, AbstractLauncherIcon* other, bool save)
{
  if (icon == other)
    return;

  int i = 0;
  int j = 0;
  bool skipped = false;
  for (auto icon_it : _inner)
  {
    if (icon_it == icon)
    {
      skipped = true;
      j++;
      continue;
    }

    if (icon_it == other)
    {
      if (!skipped)
      {
        icon->SetSortPriority(i);
        if (i != j && save)
          icon_it->SaveCenter();
        i++;
      }

      icon_it->SetSortPriority(i);
      if (i != j && save)
        icon_it->SaveCenter();
      i++;

      if (skipped)
      {
        icon->SetSortPriority(i);
        if (i != j && save)
          icon_it->SaveCenter();
        i++;
      }
    }
    else
    {
      icon_it->SetSortPriority(i);
      if (i != j && save)
        icon_it->SaveCenter();
      i++;
    }
    j++;
  }

  Sort();
}

int
LauncherModel::Size()
{
  return _inner.size();
}

AbstractLauncherIcon* LauncherModel::Selection ()
{
  return _inner[selection_];
}

int LauncherModel::SelectionIndex()
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

    if (_inner[temp]->GetQuirk(AbstractLauncherIcon::QUIRK_VISIBLE))
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

    if (_inner[temp]->GetQuirk(AbstractLauncherIcon::QUIRK_VISIBLE))
    {
      selection_ = temp;
      selection_changed.emit(Selection());
      break;
    }
    temp--;
  }
}


/* iterators */

LauncherModel::iterator
LauncherModel::begin()
{
  return _inner.begin();
}

LauncherModel::iterator
LauncherModel::end()
{
  return _inner.end();
}

LauncherModel::iterator
LauncherModel::at(int index)
{
  LauncherModel::iterator it;
  int i;

  // start currently selected icon
  for (it = _inner.begin(), i = 0; it != _inner.end(); it++, i++)
  {
    if (i == index)
      return it;
  }

  return (LauncherModel::iterator)NULL;
}

LauncherModel::reverse_iterator
LauncherModel::rbegin()
{
  return _inner.rbegin();
}

LauncherModel::reverse_iterator
LauncherModel::rend()
{
  return _inner.rend();
}

LauncherModel::iterator
LauncherModel::main_begin()
{
  return _inner_main.begin();
}

LauncherModel::iterator
LauncherModel::main_end()
{
  return _inner_main.end();
}

LauncherModel::reverse_iterator
LauncherModel::main_rbegin()
{
  return _inner_main.rbegin();
}

LauncherModel::reverse_iterator
LauncherModel::main_rend()
{
  return _inner_main.rend();
}

LauncherModel::iterator
LauncherModel::shelf_begin()
{
  return _inner_shelf.begin();
}

LauncherModel::iterator
LauncherModel::shelf_end()
{
  return _inner_shelf.end();
}

LauncherModel::reverse_iterator
LauncherModel::shelf_rbegin()
{
  return _inner_shelf.rbegin();
}

LauncherModel::reverse_iterator
LauncherModel::shelf_rend()
{
  return _inner_shelf.rend();
}

} // namespace launcher
} // namespace unity
