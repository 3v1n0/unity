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
#include <unity-protocol.h>

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
  unity::glib::Object<UnityProtocolPreview> preview(unity_protocol_preview_parse(properties));
  if (!preview)
  {
    LOG_WARN(logger) << "Unable to create Preview object for variant type: " << g_variant_get_type_string(properties);
    return nullptr;
  }

  auto proto_obj = unity::glib::object_cast<GObject>(preview);

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
  else
  {
    LOG_WARN(logger) << "Unable to create Preview for renderer: " << renderer_name;
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

Preview::Preview(glib::Object<GObject> const& proto_obj)
{
  SetupGetters();

  if (!proto_obj)
  {
    LOG_WARN(logger) << "Passed nullptr to Preview constructor";
  }
  else if (UNITY_PROTOCOL_IS_PREVIEW(proto_obj.RawPtr()))
  {
    auto preview = glib::object_cast<UnityProtocolPreview>(proto_obj);
    const gchar *s;
    s = unity_protocol_preview_get_title(preview);
    if (s) title_ = s;
    s = unity_protocol_preview_get_subtitle(preview);
    if (s) subtitle_ = s;
    s = unity_protocol_preview_get_description(preview);
    if (s) description_ = s;
    glib::Object<GIcon> icon(unity_protocol_preview_get_thumbnail(preview),
                             unity::glib::AddRef());
    thumbnail_ = icon;

    int actions_len;
    auto actions = unity_protocol_preview_get_actions(preview, &actions_len);
    for (int i = 0; i < actions_len; i++)
    {
      UnityProtocolPreviewActionRaw *raw_action = &actions[i];
      actions_list_.push_back(std::make_shared<Action>(
            raw_action->id, raw_action->display_name,
            raw_action->icon_hint, raw_action->layout_hint));
    }
    
    int info_hints_len;
    auto info_hints = unity_protocol_preview_get_info_hints(preview, &info_hints_len);
    for (int i = 0; i < info_hints_len; i++)
    {
      UnityProtocolInfoHintRaw *raw_hint = &info_hints[i];
      info_hint_list_.push_back(std::make_shared<InfoHint>(
            raw_hint->id, raw_hint->display_name, raw_hint->icon_hint,
            raw_hint->value));
    }
  }
  else
  {
    LOG_WARN(logger) << "Object passed to Preview constructor isn't UnityProtocolPreview";
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

Preview::ActionPtrList Preview::GetActions() const
{
  return actions_list_;
}

Preview::InfoHintPtrList Preview::GetInfoHints() const
{
  return info_hint_list_;
}

} // namespace dash
} // namespace unity
