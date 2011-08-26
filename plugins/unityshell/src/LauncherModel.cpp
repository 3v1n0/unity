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
#include "LauncherIcon.h"

typedef struct
{
  LauncherIcon* icon;
  LauncherModel* self;
} RemoveArg;

LauncherModel::LauncherModel()
{
}

LauncherModel::~LauncherModel()
{
  for (iterator it = _inner_shelf.begin(); it != _inner_shelf.end(); ++it)
    reinterpret_cast<LauncherIcon*>(*it)->UnReference();
  _inner_shelf.clear();

  for (iterator it = _inner_main.begin(); it != _inner_main.end(); ++it)
    reinterpret_cast<LauncherIcon*>(*it)->UnReference();
  _inner_main.clear();

  _inner.clear();
}

bool LauncherModel::IconShouldShelf(LauncherIcon* icon)
{
  return icon->Type() == LauncherIcon::TYPE_TRASH;
}

bool LauncherModel::CompareIcons(LauncherIcon* first, LauncherIcon* second)
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

  return !std::equal(begin(), end(), copy.begin());
}

void
LauncherModel::AddIcon(LauncherIcon* icon)
{
  icon->SinkReference();

  if (IconShouldShelf(icon))
    _inner_shelf.push_front(icon);
  else
    _inner_main.push_front(icon);

  Sort();

  icon_added.emit(icon);

  if (icon->on_icon_removed_connection.connected())
    icon->on_icon_removed_connection.disconnect();
  icon->on_icon_removed_connection = icon->remove.connect(sigc::mem_fun(this, &LauncherModel::OnIconRemove));
}

void
LauncherModel::RemoveIcon(LauncherIcon* icon)
{
  size_t size;

  _inner_shelf.remove(icon);
  _inner_main.remove(icon);

  size = _inner.size();
  _inner.remove(icon);

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
LauncherModel::OnIconRemove(LauncherIcon* icon)
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
  _inner_shelf.sort(&LauncherModel::CompareIcons);
  _inner_main.sort(&LauncherModel::CompareIcons);

  if (Populate())
    order_changed.emit();
}

bool
LauncherModel::IconHasSister(LauncherIcon* icon)
{
  iterator(LauncherModel::*begin_it)(void);
  iterator(LauncherModel::*end_it)(void);
  iterator it;

  if (IconShouldShelf(icon))
  {
    begin_it = &LauncherModel::shelf_begin;
    end_it = &LauncherModel::shelf_end;
  }
  else
  {
    begin_it = &LauncherModel::main_begin;
    end_it = &LauncherModel::main_end;
  }

  for (it = (this->*begin_it)(); it != (this->*end_it)(); it++)
  {
    if ((*it  != icon)
        && (*it)->Type() == icon->Type())
      return true;
  }

  return false;
}

void
LauncherModel::ReorderBefore(LauncherIcon* icon, LauncherIcon* other, bool save)
{
  if (icon == other)
    return;

  LauncherModel::iterator it;

  int i = 0;
  int j = 0;
  for (it = begin(); it != end(); it++)
  {
    if ((*it) == icon)
    {
      j++;
      continue;
    }

    if ((*it) == other)
    {
      icon->SetSortPriority(i);
      if (i != j && save)
        (*it)->SaveCenter();
      i++;

      (*it)->SetSortPriority(i);
      if (i != j && save)
        (*it)->SaveCenter();
      i++;
    }
    else
    {
      (*it)->SetSortPriority(i);
      if (i != j && save)
        (*it)->SaveCenter();
      i++;
    }
    j++;
  }

  Sort();
}

void
LauncherModel::ReorderSmart(LauncherIcon* icon, LauncherIcon* other, bool save)
{
  if (icon == other)
    return;

  LauncherModel::iterator it;

  int i = 0;
  int j = 0;
  bool skipped = false;
  for (it = begin(); it != end(); it++)
  {
    if ((*it) == icon)
    {
      skipped = true;
      j++;
      continue;
    }

    if ((*it) == other)
    {
      if (!skipped)
      {
        icon->SetSortPriority(i);
        if (i != j && save)
          (*it)->SaveCenter();
        i++;
      }

      (*it)->SetSortPriority(i);
      if (i != j && save)
        (*it)->SaveCenter();
      i++;

      if (skipped)
      {
        icon->SetSortPriority(i);
        if (i != j && save)
          (*it)->SaveCenter();
        i++;
      }
    }
    else
    {
      (*it)->SetSortPriority(i);
      if (i != j && save)
        (*it)->SaveCenter();
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
