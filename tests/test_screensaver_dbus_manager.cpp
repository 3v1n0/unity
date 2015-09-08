// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2014 Canonical Ltd
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

#include <gtest/gtest.h>
using namespace testing;


#include "lockscreen/LockScreenSettings.h"
#include "lockscreen/ScreenSaverDBusManager.h"
#include "lockscreen/ScreenSaverDBusManagerImpl.h"
#include "test_mock_session_manager.h"
#include "test_utils.h"

#include "GLibDBusProxy.h"
#include "Variant.h"

namespace unity
{
namespace lockscreen
{

struct TestScreenSaverDBusManager : Test
{
  TestScreenSaverDBusManager()
    : session_manager(std::make_shared<session::MockManager>())
    , sc_dbus_manager(std::make_shared<WrapperDBusManager>(session_manager))
  {
    gs_proxy = std::make_shared<glib::DBusProxy>("org.gnome.Test.ScreenSaver",
                                                 "/org/gnome/ScreenSaver",
                                                 "org.gnome.ScreenSaver");

    Utils::WaitUntilMSec([this] { return gs_proxy->IsConnected(); });
  }

  struct WrapperDBusManager : DBusManager
  {
    WrapperDBusManager(session::Manager::Ptr const& session_manager)
      : DBusManager(session_manager, DBusManager::TestMode())
    {}

    using DBusManager::impl_;
  };

  unity::lockscreen::Settings lockscreen_settings;
  session::MockManager::Ptr session_manager;
  WrapperDBusManager::Ptr sc_dbus_manager;
  glib::DBusProxy::Ptr gs_proxy;
};

TEST_F(TestScreenSaverDBusManager, Contructor)
{
  EXPECT_FALSE(sc_dbus_manager->active());
}

TEST_F(TestScreenSaverDBusManager, ContructorLegacy)
{
  lockscreen_settings.use_legacy = true;
  auto sc_dbus_manager_tmp = std::make_shared<WrapperDBusManager>(session_manager);

  ASSERT_EQ(nullptr, sc_dbus_manager_tmp->impl_->server_.get());
}

TEST_F(TestScreenSaverDBusManager, ContructorNoLegacy)
{
  lockscreen_settings.use_legacy = false;
  auto sc_dbus_manager_tmp = std::make_shared<WrapperDBusManager>(session_manager);

  ASSERT_NE(nullptr, sc_dbus_manager_tmp->impl_->server_.get());
}

TEST_F(TestScreenSaverDBusManager, Lock)
{
  EXPECT_CALL(*session_manager, LockScreen())
    .Times(1);

  bool call_finished = false;
  gs_proxy->Call("Lock", nullptr, [&call_finished](GVariant*) {
    call_finished = true;
  });

  Utils::WaitUntil(call_finished);
}

TEST_F(TestScreenSaverDBusManager, SetActiveTrue)
{
  EXPECT_CALL(*session_manager, ScreenSaverActivate())
    .Times(1);

  bool call_finished = false;
  gs_proxy->Call("SetActive", g_variant_new("(b)", TRUE), [&call_finished](GVariant*) {
    call_finished = true;
  });

  Utils::WaitUntil(call_finished);
}

TEST_F(TestScreenSaverDBusManager, SetActiveFalse)
{
  EXPECT_CALL(*session_manager, ScreenSaverDeactivate())
    .Times(1);

  bool call_finished = false;
  gs_proxy->Call("SetActive", g_variant_new("(b)", FALSE), [&call_finished](GVariant*) {
    call_finished = true;
  });

  Utils::WaitUntil(call_finished);
}

TEST_F(TestScreenSaverDBusManager, GetActive)
{
  bool call_finished = false;
  bool active = false;

  auto get_active_cb = [&call_finished, &active](GVariant* value) {
    call_finished = true;
    active = glib::Variant(value).GetBool();
  };

  gs_proxy->Call("GetActive", nullptr, get_active_cb);

  Utils::WaitUntil(call_finished);
  ASSERT_FALSE(active);

  /* simulate SetActive */
  sc_dbus_manager->active = true;

  call_finished = false;
  gs_proxy->Call("GetActive", nullptr, get_active_cb);

  Utils::WaitUntil(call_finished);
  ASSERT_TRUE(active);
}

TEST_F(TestScreenSaverDBusManager, GetActiveTime)
{
  bool call_finished = false;
  time_t active_time;

  auto get_active_time_cb = [&call_finished, &active_time](GVariant* value) {
    call_finished = true;
    active_time = glib::Variant(value).GetUInt32();
  };

  gs_proxy->Call("GetActiveTime", nullptr, get_active_time_cb);

  Utils::WaitUntil(call_finished);
  ASSERT_EQ(0, active_time);

  /* simulate SetActive */
  sc_dbus_manager->active = true;

  // GetActiveTime counts the number of seconds so we need to wait a while.
  Utils::WaitForTimeout(2);

  call_finished = false;
  gs_proxy->Call("GetActiveTime", nullptr, get_active_time_cb);

  Utils::WaitUntil(call_finished);
  ASSERT_NE(0, active_time);
}

TEST_F(TestScreenSaverDBusManager, ActiveChanged)
{
  bool call_finished = false;
  time_t active;

  gs_proxy->Connect("ActiveChanged", [&call_finished, &active](GVariant* value) {
    call_finished = true;
    active = glib::Variant(value).GetBool();
  });

  /* simulate SetActive */
  sc_dbus_manager->active = true;

  Utils::WaitUntil(call_finished);
  ASSERT_TRUE(active);

  /* simulate SetActive */
  call_finished = false;
  sc_dbus_manager->active = false;

  Utils::WaitUntil(call_finished);
  ASSERT_FALSE(active);
}



}
}
