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
 * Authored by: Jamal Fanaian <j@jamalfanaian.com>
 */

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <Nux/WindowCompositor.h>

#include "QuicklistView.h"
#include "QuicklistManager.h"

#include "unity-shared/UBusWrapper.h"
#include "unity-shared/UBusMessages.h"

namespace unity
{

QuicklistManager* QuicklistManager::_default = 0;

/* static */
QuicklistManager* QuicklistManager::Default()
{
  if (!_default)
    _default = new QuicklistManager();
  return _default;
}

void QuicklistManager::Destroy()
{
  delete _default;
  _default = 0;
}

QuicklistManager::QuicklistManager()
{
  _current_quicklist = 0;
}

QuicklistManager::~QuicklistManager()
{
}

nux::ObjectPtr<QuicklistView> QuicklistManager::Current()
{
  return _current_quicklist;
}

bool QuicklistManager::RegisterQuicklist(nux::ObjectPtr<QuicklistView> const& quicklist)
{
  if (std::find(_quicklist_list.begin(), _quicklist_list.end(), quicklist) != _quicklist_list.end())
  {
    // quicklist has already been registered
    g_warning("Attempted to register a quicklist that was previously registered");
    return false;
  }

  _quicklist_list.push_back(quicklist);

  quicklist->sigVisible.connect(sigc::mem_fun(this, &QuicklistManager::RecvShowQuicklist));
  quicklist->sigHidden.connect(sigc::mem_fun(this, &QuicklistManager::RecvHideQuicklist));

  return true;
}

void QuicklistManager::ShowQuicklist(nux::ObjectPtr<QuicklistView> const& quicklist, int tip_x,
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
    HideQuicklist(_current_quicklist);
  }

  quicklist->ShowQuicklistWithTipAt(tip_x, tip_y);
  nux::GetWindowCompositor().SetKeyFocusArea(quicklist.GetPointer());
}

void QuicklistManager::HideQuicklist(nux::ObjectPtr<QuicklistView> const& quicklist)
{
  quicklist->Hide();
}

void QuicklistManager::RecvShowQuicklist(nux::BaseWindow* window)
{
  QuicklistView* quicklist = (QuicklistView*) window;

  _current_quicklist = quicklist;

  quicklist_opened.emit(nux::ObjectPtr<QuicklistView>(quicklist));
  UBusManager::SendMessage(UBUS_QUICKLIST_SHOWN);
}

void QuicklistManager::RecvHideQuicklist(nux::BaseWindow* window)
{
  QuicklistView* quicklist = (QuicklistView*) window;

  if (_current_quicklist == quicklist)
  {
    _current_quicklist = 0;
  }

  quicklist_closed.emit(nux::ObjectPtr<QuicklistView>(quicklist));
}

} // NAMESPACE
