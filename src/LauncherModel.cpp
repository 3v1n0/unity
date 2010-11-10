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

void 
LauncherModel::AddIcon (LauncherIcon *icon)
{
  icon->SinkReference ();
  
  _inner.push_front (icon);
  icon_added.emit (icon);
  
  icon->remove.connect (sigc::mem_fun (this, &LauncherModel::OnIconRemove));
}

void
LauncherModel::RemoveIcon (LauncherIcon *icon)
{
  size_t size = _inner.size ();
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
LauncherModel::OnIconRemove (void *icon_pointer)
{
  RemoveArg *arg = (RemoveArg*) g_malloc0 (sizeof (RemoveArg));
  arg->icon = (LauncherIcon*) icon_pointer;
  arg->self = this;
  
  g_timeout_add (1000, &LauncherModel::RemoveCallback, arg);
}

void 
LauncherModel::Sort (SortFunc func)
{
  _inner.sort (func);
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
