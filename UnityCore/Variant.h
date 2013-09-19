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

namespace variant
{

class BuilderWrapper
{
// XXX: Move this to Introspectable
public:
  enum class ValueType : uint32_t
  {
    // This should match the Autopilot Type IDs
    SIMPLE = 0,
    RECTANGLE = 1,
    POINT = 2,
    SIZE = 3,
    COLOR = 4,
    DATE = 5,
    TIME = 6,
  };

  BuilderWrapper(GVariantBuilder* builder);

  BuilderWrapper& add(std::string const& name, bool);
  BuilderWrapper& add(std::string const& name, const char*);
  BuilderWrapper& add(std::string const& name, std::string const&);
  BuilderWrapper& add(std::string const& name, int16_t);
  BuilderWrapper& add(std::string const& name, int32_t);
  BuilderWrapper& add(std::string const& name, int64_t);
  BuilderWrapper& add(std::string const& name, uint16_t);
  BuilderWrapper& add(std::string const& name, uint32_t);
  BuilderWrapper& add(std::string const& name, uint64_t);
  BuilderWrapper& add(std::string const& name, float);
  BuilderWrapper& add(std::string const& name, double);
  BuilderWrapper& add(std::string const& name, glib::Variant const&);
  BuilderWrapper& add(std::string const& name, GVariant*);

  BuilderWrapper& add(std::string const& name, nux::Rect const&);
  BuilderWrapper& add(std::string const& name, nux::Point const&);
  BuilderWrapper& add(std::string const& name, nux::Size const&);
  BuilderWrapper& add(std::string const& name, nux::Color const&);
  BuilderWrapper& add(nux::Rect const&);

private:
  BuilderWrapper& add(std::string const& name, ValueType, std::vector<glib::Variant> const&);
  GVariantBuilder* builder_;
};

}
}

#endif
