// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011-2013 Canonical Ltd
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

#ifndef UNITY_VARIANT_H
#define UNITY_VARIANT_H

#include <string>
#include <glib.h>
#include <vector>
#include <map>

namespace unity
{
namespace glib
{

class Variant;
typedef std::map<std::string, Variant> HintsMap;
GHashTable* hashtable_from_hintsmap(HintsMap const& hints);
HintsMap const& hintsmap_from_hashtable(GHashTable* hashtable, HintsMap& hints);

struct StealRef {};

class Variant
{
public:
  Variant();
  Variant(GVariant*);
  Variant(GVariant*, StealRef const&);

  Variant(HintsMap const& hints);
  explicit Variant(std::nullptr_t);
  explicit Variant(std::string const&);
  explicit Variant(const char*);
  explicit Variant(unsigned char);
  explicit Variant(int16_t);
  explicit Variant(uint16_t);
  explicit Variant(int32_t);
  explicit Variant(uint32_t);
  explicit Variant(int64_t);
  explicit Variant(uint64_t);
  explicit Variant(bool);
  explicit Variant(double);
  explicit Variant(float);

  Variant(Variant const&);
  ~Variant();

  std::string GetString() const;
  unsigned char GetByte() const;
  int16_t GetInt16() const;
  uint16_t GetUInt16() const;
  int32_t GetInt32() const;
  uint32_t GetUInt32() const;
  int64_t GetInt64() const;
  uint64_t GetUInt64() const;
  bool GetBool() const;
  double GetDouble() const;
  float GetFloat() const;
  Variant GetVariant() const;

  bool ASVToHints(HintsMap& hints) const;

  template <typename T>
  static inline Variant FromVector(std::vector<T> const& values)
  {
    if (values.empty())
      return g_variant_new_array(G_VARIANT_TYPE_VARIANT, nullptr, 0);

    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
    for (auto const& value : values)
      g_variant_builder_add_value(&builder, Variant(value));

    return g_variant_builder_end(&builder);
  }

  void swap(Variant&);
  Variant& operator=(GVariant*);
  Variant& operator=(Variant);
  Variant& operator=(HintsMap const&);
  Variant& operator=(std::nullptr_t);
  Variant& operator=(std::string const&);
  Variant& operator=(const char*);
  Variant& operator=(unsigned char);
  Variant& operator=(int16_t);
  Variant& operator=(uint16_t);
  Variant& operator=(int32_t);
  Variant& operator=(uint32_t);
  Variant& operator=(int64_t);
  Variant& operator=(uint64_t);
  Variant& operator=(bool);
  Variant& operator=(double);
  Variant& operator=(float);
  operator GVariant*() const;
  operator bool() const;

private:
  GVariant* variant_;
};

}

}

#endif
