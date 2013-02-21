// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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

#ifndef UNITY_VARIANT_H
#define UNITY_VARIANT_H

#include <string>
#include <glib.h>
#include <map>

#include <NuxCore/Rect.h>

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

  Variant(Variant const&);
  ~Variant();

  std::string GetString() const;
  int32_t GetInt32() const;
  uint32_t GetUInt32() const;
  int64_t GetInt64() const;
  uint64_t GetUInt64() const;
  bool GetBool() const;
  double GetDouble() const;
  float GetFloat() const;

  bool ASVToHints(HintsMap& hints) const;
  static Variant FromHints(HintsMap const& hints);

  void swap(Variant&);
  Variant& operator=(GVariant*);
  Variant& operator=(Variant);
  operator GVariant*() const;
  operator bool() const;

private:
  GVariant* variant_;
};

}

namespace variant
{

class BuilderWrapper
{
public:
  BuilderWrapper(GVariantBuilder* builder);

  BuilderWrapper& add(char const* name, bool value);
  BuilderWrapper& add(char const* name, char const* value);
  BuilderWrapper& add(char const* name, std::string const& value);
  BuilderWrapper& add(char const* name, int32_t value);
  BuilderWrapper& add(char const* name, uint32_t value);
  BuilderWrapper& add(char const* name, int64_t value);
  BuilderWrapper& add(char const* name, uint64_t value);
  BuilderWrapper& add(char const* name, float value);
  BuilderWrapper& add(char const* name, double value);
  BuilderWrapper& add(char const* name, GVariant* value);
  BuilderWrapper& add(nux::Rect const& value);

private:
  GVariantBuilder* builder_;
};

}
}

#endif
