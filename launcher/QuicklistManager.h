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

#ifndef QUICKLISTMANAGER_H
#define QUICKLISTMANAGER_H

#include <NuxCore/ObjectPtr.h>
#include <sigc++/sigc++.h>

namespace nux { class BaseWindow; }

namespace unity {

class QuicklistView;

class QuicklistManager : public sigc::trackable
{
public:
  static QuicklistManager* Default();
  static void Destroy();

  ~QuicklistManager();

  nux::ObjectPtr<QuicklistView> Current();

  bool RegisterQuicklist(nux::ObjectPtr<QuicklistView> const& quicklist);
  void ShowQuicklist(nux::ObjectPtr<QuicklistView> const& quicklist, int tip_x, int tip_y, bool hide_existing_if_open = true);
  void HideQuicklist(nux::ObjectPtr<QuicklistView> const& quicklist);

  void RecvShowQuicklist(nux::BaseWindow* window);
  void RecvHideQuicklist(nux::BaseWindow* window);

  sigc::signal<void, nux::ObjectPtr<QuicklistView> const&> quicklist_opened;
  sigc::signal<void, nux::ObjectPtr<QuicklistView> const&> quicklist_closed;

private:
  QuicklistManager();
  static QuicklistManager* _default;

  std::list<nux::ObjectWeakPtr<QuicklistView>> _quicklist_list;
  nux::ObjectPtr<QuicklistView> _current_quicklist;

};

} // NAMESPACE
#endif

