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
#include <NuxCore/Logger.h>

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

Variant::Variant(bool value)
  : Variant(g_variant_new_boolean(value))
{}

Variant::Variant(double value)
  : Variant(g_variant_new_double(value))
{}

Variant::Variant(float value)
  : Variant(static_cast<double>(value))
{}

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

    LOG_ERROR(logger) << "You're trying to extract a String from a variant which is of type "
                      << g_variant_type_peek_string(g_variant_get_type(variant_));
  }

  return result ? result : "";
}

int16_t Variant::GetInt16() const
{
  gint16 value = 0;

  if (!variant_)
    return static_cast<int>(value);

  if (g_variant_is_of_type(variant_, G_VARIANT_TYPE_INT16))
  {
    value = g_variant_get_int16(variant_);
  }
  else if (g_variant_is_of_type(variant_, G_VARIANT_TYPE("(n)")))
  {
    g_variant_get(variant_, "(n)", &value);
  }
  else
  {
    auto const& variant = get_variant(variant_);
    if (variant)
      return variant.GetInt16();

    LOG_ERROR(logger) << "You're trying to extract an Int16 from a variant which is of type "
                      << g_variant_type_peek_string(g_variant_get_type(variant_));
  }

  return static_cast<int16_t>(value);
}

uint16_t Variant::GetUInt16() const
{
  guint16 value = 0;

  if (!variant_)
    return static_cast<unsigned>(value);

  if (g_variant_is_of_type(variant_, G_VARIANT_TYPE_UINT16))
  {
    value = g_variant_get_uint16(variant_);
  }
  else if (g_variant_is_of_type(variant_, G_VARIANT_TYPE("(q)")))
  {
    g_variant_get(variant_, "(q)", &value);
  }
  else
  {
    auto const& variant = get_variant(variant_);
    if (variant)
      return variant.GetUInt16();

    LOG_ERROR(logger) << "You're trying to extract an UInt16 from a variant which is of type "
                      << g_variant_type_peek_string(g_variant_get_type(variant_));
  }

  return static_cast<uint16_t>(value);
}

int32_t Variant::GetInt32() const
{
  gint32 value = 0;

  if (!variant_)
    return static_cast<int>(value);

  if (g_variant_is_of_type(variant_, G_VARIANT_TYPE_INT32))
  {
    value = g_variant_get_int32(variant_);
  }
  else if (g_variant_is_of_type(variant_, G_VARIANT_TYPE("(i)")))
  {
    g_variant_get(variant_, "(i)", &value);
  }
  else
  {
    auto const& variant = get_variant(variant_);
    if (variant)
      return variant.GetInt32();

    LOG_ERROR(logger) << "You're trying to extract an Int32 from a variant which is of type "
                      << g_variant_type_peek_string(g_variant_get_type(variant_));
  }

  return static_cast<int32_t>(value);
}

uint32_t Variant::GetUInt32() const
{
  guint32 value = 0;

  if (!variant_)
    return static_cast<unsigned>(value);

  if (g_variant_is_of_type(variant_, G_VARIANT_TYPE_UINT32))
  {
    value = g_variant_get_uint32(variant_);
  }
  else if (g_variant_is_of_type(variant_, G_VARIANT_TYPE("(u)")))
  {
    g_variant_get(variant_, "(u)", &value);
  }
  else
  {
    auto const& variant = get_variant(variant_);
    if (variant)
      return variant.GetUInt32();

    LOG_ERROR(logger) << "You're trying to extract an UInt32 from a variant which is of type "
                      << g_variant_type_peek_string(g_variant_get_type(variant_));
  }

  return static_cast<uint32_t>(value);
}

int64_t Variant::GetInt64() const
{
  gint64 value = 0;

  if (!variant_)
    return static_cast<int64_t>(value);

  if (g_variant_is_of_type(variant_, G_VARIANT_TYPE_INT64))
  {
    value = g_variant_get_int64(variant_);
  }
  else if (g_variant_is_of_type(variant_, G_VARIANT_TYPE("(x)")))
  {
    g_variant_get(variant_, "(x)", &value);
  }
  else
  {
    auto const& variant = get_variant(variant_);
    if (variant)
      return variant.GetInt64();

    LOG_ERROR(logger) << "You're trying to extract an Int64 from a variant which is of type "
                      << g_variant_type_peek_string(g_variant_get_type(variant_));
  }

  return static_cast<int64_t>(value);
}

uint64_t Variant::GetUInt64() const
{
  guint64 value = 0;

  if (!variant_)
    return static_cast<uint64_t>(value);

  if (g_variant_is_of_type(variant_, G_VARIANT_TYPE_UINT64))
  {
    value = g_variant_get_uint64(variant_);
  }
  else if (g_variant_is_of_type(variant_, G_VARIANT_TYPE("(t)")))
  {
    g_variant_get(variant_, "(t)", &value);
  }
  else
  {
    auto const& variant = get_variant(variant_);
    if (variant)
      return variant.GetUInt64();

    LOG_ERROR(logger) << "You're trying to extract an UInt64 from a variant which is of type "
                      << g_variant_type_peek_string(g_variant_get_type(variant_));
  }

  return static_cast<uint64_t>(value);
}

bool Variant::GetBool() const
{
  gboolean value = FALSE;

  if (!variant_)
    return (value != FALSE);

  if (g_variant_is_of_type(variant_, G_VARIANT_TYPE_BOOLEAN))
  {
    value = g_variant_get_boolean(variant_);
  }
  else if (g_variant_is_of_type(variant_, G_VARIANT_TYPE("(b)")))
  {
    g_variant_get(variant_, "(b)", &value);
  }
  else
  {
    auto const& variant = get_variant(variant_);
    if (variant)
      return variant.GetBool();

    LOG_ERROR(logger) << "You're trying to extract a Boolean from a variant which is of type "
                      << g_variant_type_peek_string(g_variant_get_type(variant_));
  }

  return (value != FALSE);
}

double Variant::GetDouble() const
{
  double value = 0.0;

  if (!variant_)
    return value;

  if (g_variant_is_of_type(variant_, G_VARIANT_TYPE_DOUBLE))
  {
    value = g_variant_get_double(variant_);
  }
  else if (g_variant_is_of_type(variant_, G_VARIANT_TYPE("(d)")))
  {
    g_variant_get(variant_, "(d)", &value);
  }
  else
  {
    auto const& variant = get_variant(variant_);
    if (variant)
      return variant.GetDouble();

    LOG_ERROR(logger) << "You're trying to extract a Double from a variant which is of type "
                      << g_variant_type_peek_string(g_variant_get_type(variant_));
  }

  return value;
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
    LOG_ERROR(logger) << "You're trying to extract a Variant from a variant which is of type "
                      << g_variant_type_peek_string(g_variant_get_type(variant_));
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
  Variant copy(val);
  swap(copy);

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
  {
    std::string hint_key(static_cast<gchar*>(key));
    glib::Variant hint_value(static_cast<GVariant*>(value));

    hints.insert({hint_key, hint_value});
  }
  return hints;
}

} // namespace glib

namespace variant
{
using namespace glib;

BuilderWrapper::BuilderWrapper(GVariantBuilder* builder)
  : builder_(builder)
{}

BuilderWrapper& BuilderWrapper::add(std::string const& name, ValueType type, std::vector<Variant> const& values)
{
  GVariantBuilder array;
  g_variant_builder_init(&array, G_VARIANT_TYPE("av"));
  g_variant_builder_add(&array, "v", g_variant_new_uint32(static_cast<uint32_t>(type)));

  for (auto const& value : values)
    g_variant_builder_add(&array, "v", static_cast<GVariant*>(value));

  g_variant_builder_add(builder_, "{sv}", name.c_str(), g_variant_builder_end(&array));

  return *this;
}

BuilderWrapper& BuilderWrapper::add(std::string const& name, bool value)
{
  add(name, ValueType::SIMPLE, {Variant(value)});
  return *this;
}

BuilderWrapper& BuilderWrapper::add(std::string const& name, char const* value)
{
  add(name, ValueType::SIMPLE, {Variant(value)});
  return *this;
}

BuilderWrapper& BuilderWrapper::add(std::string const& name, std::string const& value)
{
  add(name, ValueType::SIMPLE, {Variant(value)});
  return *this;
}

BuilderWrapper& BuilderWrapper::add(std::string const& name, int16_t value)
{
  add(name, ValueType::SIMPLE, {Variant(value)});
  return *this;
}

BuilderWrapper& BuilderWrapper::add(std::string const& name, int32_t value)
{
  add(name, ValueType::SIMPLE, {Variant(value)});
  return *this;
}

BuilderWrapper& BuilderWrapper::add(std::string const& name, int64_t value)
{
  add(name, ValueType::SIMPLE, {Variant(value)});
  return *this;
}

BuilderWrapper& BuilderWrapper::add(std::string const& name, uint16_t value)
{
  add(name, ValueType::SIMPLE, {Variant(value)});
  return *this;
}

BuilderWrapper& BuilderWrapper::add(std::string const& name, uint32_t value)
{
  add(name, ValueType::SIMPLE, {Variant(value)});
  return *this;
}

BuilderWrapper& BuilderWrapper::add(std::string const& name, uint64_t value)
{
  add(name, ValueType::SIMPLE, {Variant(value)});
  return *this;
}

BuilderWrapper& BuilderWrapper::add(std::string const& name, float value)
{
  add(name, ValueType::SIMPLE, {Variant(value)});
  return *this;
}

BuilderWrapper& BuilderWrapper::add(std::string const& name, double value)
{
  add(name, ValueType::SIMPLE, {Variant(value)});
  return *this;
}

BuilderWrapper& BuilderWrapper::add(std::string const& name, GVariant* value)
{
  add(name, ValueType::SIMPLE, {Variant(value)});
  return *this;
}

BuilderWrapper& BuilderWrapper::add(std::string const& name, nux::Rect const& r)
{
  add(name, ValueType::RECTANGLE, {Variant(r.x), Variant(r.y), Variant(r.width), Variant(r.height)});
  return *this;
}

BuilderWrapper& BuilderWrapper::add(std::string const& name, nux::Point const& p)
{
  add(name, ValueType::POINT, {Variant(p.x), Variant(p.y)});
  return *this;
}

BuilderWrapper& BuilderWrapper::add(std::string const& name, nux::Size const& s)
{
  add(name, ValueType::SIZE, {Variant(s.width), Variant(s.height)});
  return *this;
}

BuilderWrapper& BuilderWrapper::add(std::string const& name, nux::Color const& c)
{
  int32_t r = c.red * 255.0f, g = c.green * 255.0f, b = c.blue * 255.0f, a = c.alpha * 255.0f;
  add(name, ValueType::COLOR, {Variant(r), Variant(g), Variant(b), Variant(a)});
  return *this;
}

BuilderWrapper& BuilderWrapper::add(nux::Rect const& value)
{
  add("globalRect", value);
  // Legacy support
  add("x", value.x);
  add("y", value.y);
  add("width", value.width);
  add("height", value.height);
  return *this;
}


}
}
