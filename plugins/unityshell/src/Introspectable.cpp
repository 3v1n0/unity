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

#include "Introspectable.h"

namespace unity
{

Introspectable::Introspectable()
{
}

Introspectable::~Introspectable()
{
  for (auto parent : _parents)
    parent->_children.remove(this);
}

GVariant*
Introspectable::Introspect()
{
  GVariantBuilder* builder;
  GVariant*        result;
  GVariantBuilder* child_builder;
  gint             n_children = 0;

  builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
  AddProperties(builder);

  child_builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));

  for (auto it = _children.begin(); it != _children.end(); it++)
  {
    if ((*it)->GetName())
    {
      g_variant_builder_add(child_builder, "{sv}", (*it)->GetName(), (*it)->Introspect());
      n_children++;
    }
  }

  if (n_children > 0)
  {
    GVariant*        child_results;

    child_results = g_variant_new("(a{sv})", child_builder);
    g_variant_builder_add(builder, "{sv}", GetChildsName(), child_results);
  }
  g_variant_builder_unref(child_builder);

  result = g_variant_new("(a{sv})", builder);
  g_variant_builder_unref(builder);

  return result;
}

void
Introspectable::AddChild(Introspectable* child)
{
  _children.push_back(child);
  child->_parents.push_back(this);
}

void
Introspectable::RemoveChild(Introspectable* child)
{
  _children.remove(child);
  child->_parents.remove(this);
}

const gchar*
Introspectable::GetChildsName()
{
  return GetName();
}
}
