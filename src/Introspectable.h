/* Introspectable.h
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

#ifndef _INTROSPECTABLE_H 
#define _INTROSPECTABLE_H 1

#include <glib.h>
#include <list>

class Introspectable
{
public:
	GVariant* introspect ();
	void addChild (Introspectable *child);
	void removeChild (Introspectable *child);

protected:
	virtual const gchar* getName () = 0;
	virtual void addProperties (GVariantBuilder *builder) = 0;

private:
	std::list<Introspectable*> _children;
};

#endif