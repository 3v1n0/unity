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
 * Authored by: Marco Trevisan <marco@ubuntu.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <Nux/Nux.h>
#include <Nux/NuxTimerTickSource.h>
#include <Nux/WindowThread.h>
#include <NuxCore/AnimationController.h>

#include "unity-shared/BackgroundEffectHelper.h"
#include "SessionController.h"
#include "unity-shared/UnitySettings.h"

using namespace unity;

class MockSessionManager : public session::Manager
{
public:
  MockSessionManager() = default;

  std::string RealName() const { return "Marco Trevisan"; }
  std::string UserName() const { return "marco"; }

  void LockScreen() { std::cout << "LockScreen" << std::endl; }
  void Logout() { std::cout << "Logout" << std::endl; }
  void Reboot() { std::cout << "Reboot" << std::endl; }
  void Shutdown() { std::cout << "Shutdown" << std::endl; }
  void Suspend() { std::cout << "Suspend" << std::endl; }
  void Hibernate() { std::cout << "Hibernate" << std::endl; }

  void CancelAction() { std::cout << "CancelAction" << std::endl; }

  bool CanShutdown() const {return true;}
  bool CanSuspend() const {return true;}
  bool CanHibernate() const {return true;}
};

struct SessionWindow
{
  SessionWindow()
    : wt(nux::CreateGUIThread("Unity Session Controller", 1024, 300, 0, &SessionWindow::ThreadWidgetInit, this))
    , animation_controller(tick_source)
  {}

  void Show()
  {
    wt->Run(nullptr);
  }

private:
  void Init();

  static void ThreadWidgetInit(nux::NThread* thread, void* self)
  {
    static_cast<SessionWindow*>(self)->Init();
  }

  unity::Settings settings;
  std::shared_ptr<nux::WindowThread> wt;
  nux::NuxTimerTickSource tick_source;
  nux::animation::AnimationController animation_controller;
  session::Controller::Ptr controller;
};

void SessionWindow::Init()
{
  BackgroundEffectHelper::blur_type = BLUR_NONE;
  auto manager = std::make_shared<MockSessionManager>();
  controller = std::make_shared<session::Controller>(manager);
  manager->shutdown_requested.emit(false);
}

int main(int argc, char** argv)
{
  gtk_init(&argc, &argv);

  nux::NuxInitialize(0);
  SessionWindow().Show();

  return 0;
}
