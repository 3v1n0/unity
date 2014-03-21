// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Nick Dedekind <nick.dedekinc@canonical.com>
 */

#include <list>
#include <gmock/gmock.h>
using namespace testing;

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <unity-shared/StaticCairoText.h>
#include <unity-shared/DashStyle.h>
#include <unity-shared/PreviewStyle.h>
#include <unity-shared/ThumbnailGenerator.h>

#include <unity-protocol.h>
#include "UnityCore/MusicPreview.h"
#include "dash/previews/MusicPreview.h"
#include "dash/previews/PreviewInfoHintWidget.h"
#include "dash/previews/PreviewRatingsWidget.h"
#include "dash/previews/ActionButton.h"
#include "test_utils.h"
using namespace unity;
using namespace unity::dash;

namespace
{

class MockMusicPreview : public previews::MusicPreview
{
public:
  typedef nux::ObjectPtr<MockMusicPreview> Ptr;

  MockMusicPreview(dash::Preview::Ptr preview_model)
  : MusicPreview(preview_model)
  {}

  using MusicPreview::title_;
  using MusicPreview::subtitle_;
  using MusicPreview::action_buttons_;
  using MusicPreview::preview_info_hints_;
};

class TestPreviewMusic : public Test
{
public:
  TestPreviewMusic()
  : parent_window_(new nux::BaseWindow("TestPreviewMusic"))
  {
    glib::Object<UnityProtocolPreview> proto_obj(UNITY_PROTOCOL_PREVIEW(unity_protocol_music_preview_new()));

    GHashTable* action_hints1(g_hash_table_new(g_direct_hash, g_direct_equal));
    g_hash_table_insert (action_hints1, g_strdup ("extra-text"), g_variant_new_string("£3.99"));

    unity_protocol_preview_set_image_source_uri(proto_obj, "http://ia.media-imdb.com/images/M/MV5BMTM3NDM5MzY5Ml5BMl5BanBnXkFtZTcwNjExMDUwOA@@._V1._SY317_.jpg");
    unity_protocol_preview_set_title(proto_obj, "Music Title & special char");
    unity_protocol_preview_set_subtitle(proto_obj, "Music Subtitle > special char");
    unity_protocol_preview_set_description(proto_obj, "Music Desctiption &lt; special char");
    unity_protocol_preview_add_action(proto_obj, "action1", "Action 1", NULL, 0);
    unity_protocol_preview_add_action_with_hints(proto_obj, "action2", "Action 2", NULL, 0, action_hints1);
    unity_protocol_preview_add_action(proto_obj, "action3", "Action 3", NULL, 0);
    unity_protocol_preview_add_action(proto_obj, "action4", "Action 4", NULL, 0);
    unity_protocol_preview_add_info_hint(proto_obj, "hint1", "Hint 1", NULL, g_variant_new("s", "string hint 1"));
    unity_protocol_preview_add_info_hint(proto_obj, "hint2", "Hint 2", NULL, g_variant_new("s", "string hint 2"));
    unity_protocol_preview_add_info_hint(proto_obj, "hint3", "Hint 3", NULL, g_variant_new("i", 12));

    glib::Variant v(dee_serializable_serialize(DEE_SERIALIZABLE(proto_obj.RawPtr())), glib::StealRef());
    preview_model_ = dash::Preview::PreviewForVariant(v);

    g_hash_table_unref(action_hints1);
  }

  nux::ObjectPtr<nux::BaseWindow> parent_window_;
  dash::Preview::Ptr preview_model_;

  previews::Style panel_style;
  dash::Style dash_style;
  ThumbnailGenerator thumbnail_generator;
};

TEST_F(TestPreviewMusic, TestCreate)
{
  previews::Preview::Ptr preview_view = previews::Preview::PreviewForModel(preview_model_);

  EXPECT_TRUE(dynamic_cast<previews::MusicPreview*>(preview_view.GetPointer()) != NULL);
}

TEST_F(TestPreviewMusic, TestUIValues)
{
  MockMusicPreview::Ptr preview_view(new MockMusicPreview(preview_model_));

  EXPECT_EQ(preview_view->title_->GetText(), "Music Title &amp; special char");
  EXPECT_EQ(preview_view->subtitle_->GetText(), "Music Subtitle &gt; special char");

  EXPECT_EQ(preview_view->action_buttons_.size(), 4);

  if (preview_view->action_buttons_.size() >= 2)
  {
    auto iter = preview_view->action_buttons_.begin();
    if ((*iter)->Type().IsDerivedFromType(ActionButton::StaticObjectType))
    {
      ActionButton *action = static_cast<ActionButton*>(*iter);
      EXPECT_EQ(action->GetLabel(), "Action 1");
      EXPECT_EQ(action->GetExtraText(), "");
    }
    iter++;
    if ((*iter)->Type().IsDerivedFromType(ActionButton::StaticObjectType))
    {
      ActionButton *action = static_cast<ActionButton*>(*iter);
      EXPECT_EQ(action->GetLabel(), "Action 2");
      EXPECT_EQ(action->GetExtraText(), "£3.99");
    }
  }
}

}
