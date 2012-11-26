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

#ifndef UNITYSHELL_XDND_START_STOP_NOTIFIER_IMP_H
#define UNITYSHELL_XDND_START_STOP_NOTIFIER_IMP_H

#include "XdndStartStopNotifier.h"

#include <UnityCore/GLibSource.h>
#include <X11/Xlib.h>

namespace unity {

class XdndStartStopNotifierImp : public XdndStartStopNotifier {
public:
  XdndStartStopNotifierImp();

private:
  void DndTimeoutSetup();
  bool OnTimeout();

  Display* display_;
  Atom selection_;
  bool dnd_in_progress_;

  glib::Source::UniquePtr timeout_;
};

}

#endif