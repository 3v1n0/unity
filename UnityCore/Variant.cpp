// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2013 Canonical Ltd
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
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "Variant.h"
#include <NuxCore/Logger.h>
#include "GLibWrapper.h"

namespace unity
{
namespace glib
{
DECLARE_LOGGER(logger, "unity.glib.variant");

Variant::Variant()
  : variant_(nullptr)
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
  : Variant(other.variant_)
{}

Variant::Variant(std::nullptr_t)
  : Variant()
{}

Variant::Variant(const char* value)
  : Variant(g_variant_new_string(value ? value : value))
{}

Variant::Variant(std::string const& value)
  : Variant(g_variant_new_string(value.c_str()))
{}

Variant::Variant(unsigned char value)
  : Variant(g_variant_new_byte(value))
{}

Variant::Variant(int16_t value)
  : Variant(g_variant_new_int16(value))
{}

Variant::Variant(uint16_t value)
  : Variant(g_variant_new_uint16(value))
{}

Variant::Variant(int32_t value)
  : Variant(g_variant_new_int32(value))
{}

Variant::Variant(uint32_t value)
  : Variant(g_variant_new_uint32(value))
{}

Variant::Variant(int64_t value)
  : Variant(g_variant_new_int64(value))
{}

Variant::Variant(uint64_t value)
  : Variant(g_variant_new_uint64(value))
{}

#if __WORDSIZE != 64
Variant::Variant(long value)
  : Variant(static_cast<int64_t>(value))
{}

Variant::Variant(unsigned long value)
  : Variant(static_cast<uint64_t>(value))
{}
#endif

Variant::Variant(bool value)
  : Variant(g_variant_new_boolean(value))
{}

Variant::Variant(double value)
  : Variant(g_variant_new_double(value))
{}

Variant::Variant(float value)
  : Variant(static_cast<double>(value))
{}

Variant::Variant(HintsMap const& hints)
{
  GVariantBuilder b;
  g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));

  for (auto const& hint : hints)
    g_variant_builder_add(&b, "{sv}", hint.first.c_str(), static_cast<GVariant*>(hint.second));

  variant_ = g_variant_builder_end(&b);
  g_variant_ref_sink(variant_);
}

Variant::~Variant()
{
  if (variant_) g_variant_unref(variant_);
}

glib::Variant get_variant(GVariant *variant_)
{
  Variant value;

  if (!variant_)
    return value;

  if (g_variant_is_of_type(variant_, G_VARIANT_TYPE_VARIANT))
  {
    value = Variant(g_variant_get_variant(variant_), StealRef());
  }
  else if (g_variant_is_of_type(variant_, G_VARIANT_TYPE("(v)")))
  {
    g_variant_get(variant_, "(v)", &value);
  }

  return value;
}

std::string Variant::GetString() const
{
  const gchar *result = nullptr;

  if (!variant_)
    return "";

  if (g_variant_is_of_type(variant_, G_VARIANT_TYPE_STRING))
  {
    // g_variant_get_string doesn't duplicate the string
    result = g_variant_get_string(variant_, nullptr);
  }
  else if (g_variant_is_of_type(variant_, G_VARIANT_TYPE("(s)")))
  {
    // As we're using the '&' prefix we don't need to free the string!
    g_variant_get(variant_, "(&s)", &result);
  }
  else
  {
    auto const& variant = get_variant(variant_);
    if (variant)
      return variant.GetString();

    LOG_ERROR(logger) << "You're trying to extract a 's' from a variant which is of type '"
                      << g_variant_type_peek_string(g_variant_get_type(variant_)) << "'";
  }

  return result ? result : "";
}

template <typename TYPE, typename GTYPE>
TYPE get_numeric_value(GVariant *variant_, const char *type_str, const char *fallback_type_str)
{
  GTYPE value = 0;

  if (!variant_)
    return static_cast<TYPE>(value);

  if (g_variant_is_of_type(variant_, G_VARIANT_TYPE(type_str)))
  {
    g_variant_get(variant_, type_str, &value);
  }
  else if (g_variant_is_of_type(variant_, G_VARIANT_TYPE(fallback_type_str)))
  {
    g_variant_get(variant_, fallback_type_str, &value);
  }
  else
  {
    auto const& variant = get_variant(variant_);
    if (variant)
      return get_numeric_value<TYPE, GTYPE>(static_cast<GVariant*>(variant), type_str, fallback_type_str);

    LOG_ERROR(logger) << "You're trying to extract a '" << type_str << "'"
                      << " from a variant which is of type '"
                      << g_variant_type_peek_string(g_variant_get_type(variant_))
                      << "'";
  }

  return static_cast<TYPE>(value);
}

unsigned char Variant::GetByte() const
{
  return get_numeric_value<unsigned char, guchar>(variant_, "y", "(y)");
}

int16_t Variant::GetInt16() const
{
  return get_numeric_value<int16_t, gint16>(variant_, "n", "(n)");
}

uint16_t Variant::GetUInt16() const
{
  return get_numeric_value<uint16_t, guint16>(variant_, "q", "(q)");
}

int32_t Variant::GetInt32() const
{
  return get_numeric_value<int32_t, gint32>(variant_, "i", "(i)");
}

uint32_t Variant::GetUInt32() const
{
  return get_numeric_value<uint32_t, guint32>(variant_, "u", "(u)");
}

int64_t Variant::GetInt64() const
{
  return get_numeric_value<int64_t, gint64>(variant_, "x", "(x)");
}

uint64_t Variant::GetUInt64() const
{
  return get_numeric_value<uint64_t, guint64>(variant_, "t", "(t)");
}

bool Variant::GetBool() const
{
  return get_numeric_value<bool, gboolean>(variant_, "b", "(b)");
}

double Variant::GetDouble() const
{
  return get_numeric_value<double, gdouble>(variant_, "d", "(d)");
}

float Variant::GetFloat() const
{
  return static_cast<float>(GetDouble());
}

Variant Variant::GetVariant() const
{
  Variant value;

  if (!variant_)
    return value;

  value = get_variant(variant_);

  if (!value)
  {
    LOG_ERROR(logger) << "You're trying to extract a 'v' from a variant which is of type '"
                      << g_variant_type_peek_string(g_variant_get_type(variant_)) << "'";
  }

  return value;
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

  g_variant_iter_free(hints_iter);

  return true;
}

void Variant::swap(Variant& other)
{
  std::swap(this->variant_, other.variant_);
}

Variant& Variant::operator=(GVariant* val)
{
  Variant copy(val);
  swap(copy);

  return *this;
}

Variant& Variant::operator=(Variant other)
{
  swap(other);

  return *this;
}

Variant& Variant::operator=(std::nullptr_t) { return operator=(Variant()); }
Variant& Variant::operator=(HintsMap const& map) { return operator=(Variant(map)); }
Variant& Variant::operator=(std::string const& value) { return operator=(Variant(value)); }
Variant& Variant::operator=(const char* value) { return operator=(Variant(value)); }
Variant& Variant::operator=(unsigned char value) { return operator=(Variant(value)); }
Variant& Variant::operator=(int16_t value) { return operator=(Variant(value)); }
Variant& Variant::operator=(uint16_t value) { return operator=(Variant(value)); }
Variant& Variant::operator=(int32_t value) { return operator=(Variant(value)); }
Variant& Variant::operator=(uint32_t value) { return operator=(Variant(value)); }
Variant& Variant::operator=(int64_t value) { return operator=(Variant(value)); }
Variant& Variant::operator=(uint64_t value) { return operator=(Variant(value)); }
#if __WORDSIZE != 64
Variant& Variant::operator=(long value) { return operator=(Variant(value)); }
Variant& Variant::operator=(unsigned long value) { return operator=(Variant(value)); }
#endif
Variant& Variant::operator=(bool value) { return operator=(Variant(value)); }
Variant& Variant::operator=(double value) { return operator=(Variant(value)); }
Variant& Variant::operator=(float value) { return operator=(Variant(value)); }

Variant::operator GVariant* () const
{
  return variant_;
}

Variant::operator bool() const
{
  return bool(variant_);
}

static void g_variant_unref0(gpointer var)
{
  if (var)
    g_variant_unref((GVariant*)var);
}

GHashTable* hashtable_from_hintsmap(glib::HintsMap const& hints)
{
  GHashTable* hash_table = g_hash_table_new_full(g_str_hash, g_direct_equal, g_free, g_variant_unref0);

  if (!hash_table)
    return nullptr;

  for (auto const& hint : hints)
  {
    if (!hint.second)
      continue;

    g_hash_table_insert(hash_table, g_strdup(hint.first.c_str()), g_variant_ref(hint.second));
  }

  return hash_table;
}

HintsMap const& hintsmap_from_hashtable(GHashTable* hashtable, HintsMap& hints)
{
  if (!hashtable)
    return hints;

  GHashTableIter hints_iter;
  gpointer key, value;
  g_hash_table_iter_init(&hints_iter, hashtable);
  while (g_hash_table_iter_next(&hints_iter, &key, &value))
    hints.insert({static_cast<gchar*>(key), static_cast<GVariant*>(value)});

  return hints;
}

std::ostream& operator<<(std::ostream &os, GVariant* v)
{
  return os << (v ? String(g_variant_print(v, TRUE)) : "()");
}

std::ostream& operator<<(std::ostream &os, Variant const& v)
{
  return os << static_cast<GVariant*>(v);
}

} // namespace glib
} // namespace unity
