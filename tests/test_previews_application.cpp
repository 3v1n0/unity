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
#include "UnityCore/ApplicationPreview.h"
#include "dash/previews/ApplicationPreview.h"
#include "dash/previews/PreviewInfoHintWidget.h"
#include "dash/previews/PreviewRatingsWidget.h"
#include "dash/previews/ActionButton.h"
#include "test_utils.h"
using namespace unity;
using namespace unity::dash;

namespace
{

class MockApplicationPreview : public previews::ApplicationPreview
{
public:
  typedef nux::ObjectPtr<MockApplicationPreview> Ptr;

  MockApplicationPreview(dash::Preview::Ptr preview_model)
  : ApplicationPreview(preview_model)
  {}

  using ApplicationPreview::title_;
  using ApplicationPreview::subtitle_;
  using ApplicationPreview::license_;
  using ApplicationPreview::last_update_;
  using ApplicationPreview::copywrite_;
  using ApplicationPreview::description_;
  using ApplicationPreview::action_buttons_;
  using ApplicationPreview::preview_info_hints_;
  using ApplicationPreview::app_rating_;
};

class TestPreviewApplication : public Test
{
public:
  TestPreviewApplication()
  : parent_window_(new nux::BaseWindow("TestPreviewApplication"))
  {
    glib::Object<UnityProtocolPreview> proto_obj(UNITY_PROTOCOL_PREVIEW(unity_protocol_application_preview_new()));

    GHashTable* action_hints1(g_hash_table_new(g_direct_hash, g_direct_equal));
    g_hash_table_insert (action_hints1, g_strdup ("extra-text"), g_variant_new_string("£30.99"));

    unity_protocol_application_preview_set_app_icon(UNITY_PROTOCOL_APPLICATION_PREVIEW(proto_obj.RawPtr()), g_icon_new_for_string(TESTDATADIR "/bfb.png", NULL));
    unity_protocol_application_preview_set_license(UNITY_PROTOCOL_APPLICATION_PREVIEW(proto_obj.RawPtr()), "License & special char");
    unity_protocol_application_preview_set_copyright(UNITY_PROTOCOL_APPLICATION_PREVIEW(proto_obj.RawPtr()), "Copywrite & special char");
    unity_protocol_application_preview_set_last_update(UNITY_PROTOCOL_APPLICATION_PREVIEW(proto_obj.RawPtr()), "11th Apr 2012");
    unity_protocol_application_preview_set_rating(UNITY_PROTOCOL_APPLICATION_PREVIEW(proto_obj.RawPtr()), 0.8);
    unity_protocol_application_preview_set_num_ratings(UNITY_PROTOCOL_APPLICATION_PREVIEW(proto_obj.RawPtr()), 12);

    unity_protocol_preview_set_image_source_uri(proto_obj, "http://ia.media-imdb.com/images/M/MV5BMTM3NDM5MzY5Ml5BMl5BanBnXkFtZTcwNjExMDUwOA@@._V1._SY317_.jpg");
    unity_protocol_preview_set_title(proto_obj, "Application Title & special char");
    unity_protocol_preview_set_subtitle(proto_obj, "Application Subtitle > special char");
    unity_protocol_preview_set_description(proto_obj, "Application Desctiption &lt; special char");
    unity_protocol_preview_add_action(proto_obj, "action1", "Action 1", NULL, 0);
    unity_protocol_preview_add_action_with_hints(proto_obj, "action2", "Action 2", NULL, 0, action_hints1);
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

TEST_F(TestPreviewApplication, TestCreate)
{
  previews::Preview::Ptr preview_view = previews::Preview::PreviewForModel(preview_model_);

  EXPECT_TRUE(dynamic_cast<previews::ApplicationPreview*>(preview_view.GetPointer()) != NULL);
}

TEST_F(TestPreviewApplication, TestUIValues)
{  
  MockApplicationPreview::Ptr preview_view(new MockApplicationPreview(preview_model_));

  EXPECT_EQ(preview_view->title_->GetText(), "Application Title &amp; special char");
  EXPECT_EQ(preview_view->subtitle_->GetText(), "Application Subtitle &gt; special char");
  EXPECT_EQ(preview_view->description_->GetText(), "Application Desctiption &lt; special char");
  EXPECT_EQ(preview_view->license_->GetText(), "License &amp; special char");
  //EXPECT_EQ(preview_view->last_update_->GetText(), "Last Updated 11th Apr 2012"); // Not 100% sure this will work with translations.
  EXPECT_EQ(preview_view->copywrite_->GetText(), "Copywrite &amp; special char");

  EXPECT_EQ(preview_view->app_rating_->GetRating(), 0.8f);
  EXPECT_EQ(preview_view->action_buttons_.size(), 2);

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
      EXPECT_EQ(action->GetExtraText(), "£30.99");
    }
  }
}

}
