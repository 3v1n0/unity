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
	GVariant		*result;
	GVariant        *childResults;
	GVariantBuilder *builder;

	builder = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}"));
	for (std::list<Introspectable*>::iterator it = _children.begin (); it != _children.end (); it++) {
		g_variant_builder_add (builder, "{sv}", (*it)->getName (), (*it)->introspect ());
	}	
	addProperties (builder);

	childResults = g_variant_new ("(a{sv})", builder);
	g_variant_builder_unref (builder);

	if (_children.size () > 0) {
		builder = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}"));
		g_variant_builder_add (builder, "{sv}", getName (), childResults);
		result = g_variant_new ("(a{sv})", builder);
		g_variant_builder_unref (builder);

		return result;
	}
	
	return childResults;
}

void
Introspectable::addChild (Introspectable *child)
{
	_children.push_back (child);
}

void 
Introspectable::removeChild (Introspectable *child)
{
	_children.remove (child);
}