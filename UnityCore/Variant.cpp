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
  const gchar *result = nullptr;

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
    LOG_ERROR(logger) << "You're trying to extract a String from a variant which is of type "
                      << g_variant_type_peek_string(g_variant_get_type(variant_));
  }

  return result ? result : "";
}

int Variant::GetInt() const
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
    LOG_ERROR(logger) << "You're trying to extract an Int32 from a variant which is of type "
                      << g_variant_type_peek_string(g_variant_get_type(variant_));
  }

  return static_cast<int>(value);
}

unsigned Variant::GetUInt() const
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
    LOG_ERROR(logger) << "You're trying to extract an Uint32 from a variant which is of type "
                      << g_variant_type_peek_string(g_variant_get_type(variant_));
  }

  return static_cast<unsigned>(value);
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
    LOG_ERROR(logger) << "You're trying to extract a Boolean from a variant which is of type "
                      << g_variant_type_peek_string(g_variant_get_type(variant_));
  }

  return (value != FALSE);
}

bool Variant::ASVToHints(HintsMap& hints) const
{
  GVariantIter* hints_iter;
  char* key = NULL;
  GVariant* value = NULL;

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

BuilderWrapper& BuilderWrapper::add(char const* name, int value)
{
  g_variant_builder_add(builder_, "{sv}", name, g_variant_new_int32(value));
  return *this;
}

BuilderWrapper& BuilderWrapper::add(char const* name, long int value)
{
  g_variant_builder_add(builder_, "{sv}", name, g_variant_new_int64(value));
  return *this;
}

BuilderWrapper& BuilderWrapper::add(char const* name, long long int value)
{
  g_variant_builder_add(builder_, "{sv}", name, g_variant_new_int64(value));
  return *this;
}

BuilderWrapper& BuilderWrapper::add(char const* name, unsigned int value)
{
  g_variant_builder_add(builder_, "{sv}", name, g_variant_new_uint32(value));
  return *this;
}

BuilderWrapper& BuilderWrapper::add(char const* name, long unsigned int value)
{
  g_variant_builder_add(builder_, "{sv}", name, g_variant_new_uint64(value));
  return *this;
}

BuilderWrapper& BuilderWrapper::add(char const* name, long long unsigned int value)
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
