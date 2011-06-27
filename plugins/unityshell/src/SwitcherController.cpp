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

#include "SwitcherController.h"

SwitcherController::SwitcherController(Launcher *launcher)
{
  _model = 0;
  _launcher = launcher;
}

SwitcherController::~SwitcherController()
{

}

void 
SwitcherController::Show (SwitcherController::ShowMode show, SwitcherController::SortMode sort, bool reverse)
{
  LauncherModel::iterator it;
  std::vector<LauncherIcon*> results;
  LauncherModel *model = _launcher->GetModel ();
  
  for (it = model->begin (); it != model->end (); it++)
    if ((*it)->ShowInSwitcher ())
      results.push_back (*it);
  
  if (sort == FOCUS_ORDER)
    std::sort (_inner.begin (), _inner.end (), CompareSwitcherItem);
    
  _model = new SwitcherModel (results);
}

void 
SwitcherController::Hide ()
{

}

bool 
SwitcherController::Visible ()
{

}

void 
SwitcherController::MoveNext ()
{

}

void 
SwitcherController::MovePrev ()
{

}

bool 
SwitcherController::CompareSwitcherItemsPriority (LauncherIcon *first, LauncherIcon *second)
{
  return first->SwitcherPriority () > second->SwitcherPriority ();
}
