// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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

#ifndef SWITCHERCONTROLLER_H
#define SWITCHERCONTROLLER_H

#include "SwitcherModel.h"
#include "SwitcherView.h"

#include <boost/shared_ptr.hpp>
#include <sigc++/sigc++.h>

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <Nux/WindowCompositor.h>

namespace unity
{
namespace switcher
{

class SwitcherController : public sigc::trackable
{

public:
  enum SortMode
  {
    LAUNCHER_ORDER,
    FOCUS_ORDER,
  };

  enum ShowMode
  {
    ALL,
    CURRENT_VIEWPORT,
  };

  SwitcherController();
  virtual ~SwitcherController();

  void Show(ShowMode show, SortMode sort, bool reverse, std::vector<AbstractLauncherIcon*> results);
  void Hide();

  bool Visible();

  void MoveNext();
  void MovePrev();

  void DetailCurrent();

  void SelectFirstItem();

  void SetWorkspace(nux::Geometry geo);

  WindowRenderTargetList ExternalRenderTargets ();

private:
  void ConstructView();

  SwitcherModel::Ptr model_;
  SwitcherView* view_;

  nux::Geometry workarea_;

  nux::BaseWindow* view_window_;

  bool visible_;
  guint show_timer_;

  static gboolean OnShowTimer(gpointer data);

  static bool CompareSwitcherItemsPriority(AbstractLauncherIcon* first, AbstractLauncherIcon* second);

};

}
}

#endif // SWITCHERCONTROLLER_H

