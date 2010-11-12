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
#include <gio/gio.h>
#include "StateIntrospectionDBusInterface.h"


#define UNITY_STATE_DEBUG_BUS_NAME "com.canonical.Unity.Debug"

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
	std::cout << "bus acquired" << std::endl;
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