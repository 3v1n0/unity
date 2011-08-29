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

#include <sigc++/sigc++.h>

#include "Autopilot.h"
#include "DebugDBusInterface.h"
#include "UBusMessages.h"

namespace unity {

UBusServer* _ubus;
GDBusConnection* _dbus;
nux::TimerFunctor* test_expiration_functor;

void
TestFinished(void* arg)
{
  GError* error = NULL;
  TestArgs* args = static_cast<TestArgs*>(arg);
  GVariant* result = g_variant_new("(sb@a{sv})", args->name, args->passed, NULL);

  ubus_server_unregister_interest (_ubus, args->ubus_handle);
  g_dbus_connection_emit_signal(_dbus,
                                NULL,
                                UNITY_DBUS_DEBUG_OBJECT_PATH,
                                UNITY_DBUS_AP_IFACE_NAME,
                                UNITY_DBUS_AP_SIG_TESTFINISHED,
                                result,
                                &error);


  if (error != NULL)
  {
    g_warning("An error was encountered emitting TestFinished signal");
    g_error_free(error);
  }
}

void
on_test_passed(GVariant* payload, TestArgs* args)
{
  nux::GetTimer().RemoveTimerHandler(args->expiration_handle);
  args->passed = TRUE;
  TestFinished(args);
}

void
RegisterUBusInterest(const gchar* signal, TestArgs* args)
{
  args->ubus_handle = ubus_server_register_interest(_ubus,
                                                    signal,
                                                    (UBusCallback) on_test_passed,
                                                    args);

}

Autopilot::Autopilot(CompScreen* screen, GDBusConnection* connection)
{
  _dbus = connection;
  _ubus = ubus_server_get_default();
}

Autopilot::~Autopilot()
{
  delete test_expiration_functor;
}

void
Autopilot::StartTest(const gchar* name)
{
  TestArgs* args = static_cast<TestArgs*>(g_malloc(sizeof(TestArgs)));

  if (args == NULL)
  {
    g_error("Failed to allocate memory for TestArgs");
    return;
  }

  if (test_expiration_functor == NULL)
  {
    test_expiration_functor = new nux::TimerFunctor();
    test_expiration_functor->OnTimerExpired.connect(sigc::ptr_fun(&TestFinished));
  }

  args->name = g_strdup(name);
  args->passed = FALSE;
  args->expiration_handle = nux::GetTimer().AddTimerHandler(TEST_TIMEOUT, test_expiration_functor, args);

  if (g_strcmp0(name, "show_tooltip") == 0)
  {
    RegisterUBusInterest(UBUS_TOOLTIP_SHOWN, args);
  }
  else if (g_strcmp0(name, "show_quicklist") == 0)
  {
    RegisterUBusInterest(UBUS_QUICKLIST_SHOWN, args);
  }
  else if (g_strcmp0(name, "drag_launcher") == 0)
  {
    RegisterUBusInterest(UBUS_LAUNCHER_END_DND, args);
  }
  else if (g_strcmp0(name, "drag_launcher_icon_along_edge_drop") == 0)
  {
    RegisterUBusInterest(UBUS_LAUNCHER_ICON_END_DND, args);
  }
  else if (g_strcmp0(name, "drag_launcher_icon_out_and_drop") == 0)
  {
    RegisterUBusInterest(UBUS_LAUNCHER_ICON_END_DND, args);
  }
  else if (g_strcmp0(name, "drag_launcher_icon_out_and_move") == 0)
  {
    RegisterUBusInterest(UBUS_LAUNCHER_ICON_END_DND, args);
  }
  else
  {
    /* Some anonymous test. Will always get a failed result since we don't really know how to test it */
  }
}
}
