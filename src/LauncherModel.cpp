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
#include "Launcher.h"

typedef struct
{
  LauncherIcon *icon;
  LauncherModel *self;
} RemoveArg;

LauncherModel::LauncherModel()
{
}

LauncherModel::~LauncherModel()
{
}

bool LauncherModel::IconShouldShelf (LauncherIcon *icon)
{
  return icon->Type () == LAUNCHER_ICON_TYPE_TRASH; 
}

void
LauncherModel::Populate ()
{
  _inner.clear ();
  
  iterator it;
  
  for (it = main_begin (); it != main_end (); it++)
    _inner.push_back (*it);
  
  for (it = shelf_begin (); it != shelf_end (); it++)
    _inner.push_back (*it);
}

void 
LauncherModel::AddIcon (LauncherIcon *icon)
{
  icon->SinkReference ();
  
  if (IconShouldShelf (icon))
    _inner_shelf.push_front (icon);
  else
    _inner_main.push_front (icon);
  
  Populate ();
  
  icon_added.emit (icon);
  icon->remove.connect (sigc::mem_fun (this, &LauncherModel::OnIconRemove));
}

void
LauncherModel::RemoveIcon (LauncherIcon *icon)
{
  size_t size;
  
  _inner_shelf.remove (icon);
  _inner_main.remove (icon);
  
  size = _inner.size ();
  _inner.remove (icon);

  if (size != _inner.size ())
    icon_removed.emit (icon);
  
  icon->UnReference ();
}

gboolean
LauncherModel::RemoveCallback (gpointer data)
{
  RemoveArg *arg = (RemoveArg*) data;

  arg->self->RemoveIcon (arg->icon);
  g_free (arg);
  
  return false;
}

void 
LauncherModel::OnIconRemove (LauncherIcon *icon)
{
  RemoveArg *arg = (RemoveArg*) g_malloc0 (sizeof (RemoveArg));
  arg->icon = icon;
  arg->self = this;
  
  g_timeout_add (1000, &LauncherModel::RemoveCallback, arg);
}

void 
LauncherModel::Sort (SortFunc func)
{
  _inner.sort (func);
  _inner_main.sort (func);
  
  Populate ();
  order_changed.emit ();
}

int
LauncherModel::Size ()
{
  return _inner.size ();
}
    
LauncherModel::iterator 
LauncherModel::begin ()
{
  return _inner.begin ();
}

LauncherModel::iterator 
LauncherModel::end ()
{
  return _inner.end ();
}

LauncherModel::reverse_iterator 
LauncherModel::rbegin ()
{
  return _inner.rbegin ();
}

LauncherModel::reverse_iterator 
LauncherModel::rend ()
{
  return _inner.rend ();
}

LauncherModel::iterator 
LauncherModel::main_begin ()
{
  return _inner_main.begin ();
}

LauncherModel::iterator 
LauncherModel::main_end ()
{
  return _inner_main.end ();
}

LauncherModel::reverse_iterator 
LauncherModel::main_rbegin ()
{
  return _inner_main.rbegin ();
}

LauncherModel::reverse_iterator 
LauncherModel::main_rend ()
{
  return _inner_main.rend ();
}

LauncherModel::iterator 
LauncherModel::shelf_begin ()
{
  return _inner_shelf.begin ();
}

LauncherModel::iterator 
LauncherModel::shelf_end ()
{
  return _inner_shelf.end ();
}

LauncherModel::reverse_iterator 
LauncherModel::shelf_rbegin ()
{
  return _inner_shelf.rbegin ();
}

LauncherModel::reverse_iterator 
LauncherModel::shelf_rend ()
{
  return _inner_shelf.rend ();
}
