/* Introspectable.cpp
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

#include "Introspectable.h"

GVariant*
Introspectable::introspect ()
{
	GVariant        *result;
	GVariantBuilder *builder;

	builder = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}"));
	for (std::list<Introspectable*>::iterator it = children.begin (); it != children.end (); it++) {
		g_variant_builder_add (builder, "{sv}", (*it)->getName (), (*it)->introspect ());
	}	
	addProperties (builder);
	
	result = g_variant_new ("(a{sv})", builder);
	g_variant_builder_unref (builder);
	return result;
}