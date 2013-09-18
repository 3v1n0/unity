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
#include <vector>
#include <map>

#include <NuxCore/Rect.h>
#include <NuxCore/Color.h>

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
// XXX: Move this to Introspectable
public:
  BuilderWrapper(GVariantBuilder* builder);

  BuilderWrapper& add(std::string const& name, bool value);
  BuilderWrapper& add(std::string const& name, const char* value);
  BuilderWrapper& add(std::string const& name, std::string const& value);
  BuilderWrapper& add(std::string const& name, int16_t value);
  BuilderWrapper& add(std::string const& name, int32_t value);
  BuilderWrapper& add(std::string const& name, int64_t value);
  BuilderWrapper& add(std::string const& name, uint16_t value);
  BuilderWrapper& add(std::string const& name, uint32_t value);
  BuilderWrapper& add(std::string const& name, uint64_t value);
  BuilderWrapper& add(std::string const& name, float value);
  BuilderWrapper& add(std::string const& name, double value);
  BuilderWrapper& add(std::string const& name, GVariant* value);

  BuilderWrapper& add(std::string const& name, nux::Rect const& value);
  BuilderWrapper& add(std::string const& name, nux::Point const& value);
  BuilderWrapper& add(std::string const& name, nux::Size const& value);
  BuilderWrapper& add(std::string const& name, nux::Color const& value);
  BuilderWrapper& add(nux::Rect const& value);

private:
  BuilderWrapper& add(std::string const& name, std::vector<int32_t> const& value);
  GVariantBuilder* builder_;
};

}
}

#endif
