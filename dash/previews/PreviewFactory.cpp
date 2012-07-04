// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2011 Canonical Ltd.
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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *
 */

#include "PreviewFactory.h"
#include "unity-shared/IntrospectableWrappers.h"
#include <NuxCore/Logger.h>
#include <unity-protocol.h>

#include "GenericPreview.h"

namespace unity
{
namespace dash
{
namespace
{
nux::logging::Logger logger("unity.dash.previewfactory");
}


PreviewFactory::PreviewFactory()
{
  RegisterItem("preview-generic", new PreviewFactoryItem<dash::GenericPreview, previews::GenericPreview>());
}

PreviewFactory::~PreviewFactory()
{
}

PreviewFactory& PreviewFactory::Instance()
{
  static PreviewFactory factory;
  return factory;
}

bool PreviewFactory::RegisterItem(std::string const& renderer_name, IPreviewFactoryItem*const item)
{
  LOG_INFO(logger) << "Registering dash preview " << renderer_name; 

  if (factory_items_.find(renderer_name) != factory_items_.end())
  {
    delete item;
    return false;
  }

  factory_items_[renderer_name] = item;
  return true;
}

PreviewFactoryOperator PreviewFactory::Item(glib::Variant& properties)
{  
  static PreviewFactoryOperator nullOperator;
  glib::Object<UnityProtocolPreview> preview(unity_protocol_preview_parse(properties));
  if (!preview)
  {
    LOG_WARN(logger) << "Unable to create Preview object for variant type: " << g_variant_get_type_string(properties);
    return nullOperator;
  }

  auto proto_obj = unity::glib::object_cast<GObject>(preview);
  return Item(proto_obj);
}

PreviewFactoryOperator PreviewFactory::Item(glib::Object<GObject> const& proto_obj)
{
  static PreviewFactoryOperator nullOperator;
  if (!proto_obj || !UNITY_PROTOCOL_IS_PREVIEW(proto_obj.RawPtr()))
  {
    LOG_WARN(logger) << "Unable to create Preview object from " << proto_obj.RawPtr();
    return nullOperator;
  }

  std::string renderer_name(
      unity_protocol_preview_get_renderer_name(
        UNITY_PROTOCOL_PREVIEW(proto_obj.RawPtr())));

  auto iter = factory_items_.find(renderer_name);
  if (iter == factory_items_.end())
  {
    LOG_WARN(logger) << "Factory item '" << renderer_name << " not registered.";
    return nullOperator;
  }

  return PreviewFactoryOperator(proto_obj, (*iter).second);
}

} // namespace dash
} // namespace unity
