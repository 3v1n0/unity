// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
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

//#include "config.h"
//#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <Nux/Nux.h>
//#include <Nux/NuxTimerTickSource.h>
#include <Nux/WindowThread.h>
//#include <NuxCore/AnimationController.h>

#include "LockScreenController.h"
//#include "unity-shared/UnitySettings.h"

class MockSessionManager : public unity::session::Manager
{
public:
  std::string RealName() const { return ""; }
  std::string UserName() const { return ""; }

  void LockScreen() {}
  void Logout() {}
  void Reboot() {}
  void Shutdown() {}
  void Suspend() {}
  void Hibernate() {}

  void CancelAction() {}

  bool CanShutdown() const {return true;}
  bool CanSuspend() const {return true;}
  bool CanHibernate() const {return true;}
};

struct LockScreenWindow
{
  LockScreenWindow()
    : wt(nux::CreateGUIThread("Unity LockScreen Controller", 1024, 300, 0, &LockScreenWindow::ThreadWidgetInit, this))
    //, animation_controller(tick_source)
  {}

  void Show()
  {
    wt->Run(nullptr);
  }

private:
  void Init();

  static void ThreadWidgetInit(nux::NThread* thread, void* self)
  {
    static_cast<LockScreenWindow*>(self)->Init();
  }

  //unity::Settings settings;
  std::shared_ptr<nux::WindowThread> wt;
  //nux::NuxTimerTickSource tick_source;
  //nux::animation::AnimationController animation_controller;
  std::shared_ptr<unity::lockscreen::Controller> controller;
};

void LockScreenWindow::Init()
{
  auto manager = std::make_shared<MockSessionManager>();
  controller = std::make_shared<unity::lockscreen::Controller>(manager);
  manager->lock_requested.emit();
}

int main(int argc, char** argv)
{
  gtk_init(&argc, &argv);

  nux::NuxInitialize(0);
  LockScreenWindow().Show();

  return 0;
}
