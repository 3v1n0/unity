// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2012 Canonical Ltd
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
* Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
*/

#ifndef UNITYSHELL_XDND_MANAGER_IMP_H
#define UNITYSHELL_XDND_MANAGER_IMP_H

#include <sigc++/trackable.h>

#include "XdndManager.h"

#include "XdndCollectionWindow.h"
#include "XdndStartStopNotifier.h"
#include "UnityCore/GLibSource.h"

namespace unity {

class XdndManagerImp : public XdndManager, public sigc::trackable {
public:
  XdndManagerImp(XdndStartStopNotifier::Ptr const&, XdndCollectionWindow::Ptr const&);

  virtual int Monitor() const;

private:
  void OnDndStarted();
  void OnDndFinished();
  void OnDndDataCollected(std::vector<std::string> const& mimes);
  bool IsAValidDnd(std::vector<std::string> const& mimes);
  bool CheckMousePosition();

  XdndStartStopNotifier::Ptr xdnd_start_stop_notifier_;
  XdndCollectionWindow::Ptr xdnd_collection_window_;
  int last_monitor_;
  std::string dnd_data_;

  glib::Source::UniquePtr mouse_poller_timeout_;
};

}

#endif
