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
 * Authored by: Alex Launi <alex.launi@canonical.com>
 */

#include "Autopilot.h"

#include <sigc++/sigc++.h>

#include "AggregateMonitor.h"
#include "DebugDBusInterface.h"
#include "UBusMessages.h"

namespace unity {

UBusServer* _ubus;
GDBusConnection* _dbus;
nux::TimerFunctor* test_expiration_functor;

void Autopilot::RegisterUBusInterest(const std::string signal, TestArgs* args)
{
  args->ubus_handle = ubus_server_register_interest(_ubus,
                                                    signal.c_str(),
                                                    (UBusCallback) &Autopilot::OnTestPassed,
                                                    args);

}

Autopilot::Autopilot(CompScreen* screen, GDBusConnection* connection)
{
  _dbus = connection;
  _ubus = ubus_server_get_default();
}

Autopilot::~Autopilot()
{
}

void Autopilot::StartTest(const std::string name)
{
  TestArgs* args = new TestArgs ();

  if (args == NULL)
  {
    g_error("Failed to allocate memory for TestArgs");
    return;
  }

  if (test_expiration_functor == NULL)
  {
    test_expiration_functor = new nux::TimerFunctor();
    test_expiration_functor->time_expires.connect(sigc::ptr_fun(Autopilot::TestFinished));
  }

  args->name = name;
  args->passed = FALSE;
  args->expiration_handle = nux::GetTimer().AddTimerHandler(TEST_TIMEOUT, test_expiration_functor, args);
  args->monitor = new unity::performance::AggregateMonitor();
  args->monitor->Start();

  if (name == "show_tooltip")
  {
    RegisterUBusInterest(UBUS_TOOLTIP_SHOWN, args);
  }
  else if (name == "show_quicklist")
  {
    RegisterUBusInterest(UBUS_QUICKLIST_SHOWN, args);
  }
  else if (name == "show_dash")
  {
    RegisterUBusInterest(UBUS_PLACE_VIEW_SHOWN, args);
  }
  else if (name == "drag_launcher")
  {
    RegisterUBusInterest(UBUS_LAUNCHER_END_DND, args);
  }
  else if (name == "drag_launcher_icon_along_edge_drop")
  {
    RegisterUBusInterest(UBUS_LAUNCHER_ICON_END_DND, args);
  }
  else if (name == "drag_launcher_icon_out_and_drop")
  {
    RegisterUBusInterest(UBUS_LAUNCHER_ICON_END_DND, args);
  }
  else if (name == "drag_launcher_icon_out_and_move")
  {
    RegisterUBusInterest(UBUS_LAUNCHER_ICON_END_DND, args);
  }
  else
  {
    /* Some anonymous test. Will always get a failed result since we don't really know how to test it */
  }
}

void Autopilot::TestFinished(void* arg)
{
  GError* error = NULL;

  TestArgs* args = (TestArgs*) arg;
  if (args == NULL)
    return;

  ubus_server_unregister_interest (_ubus, args->ubus_handle);
  GVariant* result = g_variant_new("(sb@a{sv})", 
                                   args->name.c_str(), 
                                   args->passed, 
                                   args->monitor->Stop());

  g_dbus_connection_emit_signal(_dbus,
                                NULL,
                                DebugDBusInterface::UNITY_DBUS_DEBUG_OBJECT_PATH.c_str(),
                                DebugDBusInterface::UNITY_DBUS_AP_IFACE_NAME.c_str(),
                                DebugDBusInterface::UNITY_DBUS_AP_SIG_TESTFINISHED.c_str(),
                                result,
                                &error);


  if (error != NULL)
  {
    g_warning("An error was encountered emitting TestFinished signal");
    g_error_free(error);
  }
}

void Autopilot::OnTestPassed(GVariant* payload, TestArgs* val)
{
  TestArgs* args = (TestArgs*) val;

  nux::GetTimer().RemoveTimerHandler(args->expiration_handle);
  args->passed = TRUE;
  TestFinished(args);
}
}
