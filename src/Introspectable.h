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