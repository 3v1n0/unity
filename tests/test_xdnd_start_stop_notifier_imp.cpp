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

#include <gmock/gmock.h>
using namespace testing;

#include "XdndStartStopNotifierImp.h"

#include <Nux/Nux.h>
#include <X11/Xlib.h>
//#include <X11/extensions/XTest.h>

#include "unity-shared/WindowManager.h"
#include "test_utils.h"

namespace {

struct TestXdndStartStopNotifierImp : public Test {
  TestXdndStartStopNotifierImp()
    : display_(nux::GetGraphicsDisplay()->GetX11Display())
    , selection_(XInternAtom(display_, "XdndSelection", false))
  {
    Window root = DefaultRootWindow(display_);
    owner_= XCreateSimpleWindow(display_, root, -1000, -1000, 10, 10, 0, 0, 0);
  }

  Display* display_;
  Atom selection_;
  Window owner_;

  unity::XdndStartStopNotifierImp xdnd_start_stop_notifier;
};

TEST_F(TestXdndStartStopNotifierImp, DISABLED_SignalStarted)
{
  bool signal_received = false;
  xdnd_start_stop_notifier.started.connect([&](){
    signal_received = true;
  });

  XSetSelectionOwner(display_, selection_, owner_, CurrentTime);
  //XTestFakeButtonEvent(display_, 1, True, CurrentTime);
  auto& wm = unity::WindowManager::Default();
  wm.window_mapped.emit(0);

  Utils::WaitUntil(signal_received);
  //XTestFakeButtonEvent(display_, 1, False, CurrentTime);
}

TEST_F(TestXdndStartStopNotifierImp, DISABLED_SignalFinished)
{
  bool signal_received = false;
  xdnd_start_stop_notifier.finished.connect([&](){
    signal_received = true;
  });

  XSetSelectionOwner(display_, selection_, owner_, CurrentTime);
  //XTestFakeButtonEvent(display_, 1, True, CurrentTime);
  auto& wm = unity::WindowManager::Default();
  wm.window_mapped.emit(0);

  Utils::WaitForTimeoutMSec(500);

  XSetSelectionOwner(display_, selection_, None, CurrentTime);
  //XTestFakeButtonEvent(display_, 1, False, CurrentTime);
  wm.window_unmapped.emit(0);

  Utils::WaitUntil(signal_received);
}

TEST_F(TestXdndStartStopNotifierImp, DISABLED_SignalFinished_QT)
{
  bool signal_received = false;
  xdnd_start_stop_notifier.finished.connect([&](){
    signal_received = true;
  });

  XSetSelectionOwner(display_, selection_, owner_, CurrentTime);
  //XTestFakeButtonEvent(display_, 1, True, CurrentTime);
  auto& wm = unity::WindowManager::Default();
  wm.window_mapped.emit(0);

  Utils::WaitForTimeoutMSec(500);

  //XTestFakeButtonEvent(display_, 1, False, CurrentTime);
  wm.window_unmapped.emit(0);

  Utils::WaitUntil(signal_received);
}

}
