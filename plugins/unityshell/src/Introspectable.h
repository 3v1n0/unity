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

#ifndef _INTROSPECTABLE_H
#define _INTROSPECTABLE_H 1

#include <glib.h>
#include <list>

namespace unity
{
class Introspectable
{
typedef std::list<Introspectable*> IntrospectableList;

public:
  Introspectable();
  virtual ~Introspectable();

  GVariant* Introspect();
  virtual const gchar* GetName() = 0;
  void AddChild(Introspectable* child);
  void RemoveChild(Introspectable* child);
  IntrospectableList const& GetIntrospectableChildren() { return _children; };

protected:
  virtual const gchar* GetChildsName();
  virtual void AddProperties(GVariantBuilder* builder) = 0;

  /*
   * AddProperties should be implemented as such ...
   * void ClassFoo::AddProperties (GVariantBuilder *builder)
   * {
   *  g_variant_builder_add (builder, "{sv}", "label", g_variant_new_string ("_File") );
   *  g_variant_builder_add (builder, "{sv}", "image", g_variant_new_string ("") );
   *  g_variant_builder_add (builder, "{sv}", "visible", g_variant_new_boolean (TRUE) );
   *    g_variant_builder_add (builder, "{sv}", "sensitive", g_variant_new_boolean (TRUE) );
   *    g_variant_builder_add (builder, "{sv}", "active", g_variant_new_boolean (FALSE) );
   *    g_variant_builder_add (builder, "{sv}", "label", g_variant_new_int32 (34) );
   *    g_variant_builder_add (builder, "{sv}", "label", g_variant_new_int32 (24) );
   * }
   * That's all. Just add a bunch of key-value properties to the builder.
   */

private:
  std::list<Introspectable*> _children;
  std::list<Introspectable*> _parents;
};
}
#endif
