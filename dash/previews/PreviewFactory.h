// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

#ifndef PREVIEWFACTORY_H
#define PREVIEWFACTORY_H

#include "PreviewFactoryItem.h"
#include <map>

namespace unity
{
namespace dash
{
class PreviewFactory
{
  typedef std::map<std::string, IPreviewFactoryItem*> FactoryItems;
public:
  PreviewFactory();
  ~PreviewFactory();
  
  static PreviewFactory& Instance();

  bool RegisterItem(std::string const& renderer_name, IPreviewFactoryItem*const item);

  PreviewFactoryOperator Item(glib::Variant const& properties);
  PreviewFactoryOperator Item(glib::Object<GObject> const& proto_obj);

private:

  FactoryItems factory_items_;
};

} // namespace dash
} // namespace unity

#endif //PREVIEWFACTORY_H
