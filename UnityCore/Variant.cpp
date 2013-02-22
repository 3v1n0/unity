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
 * Authored by: Tim Penhey <tim.penhey@canonical.com>
 */

#include "Variant.h"

namespace unity
{
namespace glib
{

Variant::Variant()
  : variant_(NULL)
{}

Variant::Variant(GVariant* variant)
  : variant_(variant)
{
  if (variant) g_variant_ref_sink(variant_);
}

Variant::Variant(GVariant* variant, StealRef const& ref)
  : variant_(variant)
{}

Variant::Variant(Variant const& other)
  : variant_(other.variant_)
{
  if (variant_) g_variant_ref_sink(variant_);
}

Variant::~Variant()
{
  if (variant_) g_variant_unref(variant_);
}

std::string Variant::GetString() const
{
  // g_variant_get_string doesn't duplicate the string
  const gchar *result = g_variant_get_string (variant_, NULL);
  return result != NULL ? result : "";
}

int32_t Variant::GetInt32() const
{
  return static_cast<int32_t>(g_variant_get_int32 (variant_));
}

uint32_t Variant::GetUInt32() const
{
  return static_cast<uint32_t>(g_variant_get_uint32 (variant_));
}

int64_t Variant::GetInt64() const
{
  return static_cast<int64_t>(g_variant_get_int64 (variant_));
}

uint64_t Variant::GetUInt64() const
{
  return static_cast<uint64_t>(g_variant_get_uint64 (variant_));
}

bool Variant::GetBool() const
{
  return (g_variant_get_boolean (variant_) != FALSE);
}

double Variant::GetDouble() const
{
  return g_variant_get_double (variant_);
}

float Variant::GetFloat() const
{
  return static_cast<float>(g_variant_get_double (variant_));
}

bool Variant::ASVToHints(HintsMap& hints) const
{
  GVariantIter* hints_iter;
  char* key = NULL;
  GVariant* value = NULL;

  if (!variant_)
    return false;

  if (!g_variant_is_of_type (variant_, G_VARIANT_TYPE ("(a{sv})")) &&
      !g_variant_is_of_type (variant_, G_VARIANT_TYPE ("a{sv}")))
  {
    return false;
  }

  g_variant_get(variant_, g_variant_get_type_string(variant_), &hints_iter);

  while (g_variant_iter_loop(hints_iter, "{sv}", &key, &value))
  {
    hints[key] = value;
  }

  g_variant_iter_free (hints_iter);

  return true;
}

Variant Variant::FromHints(HintsMap const& hints)
{
  GVariantBuilder b;
  g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));

  for (glib::HintsMap::const_iterator it = hints.begin(); it != hints.end(); ++it)
  {
    const gchar* key = it->first.c_str();
    GVariant* ptr = it->second;

    g_variant_builder_add(&b, "{sv}", key, ptr);
  }

  return g_variant_builder_end(&b);
}

void Variant::swap(Variant& other)
{
  std::swap(this->variant_, other.variant_);
}

Variant& Variant::operator=(GVariant* val)
{
  if (variant_) g_variant_unref(variant_);
  variant_ = val ? g_variant_ref_sink(val) : val;

  return *this;
}

Variant& Variant::operator=(Variant other)
{
  swap(other);

  return *this;
}

Variant::operator GVariant* () const
{
  return variant_;
}

Variant::operator bool() const
{
  return bool(variant_);
}

static void g_variant_unref0 (gpointer var)
{
  if (var)
    g_variant_unref((GVariant*)var);
}

GHashTable* hashtable_from_hintsmap(glib::HintsMap const& hints)
{
  GHashTable* hash_table = g_hash_table_new_full(g_str_hash, g_direct_equal, g_free, g_variant_unref0);

  if (!hash_table)
    return nullptr;

  for (glib::HintsMap::const_iterator it = hints.begin(); it != hints.end(); ++it)
  {
    g_hash_table_insert(hash_table, g_strdup(it->first.c_str()), g_variant_ref(it->second));
  }
  return hash_table;
}

HintsMap const& hintsmap_from_hashtable(GHashTable* hashtable, HintsMap& hints)
{
  if (!hashtable)
    return hints;

  GHashTableIter hints_iter;
  gpointer key, value;
  g_hash_table_iter_init (&hints_iter, hashtable);
  while (g_hash_table_iter_next (&hints_iter, &key, &value))
  {
    std::string hint_key(static_cast<gchar*>(key));
    glib::Variant hint_value(static_cast<GVariant*>(value));

    hints[hint_key] = hint_value;
  }
  return hints;
}

} // namespace glib

namespace variant
{

BuilderWrapper::BuilderWrapper(GVariantBuilder* builder)
  : builder_(builder)
{}

BuilderWrapper& BuilderWrapper::add(char const* name, bool value)
{
  g_variant_builder_add(builder_, "{sv}", name, g_variant_new_boolean(value));
  return *this;
}

BuilderWrapper& BuilderWrapper::add(char const* name, char const* value)
{
  if (value)
    g_variant_builder_add(builder_, "{sv}", name, g_variant_new_string(value));
  else
    g_variant_builder_add(builder_, "{sv}", name, g_variant_new_string(""));

  return *this;
}

BuilderWrapper& BuilderWrapper::add(char const* name, std::string const& value)
{
  g_variant_builder_add(builder_, "{sv}", name,
                        g_variant_new_string(value.c_str()));
  return *this;
}

BuilderWrapper& BuilderWrapper::add(char const* name, int32_t value)
{
  g_variant_builder_add(builder_, "{sv}", name, g_variant_new_int32(value));
  return *this;
}

BuilderWrapper& BuilderWrapper::add(char const* name, uint32_t value)
{
  g_variant_builder_add(builder_, "{sv}", name, g_variant_new_uint32(value));
  return *this;
}

BuilderWrapper& BuilderWrapper::add(char const* name, int64_t value)
{
  g_variant_builder_add(builder_, "{sv}", name, g_variant_new_int64(value));
  return *this;
}

BuilderWrapper& BuilderWrapper::add(char const* name, uint64_t value)
{
  g_variant_builder_add(builder_, "{sv}", name, g_variant_new_uint64(value));
  return *this;
}

BuilderWrapper& BuilderWrapper::add(char const* name, float value)
{
  g_variant_builder_add(builder_, "{sv}", name, g_variant_new_double(value));
  return *this;
}

BuilderWrapper& BuilderWrapper::add(char const* name, double value)
{
  g_variant_builder_add(builder_, "{sv}", name, g_variant_new_double(value));
  return *this;
}

BuilderWrapper& BuilderWrapper::add(char const* name, GVariant* value)
{
  g_variant_builder_add(builder_, "{sv}", name, value);
  return *this;
}

BuilderWrapper& BuilderWrapper::add(nux::Rect const& value)
{
  add("x", value.x);
  add("y", value.y);
  add("width", value.width);
  add("height", value.height);
  return *this;
}

}
}
