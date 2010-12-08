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
 */

#include "Nux/Nux.h"
#include "Nux/VLayout.h"
#include "Nux/WindowCompositor.h"
#include "QuicklistManager.h"

QuicklistManager::QuicklistManager ()
{
  _current_quicklist = 0;
}

QuicklistManager::~QuicklistManager ()
{
  // TODO: Lots of stuff to free up
}

QuicklistView *QuicklistManager::Current ()
{
  return _current_quicklist;
}

void QuicklistManager::RegisterQuicklist (QuicklistView *quicklist)
{
  printf("--QuicklistManager::RegisterQuicklist\n");

  // TODO: Check for duplicate quicklist items
  _quicklist_list.push_back (quicklist);
}

void QuicklistManager::ShowQuicklist (QuicklistView *quicklist, int tip_x, int tip_y, bool hide_existing_if_open)
{
  printf("--QuicklistManager::ShowQuicklist\n");

  if (_current_quicklist == quicklist) 
  {
    // this quicklist is already active
    return;
  }

  _current_quicklist = quicklist;

  quicklist->ShowQuicklistWithTipAt (tip_x, tip_y);

  quicklist->EnableInputWindow (true, 1);
  quicklist->GrabPointer ();

  nux::GetWindowCompositor ().SetAlwaysOnFrontWindow (quicklist);

  quicklist->NeedRedraw ();
}

void QuicklistManager::HideQuicklist (QuicklistView *quicklist)
{
  printf("--QuicklistManager::HideQuicklist\n");

  quicklist->EnableInputWindow (false);
  quicklist->CaptureMouseDownAnyWhereElse (false);
  quicklist->ShowWindow(false);

  if (_current_quicklist == quicklist) 
  { 
    _current_quicklist = 0;
  }

  if (_current_quicklist) 
  {
    printf("dangling current\n");
  }
}

