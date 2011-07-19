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
 * Authored by: Alex Launi <alex.launi@canonical.com>
 */

#include <Nux/Nux.h>
#include <Nux/HLayout.h>
#include <Nux/BaseWindow.h>
#include <Nux/WindowCompositor.h>
#include <Nux/WindowThread.h>

#include "Autopilot.h"
#include "DebugDBusInterface.h"
#include "unityshell.h"

#define SI_METHOD_NAME_GETSTATE  "GetState"
#define AP_METHOD_NAME_STARTTEST "StartTest"

void StartTest (const gchar*);
GVariant* GetState (const gchar*);
void DBusMethodCall (GDBusConnection*, const gchar*, const gchar*,
                     const gchar*, const gchar*, GVariant*,
                     GDBusMethodInvocation*, gpointer);

static const GDBusInterfaceVTable si_vtable =
{
  &DBusMethodCall,
  NULL,
  NULL
};

static const GDBusArgInfo si_getstate_in_args =
{
  -1,
  (gchar*) "piece",
  (gchar*) "s",
  NULL
};

static const GDBusArgInfo *const si_getstate_in_arg_pointers[] = { &si_getstate_in_args, NULL };

static const GDBusArgInfo si_getstate_out_args =
{
  -1,
  (gchar*) "state",
  (gchar*) "a{sv}",
  NULL
};
static const GDBusArgInfo *const si_getstate_out_arg_pointers[] = { &si_getstate_out_args, NULL };

static const GDBusMethodInfo si_method_info_getstate =
{
  -1,
  (gchar*) SI_METHOD_NAME_GETSTATE,
  (GDBusArgInfo **) &si_getstate_in_arg_pointers,
  (GDBusArgInfo **) &si_getstate_out_arg_pointers,
  NULL
};

static const GDBusArgInfo ap_starttest_in_args =
{
  -1,
  (gchar*) "name",
  (gchar*) "s",
  NULL
};
static const GDBusArgInfo *const ap_starttest_in_arg_pointers[] = { &ap_starttest_in_args, NULL };

static GDBusMethodInfo ap_method_info_starttest =
{
  -1,
  (gchar*) AP_METHOD_NAME_STARTTEST,
  (GDBusArgInfo **) &ap_starttest_in_arg_pointers,
  NULL,
  NULL
};

static const GDBusMethodInfo *const si_method_info_pointers [] = { &si_method_info_getstate, NULL };
static const GDBusMethodInfo *const ap_method_info_pointers [] = { &ap_method_info_starttest, NULL };

static GDBusArgInfo ap_testfinished_arg_name = 
{
  -1,
  (gchar*) "name",
  (gchar*) "s",
  NULL
};

static GDBusArgInfo ap_testfinished_arg_passed = 
{
  -1,
  (gchar*) "passed",
  (gchar*) "b",
  NULL
};

static const GDBusArgInfo *const ap_signal_testfinished_arg_pointers [] = { &ap_testfinished_arg_name,
                                                                            &ap_testfinished_arg_passed,
                                                                            NULL };
static GDBusSignalInfo ap_signal_info_testfinished = 
{
  -1,
  (gchar*) UNITY_DBUS_AP_SIG_TESTFINISHED,
  (GDBusArgInfo **) &ap_signal_testfinished_arg_pointers,
  NULL
};

static const GDBusSignalInfo *const ap_signal_info_pointers [] = { &ap_signal_info_testfinished, NULL };

static const GDBusInterfaceInfo si_iface_info =
{
  -1,
  (gchar*) UNITY_DBUS_INTROSPECTION_IFACE_NAME,
  (GDBusMethodInfo **) &si_method_info_pointers,
  NULL,
  NULL,
  NULL
};

static const GDBusInterfaceInfo ap_iface_info =
{
  -1,
  (gchar *) UNITY_DBUS_AP_IFACE_NAME,
  (GDBusMethodInfo **) &ap_method_info_pointers,
  (GDBusSignalInfo **) &ap_signal_info_pointers,
  NULL,
  NULL
};

static unity::Introspectable *_introspectable;
static Autopilot *_autopilot;

DebugDBusInterface::DebugDBusInterface (unity::Introspectable *introspectable)
{
  _introspectable = introspectable;
  _owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                              UNITY_DBUS_BUS_NAME,
                              G_BUS_NAME_OWNER_FLAGS_NONE,
                              &DebugDBusInterface::OnBusAcquired,
                              &DebugDBusInterface::OnNameAcquired,
                              &DebugDBusInterface::OnNameLost,
                              this,
                              NULL);
}

DebugDBusInterface::~DebugDBusInterface ()
{
  g_bus_unown_name (_owner_id);
}

static const GDBusInterfaceInfo *const debug_object_interfaces [] = { &si_iface_info, &ap_iface_info, NULL };

void 
DebugDBusInterface::OnBusAcquired (GDBusConnection *connection, const gchar *name, gpointer data)
{
  int i = 0;
  GError *error;

  UnityScreen *uscreen = dynamic_cast<UnityScreen*> (_introspectable);
  if (uscreen != NULL) 
  {
    _autopilot = new Autopilot (uscreen->screen, connection);
  }

  while (debug_object_interfaces[i] != NULL)
  {
    error = NULL;
    g_dbus_connection_register_object (connection,
                                       UNITY_DBUS_DEBUG_OBJECT_PATH,
                                       (GDBusInterfaceInfo* ) debug_object_interfaces[i],
                                       &si_vtable,
                                       NULL,
                                       NULL,
                                       &error);
    if (error != NULL)
    {
      g_warning ("Could not register debug interface onto d-bus");
      g_error_free (error);
    }
    i++;
  }
}

void
DebugDBusInterface::OnNameAcquired (GDBusConnection *connection, const gchar *name, gpointer data)
{
}

void
DebugDBusInterface::OnNameLost (GDBusConnection *connection, const gchar *name, gpointer data)
{
}

void
DBusMethodCall (GDBusConnection *connection,
                const gchar *sender,
                const gchar *objectPath,
                const gchar *ifaceName,
                const gchar *methodName,
                GVariant *parameters,
                GDBusMethodInvocation *invocation,
                gpointer data)
{
  if (g_strcmp0 (methodName, SI_METHOD_NAME_GETSTATE) == 0)
  {
    GVariant *ret;
    const gchar *input;
    g_variant_get (parameters, "(&s)", &input);

    ret = GetState (input);
    g_dbus_method_invocation_return_value (invocation, ret);
    g_variant_unref (ret);
  }
  else if (g_strcmp0 (methodName, AP_METHOD_NAME_STARTTEST) == 0)
  {
    const gchar *name;
    g_variant_get (parameters, "(&s)", &name);

    StartTest (name);
    g_dbus_method_invocation_return_value (invocation, NULL);
  }
  else
  {
    g_dbus_method_invocation_return_dbus_error (invocation, UNITY_DBUS_BUS_NAME,
                                                "Failed to find method");
  }
}

GVariant*
GetState (const gchar *piece)
{
  return _introspectable->Introspect ();
}

void
StartTest (const gchar *name)
{
  _autopilot->StartTest (name);
}

/* a very contrived example purely for giving QA something purposes */
GVariant*
DebugDBusInterface::BuildFakeReturn ()
{
  GVariantBuilder *builder;
  GVariant *result, *panel_result, *indicators_result, *appmenu_result, *entries_result, *zero_result, *one_result;

  builder = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}") );
  g_variant_builder_add (builder, "{sv}", "label", g_variant_new_string ("_File") );
  g_variant_builder_add (builder, "{sv}", "image", g_variant_new_string ("") );
  g_variant_builder_add (builder, "{sv}", "visible", g_variant_new_boolean (TRUE) );
  g_variant_builder_add (builder, "{sv}", "sensitive", g_variant_new_boolean (TRUE) );
  g_variant_builder_add (builder, "{sv}", "active", g_variant_new_boolean (FALSE) );
  g_variant_builder_add (builder, "{sv}", "label", g_variant_new_int32 (34) );
  g_variant_builder_add (builder, "{sv}", "label", g_variant_new_int32 (24) );
  zero_result = g_variant_new ("(a{sv})", builder);
  g_variant_builder_unref (builder);

  builder = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}") );
  g_variant_builder_add (builder, "{sv}", "label", g_variant_new_string ("_Edit") );
  g_variant_builder_add (builder, "{sv}", "image", g_variant_new_string ("") );
  g_variant_builder_add (builder, "{sv}", "visible", g_variant_new_boolean (TRUE) );
  g_variant_builder_add (builder, "{sv}", "sensitive", g_variant_new_boolean (TRUE) );
  g_variant_builder_add (builder, "{sv}", "active", g_variant_new_boolean (FALSE) );
  g_variant_builder_add (builder, "{sv}", "label", g_variant_new_int32 (34) );
  g_variant_builder_add (builder, "{sv}", "label", g_variant_new_int32 (24) );
  one_result = g_variant_new ("(a{sv})", builder);
  g_variant_builder_unref (builder);

  builder = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}") );
  g_variant_builder_add (builder, "{sv}", "0", zero_result);
  g_variant_builder_add (builder, "{sv}", "1", one_result);
  entries_result = g_variant_new ("(a{sv})", builder);
  g_variant_builder_unref (builder);

  builder = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}") );
  g_variant_builder_add (builder, "{sv}", "model-name",
                         g_variant_new_string ("com.canonical.Unity.Panel.Service.Indicators.appmenu.324234243") );
  g_variant_builder_add (builder, "{sv}", "entries", entries_result);
  appmenu_result = g_variant_new ("(a{sv})", builder);
  g_variant_builder_unref (builder);

  builder = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}") );
  g_variant_builder_add (builder, "{sv}", "appmenu", appmenu_result);
  indicators_result = g_variant_new ("(a{sv})", builder);
  g_variant_builder_unref (builder);

  builder = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}") );
  g_variant_builder_add (builder, "{sv}", "backend",
                         g_variant_new_string ("/com/canonical/Unity/Panel/Service/324234243") );
  g_variant_builder_add (builder, "{sv}", "launch-type",
                         g_variant_new_string ("dbus") );
  g_variant_builder_add (builder, "{sv}", "width", g_variant_new_int32 (1024) );
  g_variant_builder_add (builder, "{sv}", "width", g_variant_new_int32 (32) );
  g_variant_builder_add (builder, "{sv}", "theme", g_variant_new_string ("gtk") );
  g_variant_builder_add (builder, "{sv}", "indicators", indicators_result);
  panel_result = g_variant_new ("(a{sv})", builder);
  g_variant_builder_unref (builder);

  builder = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}") );
  g_variant_builder_add (builder, "{sv}", "panel", panel_result);
  result = g_variant_new ("(a{sv})", builder);
  g_variant_builder_unref (builder);

  gchar *s = g_variant_print (result, TRUE);
  g_free (s);
  return result;
}
