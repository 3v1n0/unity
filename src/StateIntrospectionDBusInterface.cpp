/* StateIntrospectionDBusInterface.cpp
 *
 * Copyright (c) 2010 Alex Launi <alex.launi@canonical.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <iostream>
#include "StateIntrospectionDBusInterface.h"


#define UNITY_STATE_DEBUG_BUS_NAME "com.canonical.Unity.Debug"

static const GDBusInterfaceVTable si_vtable =
{
	&StateIntrospectionDBusInterface::dBusMethodCall,
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
static const GDBusArgInfo * const si_getstate_in_arg_pointers[] = { &si_getstate_in_args, NULL };

// TODO: this is really a a{sv} or something like that.
static const GDBusArgInfo si_getstate_out_args =
{
	-1,
	"state",
	"s",
	NULL
};
static const GDBusArgInfo * const si_getstate_out_arg_pointers[] = { &si_getstate_out_args, NULL };

static const GDBusMethodInfo si_method_info_getstate =
{
	-1,
	"GetState",
	(GDBusArgInfo **) &si_getstate_in_arg_pointers,
	(GDBusArgInfo **) &si_getstate_out_arg_pointers,
	NULL
};

static const GDBusMethodInfo * const si_method_info_pointers[] = { &si_method_info_getstate, NULL };

static const GDBusInterfaceInfo si_iface_info =
{
	-1,
	"org.canonical.Unity.Debug.StateIntrospection",
	(GDBusMethodInfo **) &si_method_info_pointers,
	NULL,
	NULL,
	NULL,
};

StateIntrospectionDBusInterface::StateIntrospectionDBusInterface ()
{
}

StateIntrospectionDBusInterface::~StateIntrospectionDBusInterface ()
{
	g_bus_unown_name (_owner_id);
}

void
StateIntrospectionDBusInterface::initStateIntrospection ()
{
	std::cout << "tryna own da bus jawn" << std::endl;
    _owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                               UNITY_STATE_DEBUG_BUS_NAME,
                               G_BUS_NAME_OWNER_FLAGS_NONE,
                               &StateIntrospectionDBusInterface::onBusAcquired,
                               &StateIntrospectionDBusInterface::onNameAcquired,
                               &StateIntrospectionDBusInterface::onNameLost,
                               this,
                               NULL);
                               
}

void
StateIntrospectionDBusInterface::onBusAcquired (GDBusConnection *connection, const gchar *name, gpointer data)
{
	GError *error;
	std::cout << "bus acquired" << std::endl;
	g_dbus_connection_register_object (connection,
	                                   "/org/canonical/Unity/Debug/StateIntrospection",
	                                   (GDBusInterfaceInfo*) &si_iface_info,
	                                   &si_vtable,
	                                   NULL,
	                                   NULL,
	                                   &error);
}

void
StateIntrospectionDBusInterface::onNameAcquired (GDBusConnection *connection, const gchar *name, gpointer data)
{
	std::cout << "name aquired" << std::endl;
}

void
StateIntrospectionDBusInterface::onNameLost (GDBusConnection *connection, const gchar *name, gpointer data)
{
}

void
StateIntrospectionDBusInterface::dBusMethodCall (GDBusConnection *connection, 
                                                 const gchar *sender,
                                                 const gchar *objectPath,
                                                 const gchar *ifaceName,
                                                 const gchar *methodName,
                                                 GVariant *parameters,
                                                 GDBusMethodInvocation *invocation, 
                                                 gpointer data)
{
}