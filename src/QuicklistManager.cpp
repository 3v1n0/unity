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
 * Authored by: Jamal Fanaian <j@jamalfanaian.com>
 */

#include "Nux/Nux.h"
#include "Nux/BaseWindow.h"
#include "Nux/VLayout.h"
#include "Nux/WindowCompositor.h"
#include "QuicklistView.h"
#include "QuicklistManager.h"

QuicklistManager::QuicklistManager ()
{
  _current_quicklist = 0;
}

QuicklistManager::~QuicklistManager ()
{
}

QuicklistView *QuicklistManager::Current ()
{
  return _current_quicklist;
}

void QuicklistManager::RegisterQuicklist (QuicklistView *quicklist)
{
  std::list<QuicklistView*>::iterator it;

  for (it = _quicklist_list.begin(); it != _quicklist_list.end(); it++)
  {
    if (*it == quicklist)
    {
      // quicklist has already been registered
      return;
    }
  }

  _quicklist_list.push_back (quicklist);

  quicklist->sigVisible.connect (sigc::mem_fun (this, &QuicklistManager::RecvShowQuicklist));
  quicklist->sigHidden.connect (sigc::mem_fun (this, &QuicklistManager::RecvHideQuicklist));
}

void QuicklistManager::ShowQuicklist (QuicklistView *quicklist, int tip_x, 
                                      int tip_y, bool hide_existing_if_open)
{
  if (_current_quicklist == quicklist) 
  {
    // this quicklist is already active
    // do we want to still redraw in case the position has changed?
    return;
  }

  if (hide_existing_if_open && _current_quicklist)
  {
    HideQuicklist (_current_quicklist);
  }

  quicklist->ShowQuicklistWithTipAt (tip_x, tip_y);
}

void QuicklistManager::HideQuicklist (QuicklistView *quicklist)
{
  quicklist->Hide();
}

void QuicklistManager::RecvShowQuicklist (nux::BaseWindow *window)
{
  QuicklistView *quicklist = (QuicklistView*) window;
  _current_quicklist = quicklist;
}

void QuicklistManager::RecvHideQuicklist (nux::BaseWindow *window)
{
  if (_current_quicklist == window) 
  { 
    _current_quicklist = 0;
  }
}

