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

#include <UnityCore/Variant.h>
#include "Introspectable.h"

namespace unity
{
namespace debug
{

const std::string CHILDREN_NAME = "Children";

Introspectable::Introspectable()
{
  static int32_t unique_id_ = 0;
  id_ = unique_id_++;
}

Introspectable::~Introspectable()
{
  for (auto parent : parents_)
    parent->children_.remove(this);
  for (auto child : children_)
    child->parents_.remove(this);
}

Introspectable::IntrospectableList Introspectable::GetIntrospectableChildren()
{
  return children_;
}

GVariant*
Introspectable::Introspect()
{
  GVariantBuilder builder;
  GVariantBuilder child_builder;
  bool has_valid_children = false;

  g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));
  variant::BuilderWrapper build_wrapper(&builder);
  build_wrapper.add("id", id_);
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

  glib::Variant child_results(g_variant_builder_end(&child_builder));

  if (has_valid_children)
    build_wrapper.add(CHILDREN_NAME, child_results);

  return g_variant_builder_end(&builder);
}

void
Introspectable::AddChild(Introspectable* child)
{
  children_.push_back(child);
  child->parents_.push_back(this);
}

void
Introspectable::RemoveChild(Introspectable* child)
{
  children_.remove(child);
  child->parents_.remove(this);
}

int32_t Introspectable::GetIntrospectionId() const
{
  return id_;
}

void Introspectable::RemoveAllChildren()
{
  for (auto child : children_)
    child->parents_.remove(this);

  children_.clear();
}

}
}

