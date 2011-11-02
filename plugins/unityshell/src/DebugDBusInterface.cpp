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

#define SI_METHOD_NAME_GETSTATE  "GetState"
#define AP_METHOD_NAME_STARTTEST "StartTest"

void StartTest(const gchar*);
GVariant* GetState(const gchar*);
void DBusMethodCall(GDBusConnection*, const gchar*, const gchar*,
                    const gchar*, const gchar*, GVariant*,
                    GDBusMethodInvocation*, gpointer);
Introspectable* FindPieceToIntrospect(std::queue<Introspectable*> queue, 
                                      const gchar* pieceName);

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

static const GDBusArgInfo* const si_getstate_in_arg_pointers[] = { &si_getstate_in_args, NULL };

static const GDBusArgInfo si_getstate_out_args =
{
  -1,
  (gchar*) "state",
  (gchar*) "a{sv}",
  NULL
};
static const GDBusArgInfo* const si_getstate_out_arg_pointers[] = { &si_getstate_out_args, NULL };

static const GDBusMethodInfo si_method_info_getstate =
{
  -1,
  (gchar*) SI_METHOD_NAME_GETSTATE,
  (GDBusArgInfo**)& si_getstate_in_arg_pointers,
  (GDBusArgInfo**)& si_getstate_out_arg_pointers,
  NULL
};

static const GDBusArgInfo ap_starttest_in_args =
{
  -1,
  (gchar*) "name",
  (gchar*) "s",
  NULL
};
static const GDBusArgInfo* const ap_starttest_in_arg_pointers[] = { &ap_starttest_in_args, NULL };

static GDBusMethodInfo ap_method_info_starttest =
{
  -1,
  (gchar*) AP_METHOD_NAME_STARTTEST,
  (GDBusArgInfo**)& ap_starttest_in_arg_pointers,
  NULL,
  NULL
};

static const GDBusMethodInfo* const si_method_info_pointers [] = { &si_method_info_getstate, NULL };
static const GDBusMethodInfo* const ap_method_info_pointers [] = { &ap_method_info_starttest, NULL };

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

static GDBusArgInfo ap_testfinished_arg_data =
{
  -1,
  (gchar*) "data",
  (gchar*) "a{sv}",
  NULL
};

static const GDBusArgInfo* const ap_signal_testfinished_arg_pointers [] = { &ap_testfinished_arg_name,
                                                                            &ap_testfinished_arg_passed,
                                                                            &ap_testfinished_arg_data,
                                                                            NULL
                                                                          };
static GDBusSignalInfo ap_signal_info_testfinished =
{
  -1,
  (gchar*) UNITY_DBUS_AP_SIG_TESTFINISHED,
  (GDBusArgInfo**)& ap_signal_testfinished_arg_pointers,
  NULL
};

static const GDBusSignalInfo* const ap_signal_info_pointers [] = { &ap_signal_info_testfinished, NULL };

static const GDBusInterfaceInfo si_iface_info =
{
  -1,
  (gchar*) UNITY_DBUS_INTROSPECTION_IFACE_NAME,
  (GDBusMethodInfo**)& si_method_info_pointers,
  NULL,
  NULL,
  NULL
};

static const GDBusInterfaceInfo ap_iface_info =
{
  -1,
  (gchar*) UNITY_DBUS_AP_IFACE_NAME,
  (GDBusMethodInfo**)& ap_method_info_pointers,
  (GDBusSignalInfo**)& ap_signal_info_pointers,
  NULL,
  NULL
};

static CompScreen* _screen;
static unity::Autopilot* _autopilot;
static unity::Introspectable* _parent_introspectable;

DebugDBusInterface::DebugDBusInterface(unity::Introspectable* parent, 
                                       CompScreen* screen)
{
  _screen = screen;
  _parent_introspectable = parent;
  _owner_id = g_bus_own_name(G_BUS_TYPE_SESSION,
                             UNITY_DBUS_BUS_NAME,
                             G_BUS_NAME_OWNER_FLAGS_NONE,
                             &DebugDBusInterface::OnBusAcquired,
                             &DebugDBusInterface::OnNameAcquired,
                             &DebugDBusInterface::OnNameLost,
                             NULL,
                             NULL);
}

DebugDBusInterface::~DebugDBusInterface()
{
  g_bus_unown_name(_owner_id);
}

static const GDBusInterfaceInfo* const debug_object_interfaces [] = { &si_iface_info, &ap_iface_info, NULL };

void
DebugDBusInterface::OnBusAcquired(GDBusConnection* connection, const gchar* name, gpointer data)
{
  int i = 0;
  GError* error;

  _autopilot = new unity::Autopilot(_screen, connection);
  
  while (debug_object_interfaces[i] != NULL)
  {
    error = NULL;
    g_dbus_connection_register_object(connection,
                                      UNITY_DBUS_DEBUG_OBJECT_PATH,
                                      (GDBusInterfaceInfo*) debug_object_interfaces[i],
                                      &si_vtable,
                                      NULL,
                                      NULL,
                                      &error);
    if (error != NULL)
    {
      g_warning("Could not register debug interface onto d-bus");
      g_error_free(error);
    }
    i++;
  }
}

void
DebugDBusInterface::OnNameAcquired(GDBusConnection* connection, const gchar* name, gpointer data)
{
}

void
DebugDBusInterface::OnNameLost(GDBusConnection* connection, const gchar* name, gpointer data)
{
}

void
DBusMethodCall(GDBusConnection* connection,
               const gchar* sender,
               const gchar* objectPath,
               const gchar* ifaceName,
               const gchar* methodName,
               GVariant* parameters,
               GDBusMethodInvocation* invocation,
               gpointer data)
{
  if (g_strcmp0(methodName, SI_METHOD_NAME_GETSTATE) == 0)
  {
    GVariant* ret;
    const gchar* input;
    g_variant_get(parameters, "(&s)", &input);

    ret = GetState(input);
    g_dbus_method_invocation_return_value(invocation, ret);
    g_variant_unref(ret);
  }
  else if (g_strcmp0(methodName, AP_METHOD_NAME_STARTTEST) == 0)
  {
    const gchar* name;
    g_variant_get(parameters, "(&s)", &name);

    StartTest(name);
    g_dbus_method_invocation_return_value(invocation, NULL);
  }
  else
  {
    g_dbus_method_invocation_return_dbus_error(invocation, UNITY_DBUS_BUS_NAME,
                                               "Failed to find method");
  }
}

GVariant*
GetState(const gchar* pieceName)
{
  std::queue<Introspectable*> queue;
  queue.push(_parent_introspectable);

  // Since the empty string won't really match the name of the parent (Unity),
  // we make sure that we're able to accept a blank string and just define it to
  // mean the top level.
  Introspectable* piece = g_strcmp0(pieceName, "") == 0
    ? _parent_introspectable
    : FindPieceToIntrospect(queue, pieceName);

  // FIXME this might not work, make sure it does.
  if (piece == NULL)
    return NULL;

  return piece->Introspect();
}

void
StartTest(const gchar* name)
{
  _autopilot->StartTest(name);
}

/*
 * Do a breadth-first search of the introspectable tree.
 */
Introspectable*
FindPieceToIntrospect(std::queue<Introspectable*> queue, const gchar* pieceName)
{
  Introspectable* piece;

  while (!queue.empty())
  {
    piece = queue.front();
    queue.pop();

    if (g_strcmp0 (piece->GetName(), pieceName) == 0)
    {
      return piece;
    }

    for (auto it = piece->GetIntrospectableChildren().begin(), last = piece->GetIntrospectableChildren().end(); it != last; it++)
    {
      queue.push(*it);
    }

    FindPieceToIntrospect(queue, pieceName);
  }

  return NULL;
}
