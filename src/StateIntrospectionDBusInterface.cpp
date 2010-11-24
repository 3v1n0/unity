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

#include <iostream>
#include "StateIntrospectionDBusInterface.h"

#define UNITY_STATE_DEBUG_BUS_NAME "com.canonical.Unity.Debug"

static const GDBusInterfaceVTable si_vtable =
{
  &StateIntrospectionDBusInterface::DBusMethodCall,
  NULL,
  NULL
};

static const GDBusArgInfo si_getstate_in_args =
{
  -1,
  "piece",
  "s",
  NULL
};
static const GDBusArgInfo *const si_getstate_in_arg_pointers[] = { &si_getstate_in_args, NULL };

// TODO: this is really a a{sv} or something like that.
static const GDBusArgInfo si_getstate_out_args =
{
  -1,
  "state",
  "a{sv}",
  NULL
};
static const GDBusArgInfo *const si_getstate_out_arg_pointers[] = { &si_getstate_out_args, NULL };

static const GDBusMethodInfo si_method_info_getstate =
{
  -1,
  "GetState",
  (GDBusArgInfo **) &si_getstate_in_arg_pointers,
  (GDBusArgInfo **) &si_getstate_out_arg_pointers,
  NULL
};

static const GDBusMethodInfo *const si_method_info_pointers[] = { &si_method_info_getstate, NULL };

static const GDBusInterfaceInfo si_iface_info =
{
  -1,
  "com.canonical.Unity.Debug.StateIntrospection",
  (GDBusMethodInfo **) &si_method_info_pointers,
  NULL,
  NULL,
  NULL,
};

static Introspectable      *_introspectable;

StateIntrospectionDBusInterface::StateIntrospectionDBusInterface (Introspectable *introspectable)
{
  _introspectable = introspectable;
  _owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                              UNITY_STATE_DEBUG_BUS_NAME,
                              G_BUS_NAME_OWNER_FLAGS_NONE,
                              &StateIntrospectionDBusInterface::OnBusAcquired,
                              &StateIntrospectionDBusInterface::OnNameAcquired,
                              &StateIntrospectionDBusInterface::OnNameLost,
                              this,
                              NULL);
}

StateIntrospectionDBusInterface::~StateIntrospectionDBusInterface ()
{
  g_bus_unown_name (_owner_id);
}

void
StateIntrospectionDBusInterface::OnBusAcquired (GDBusConnection *connection, const gchar *name, gpointer data)
{
  GError *error = NULL;
  g_dbus_connection_register_object (connection,
                                     "/com/canonical/Unity/Debug/StateIntrospection",
                                     (GDBusInterfaceInfo *) &si_iface_info,
                                     &si_vtable,
                                     NULL,
                                     NULL,
                                     &error);
  if (error != NULL)
  {
    g_warning ("Could not register StateIntrospection object onto d-bus");
  }
}

void
StateIntrospectionDBusInterface::OnNameAcquired (GDBusConnection *connection, const gchar *name, gpointer data)
{
}

void
StateIntrospectionDBusInterface::OnNameLost (GDBusConnection *connection, const gchar *name, gpointer data)
{
}

void
StateIntrospectionDBusInterface::DBusMethodCall (GDBusConnection *connection,
                                                 const gchar *sender,
                                                 const gchar *objectPath,
                                                 const gchar *ifaceName,
                                                 const gchar *methodName,
                                                 GVariant *parameters,
                                                 GDBusMethodInvocation *invocation,
                                                 gpointer data)
{
  if (g_strcmp0 (methodName, "GetState") == 0)
  {
    GVariant *ret;
    const gchar *input;
    g_variant_get (parameters, "(&s)", &input);

    ret = GetState (input);
    g_dbus_method_invocation_return_value (invocation, ret);
    g_variant_unref (ret);
  }
  else
  {
    g_dbus_method_invocation_return_dbus_error (invocation, "com.canonical.Unity",
                                                "EAT IT, BITCH");
  }
}

GVariant*
StateIntrospectionDBusInterface::GetState (const gchar *piece)
{
  return _introspectable->introspect ();
}

/* a very contrived example purely for giving QA something purposes */
GVariant*
StateIntrospectionDBusInterface::BuildFakeReturn ()
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