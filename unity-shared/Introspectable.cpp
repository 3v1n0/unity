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
namespace debug
{

Introspectable::Introspectable()
{
  static guint64 unique_id=0;
  _id = unique_id++;
}

Introspectable::~Introspectable()
{
  for (auto parent : _parents)
    parent->_children.remove(this);
  for (auto child : _children)
    child->_parents.remove(this);
}

Introspectable::IntrospectableList Introspectable::GetIntrospectableChildren()
{
  return _children;
}

GVariant*
Introspectable::Introspect()
{
  GVariantBuilder builder;
  GVariantBuilder child_builder;
  bool has_valid_children = false;

  g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&builder, "{sv}", "id", g_variant_new_uint64(_id));

  AddProperties(&builder);

  g_variant_builder_init(&child_builder, G_VARIANT_TYPE("as"));

  for (auto const& child : GetIntrospectableChildren())
  {
    auto const& child_name = child->GetName();

    if (!child_name.empty())
    {
      g_variant_builder_add(&child_builder, "s", child_name.c_str());
      has_valid_children = true;
    }
  }

  GVariant* child_results = g_variant_builder_end(&child_builder);

  if (has_valid_children)
    g_variant_builder_add(&builder, "{sv}", GetChildsName().c_str(), child_results);

  return g_variant_builder_end(&builder);
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

std::string
Introspectable::GetChildsName() const
{
  return "Children";
}

guint64 Introspectable::GetIntrospectionId() const
{
  return _id;
}

void Introspectable::RemoveAllChildren()
{
  for (auto child : _children)
  {
    child->_parents.remove(this);
  }
  _children.clear();
}

}
}

