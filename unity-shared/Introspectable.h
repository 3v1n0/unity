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

#ifndef _INTROSPECTABLE_H
#define _INTROSPECTABLE_H

#include <glib.h>
#include <list>
#include <string>

namespace unity
{
namespace debug
{
class Introspectable
{
public:
  typedef std::list<Introspectable*> IntrospectableList;

  Introspectable();
  virtual ~Introspectable();
  GVariant* Introspect();
  virtual std::string GetName() const = 0;
  void AddChild(Introspectable* child);
  void RemoveChild(Introspectable* child);
  virtual void AddProperties(GVariantBuilder* builder) = 0;
  virtual IntrospectableList GetIntrospectableChildren();
  guint64 GetIntrospectionId() const;

protected:
  /// Please don't override this unless you really need to. The only valid reason
  /// is if you have a property that simply *must* be called 'Children'.
  virtual std::string GetChildsName() const;

  void RemoveAllChildren();

  /*
   * AddProperties should be implemented as such ...
   * void ClassFoo::AddProperties (GVariantBuilder *builder)
   * {
   *  unity::variant::BuilderWrapper wrapper(builder);
   *  wrapper.add("label", some_value);
   * }
   * That's all. Just add a bunch of key-value properties to the builder.
   */

private:
  std::list<Introspectable*> _children;
  std::list<Introspectable*> _parents;
  guint64 _id;
};
}
}
#endif
