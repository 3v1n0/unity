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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include <NuxCore/Logger.h>

#include "Preview.h"

#include "ApplicationPreview.h"
#include "GenericPreview.h"
#include "MusicPreview.h"
#include "MoviePreview.h"
#include "SeriesPreview.h"

namespace unity
{
namespace dash
{

namespace
{
  nux::logging::Logger logger("unity.dash.preview");
}

Preview::Ptr Preview::PreviewForVariant(unity::glib::Variant &properties)
{
  unity::glib::Variant renderer(g_variant_get_child_value(properties, 0));
  std::string renderer_name(renderer.GetString());
  unity::glib::Object<UnityProtocolPreview> proto_obj(unity_protocol_preview_parse(properties));

  if (renderer_name == "preview-generic")
  {
    return Preview::Ptr(new GenericPreview(proto_obj));
  }
  else if (renderer_name == "preview-application")
  {
    return Preview::Ptr(new ApplicationPreview(proto_obj));
  }
  else if (renderer_name == "preview-music")
  {
    return Preview::Ptr(new MusicPreview(proto_obj));
  }
  else if (renderer_name == "preview-movie")
  {
    return Preview::Ptr(new MoviePreview(proto_obj));
  }
  else if (renderer_name == "preview-series")
  {
    return Preview::Ptr(new SeriesPreview(proto_obj));
  }

  return nullptr;
}

unity::glib::Object<GIcon> Preview::IconForString(std::string const& icon_hint)
{
  glib::Error error;
  glib::Object<GIcon> icon(g_icon_new_for_string(icon_hint.c_str(), &error));

  if (error)
  {
    LOG_WARN(logger) << "Unable to instantiate icon: " << icon_hint;
  }

  return icon;
}

Preview::Preview(glib::Object<UnityProtocolPreview> const& proto_obj)
{
  SetupGetters();

  if (!proto_obj) LOG_WARN(logger) << "Passed nullptr to Preview constructor";
  else
  {
    const gchar *s;
    s = unity_protocol_preview_get_title (proto_obj);
    if (s) title_ = s;
    s = unity_protocol_preview_get_subtitle (proto_obj);
    if (s) subtitle_ = s;
    s = unity_protocol_preview_get_description (proto_obj);
    if (s) description_ = s;
    thumbnail_ = unity_protocol_preview_get_thumbnail (proto_obj);
  }
}

Preview::~Preview()
{}

void Preview::SetupGetters()
{
  title.SetGetterFunction(sigc::mem_fun(this, &Preview::get_title));
  subtitle.SetGetterFunction(sigc::mem_fun(this, &Preview::get_subtitle));
  description.SetGetterFunction(sigc::mem_fun(this, &Preview::get_description));
  thumbnail.SetGetterFunction(sigc::mem_fun(this, &Preview::get_thumbnail));
}

} // namespace dash
} // namespace unity
