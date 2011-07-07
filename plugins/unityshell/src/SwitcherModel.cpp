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

#include <math.h>

#include "SwitcherModel.h"

namespace unity {
namespace switcher {

SwitcherModel::SwitcherModel(std::vector<AbstractLauncherIcon*> icons)
{
  _inner = icons;
  _index = 0;
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

int 
SwitcherModel::Size ()
{
  return _inner.size ();
}

AbstractLauncherIcon *
SwitcherModel::Selected ()
{
  return _inner.at (_index);
}

int 
SwitcherModel::SelectedIndex ()
{
  return _index;
}

void 
SwitcherModel::Next ()
{
  _index++;
  if (_index >= _inner.size ())
    _index = 0;
}

void 
SwitcherModel::Prev ()
{
  if (_index > 0)
    _index--;
  else
    _index = _inner.size () - 1;
}

void 
SwitcherModel::Select (AbstractLauncherIcon *selection)
{
  int i = 0;
  for (iterator it = begin(), e = end(); it != e; ++it)
  {
    if (*it == selection)
    {
      _index = i;
      break;
    }
    ++i;
  }
}

void 
SwitcherModel::Select (int index)
{
  _index = CLAMP (index, 0, _inner.size () - 1);
}

}
}
