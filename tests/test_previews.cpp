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
 * Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#include <list>
#include <algorithm>
#include <gmock/gmock.h>
#include <gio/gio.h>
#include <UnityCore/Variant.h>
#include <UnityCore/Preview.h>
#include <unity-protocol.h>

using namespace std;
using namespace testing;
using namespace unity;
using namespace unity::glib;
using namespace unity::dash;

namespace
{

bool IsVariant(Variant const& variant)
{
  return g_variant_get_type_string (variant) != NULL;
}

TEST(TestPreviews, DeserializeGeneric)
{
  Object<GIcon> icon(g_icon_new_for_string ("accessories", NULL));
  Object<UnityProtocolPreview> proto_obj(UNITY_PROTOCOL_PREVIEW (unity_protocol_generic_preview_new ()));
  unity_protocol_preview_set_title (proto_obj, "Title");
  unity_protocol_preview_set_subtitle (proto_obj, "Subtitle");
  unity_protocol_preview_set_description (proto_obj, "Description");
  unity_protocol_preview_set_thumbnail (proto_obj, icon);

  Variant v (dee_serializable_serialize (DEE_SERIALIZABLE (proto_obj.RawPtr())),
             glib::StealRef());
  EXPECT_TRUE(IsVariant(v));

  Preview::Ptr preview = Preview::PreviewForVariant(v);
  EXPECT_TRUE(preview != nullptr);

  EXPECT_EQ(preview->title, "Title");
  EXPECT_EQ(preview->subtitle, "Subtitle");
  EXPECT_EQ(preview->description, "Description");
  EXPECT_TRUE(g_icon_equal(preview->image(), icon) != FALSE);
}

TEST(TestPreviews, DeserializeGenericWithMeta)
{
  Object<GIcon> icon(g_icon_new_for_string ("accessories", NULL));
  Object<UnityProtocolPreview> proto_obj(UNITY_PROTOCOL_PREVIEW (unity_protocol_generic_preview_new ()));
  unity_protocol_preview_set_title (proto_obj, "Title");
  unity_protocol_preview_set_subtitle (proto_obj, "Subtitle");
  unity_protocol_preview_set_description (proto_obj, "Description");
  unity_protocol_preview_set_thumbnail (proto_obj, icon);
  unity_protocol_preview_add_action (proto_obj, "action1", "Action #1", NULL, 0);
  unity_protocol_preview_add_action (proto_obj, "action2", "Action #2", NULL, 0);
  unity_protocol_preview_add_info_hint (proto_obj, "hint1", "Hint 1", NULL, g_variant_new ("i", 34));
  unity_protocol_preview_add_info_hint (proto_obj, "hint2", "Hint 2", NULL, g_variant_new ("s", "string hint"));

  Variant v (dee_serializable_serialize (DEE_SERIALIZABLE (proto_obj.RawPtr())),
             glib::StealRef());
  EXPECT_TRUE(IsVariant(v));

  Preview::Ptr preview = Preview::PreviewForVariant(v);
  EXPECT_TRUE(preview != nullptr);

  EXPECT_EQ(preview->title, "Title");
  EXPECT_EQ(preview->subtitle, "Subtitle");
  EXPECT_EQ(preview->description, "Description");
  EXPECT_TRUE(g_icon_equal(preview->image(), icon) != FALSE);

  auto actions = preview->GetActions();
  auto info_hints = preview->GetInfoHints();

  EXPECT_EQ(actions.size(), 2);

  auto action1 = actions[0];
  EXPECT_EQ(action1->id, "action1");
  EXPECT_EQ(action1->display_name, "Action #1");
  EXPECT_EQ(action1->icon_hint, "");
  EXPECT_EQ(action1->layout_hint, 0);

  auto action2 = actions[1];
  EXPECT_EQ(action2->id, "action2");
  EXPECT_EQ(action2->display_name, "Action #2");
  EXPECT_EQ(action2->icon_hint, "");

  EXPECT_EQ(info_hints.size(), 2);
  auto hint1 = info_hints[0];
  EXPECT_EQ(hint1->id, "hint1");
  EXPECT_EQ(hint1->display_name, "Hint 1");
  EXPECT_EQ(hint1->icon_hint, "");
  EXPECT_EQ(hint1->value.GetInt(), 34);
  auto hint2 = info_hints[1];
  EXPECT_EQ(hint2->id, "hint2");
  EXPECT_EQ(hint2->display_name, "Hint 2");
  EXPECT_EQ(hint2->icon_hint, "");
  EXPECT_EQ(hint2->value.GetString(), "string hint");
}

} // Namespace
