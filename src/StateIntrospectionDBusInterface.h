/* StateIntrospectionDBusInterface.h
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
 *
 * Provides a convenient way to poke into Unity and a snapshot text
 * representation of what Unity's models look like for debugging.
 */
#ifndef _STATE_INTROSPECTION_DBUS_INTERFACE_H
#define _STATE_INTROSPECTION_DBUS_INTERFACE_H 1

#include <glib.h>
#include <gio/gio.h>

class StateIntrospectionDBusInterface
{
public:
	StateIntrospectionDBusInterface ();
	~StateIntrospectionDBusInterface ();
	
	void
	initStateIntrospection ();
	
	static void
	dBusMethodCall (GDBusConnection *connection, const gchar *sender,
	                const gchar *objectPath, const gchar *ifaceName,
	                const gchar *methodName, GVariant *parameters,
	                GDBusMethodInvocation *invocation, gpointer data);
	
private:
	/* methods */
	static void
	onBusAcquired (GDBusConnection *connection, const gchar *name, gpointer data);

	static void
	onNameAcquired (GDBusConnection *connection, const gchar *name, gpointer data);

	static void
    onNameLost (GDBusConnection *connection, const gchar *name, gpointer data);

	

	/* members */
	guint _owner_id;
};

#endif
