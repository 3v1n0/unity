/*
 * Copyright (C) 2011 Canonical Ltd
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

#include <math.h>

#include "SwitcherModel.h"

namespace unity {
namespace switcher {

SwitcherModel::SwitcherModel(std::vector<AbstractLauncherIcon*> icons)
: _inner (icons)
, _index (0)
, _last_index (0)
{
  _change_time.tv_sec = 0;
  _change_time.tv_nsec = 0;
}

SwitcherModel::~SwitcherModel()
{

}

SwitcherModel::iterator 
SwitcherModel::begin ()
{
  return _inner.begin ();
}

SwitcherModel::iterator 
SwitcherModel::end ()
{
  return _inner.end ();
}

SwitcherModel::reverse_iterator 
SwitcherModel::rbegin ()
{
  return _inner.rbegin ();
}

SwitcherModel::reverse_iterator 
SwitcherModel::rend ()
{
  return _inner.rend ();
}

int 
SwitcherModel::Size ()
{
  return _inner.size ();
}

AbstractLauncherIcon *
SwitcherModel::Selection ()
{
  return _inner.at (_index);
}

int 
SwitcherModel::SelectionIndex ()
{
  return _index;
}

AbstractLauncherIcon *
SwitcherModel::LastSelection ()
{
  return _inner.at (_last_index);
}

int 
SwitcherModel::LastSelectionIndex ()
{
  return _last_index;
}

void 
SwitcherModel::Next ()
{
  _last_index = _index;

  _index++;
  if (_index >= _inner.size ())
    _index = 0;
  
  clock_gettime (CLOCK_MONOTONIC, &_change_time);
  selection_changed.emit (Selection ());
}

void 
SwitcherModel::Prev ()
{
  _last_index = _index;

  if (_index > 0)
    _index--;
  else
    _index = _inner.size () - 1;
  
  clock_gettime (CLOCK_MONOTONIC, &_change_time);
  selection_changed.emit (Selection ());
}

timespec
SwitcherModel::SelectionChangeTime ()
{
  return _change_time;
}

void 
SwitcherModel::Select (AbstractLauncherIcon *selection)
{
  int i = 0;
  for (iterator it = begin(), e = end(); it != e; ++it)
  {
    if (*it == selection)
    {
      if ((int) _index != i)
      {
        _last_index = _index;
        _index = i;

        clock_gettime (CLOCK_MONOTONIC, &_change_time);
        selection_changed.emit (Selection ());
      }
      break;
    }
    ++i;
  }
}

void 
SwitcherModel::Select (int index)
{
  unsigned int target = CLAMP (index, 0, _inner.size () - 1);

  if (target != _index)
  {
    _last_index = _index;
    _index = target;
  }
}

}
}
