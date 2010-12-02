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

#include "Introspectable.h"

GVariant*
Introspectable::Introspect ()
{
  GVariant        *childResults;
  GVariantBuilder *builder;

  builder = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}"));
  
  AddProperties (builder);

   for (std::list<Introspectable *>::iterator it = _children.begin (); it != _children.end (); it++)
  {
    if ((*it)->GetName ())
      g_variant_builder_add (builder, "{sv}", (*it)->GetName (), (*it)->Introspect () );
  }

  childResults = g_variant_new ("(a{sv})", builder);
  g_variant_builder_unref (builder);

  return childResults;
}

void
Introspectable::AddChild (Introspectable *child)
{
  _children.push_back (child);
}

void
Introspectable::RemoveChild (Introspectable *child)
{
  _children.remove (child);
}
