// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012-2013 Canonical Ltd
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
 * Authored by: Manuel de la Pena <manuel.delapena@canonical.com>
 */

#include <list>
#include <gmock/gmock.h>

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <unity-shared/StaticCairoText.h>
#include <unity-shared/CoverArt.h>
#include <unity-shared/DashStyle.h>
#include <unity-shared/PreviewStyle.h>
#include <unity-shared/ThumbnailGenerator.h>
#include "unity-shared/UnitySettings.h"

#include <unity-protocol.h>
#include "dash/previews/MusicPaymentPreview.h"
#include "test_utils.h"

using namespace testing;

namespace unity
{

namespace dash
{

namespace previews
{

class MockedMusicPaymentPreview : public MusicPaymentPreview
{
public:
  typedef nux::ObjectPtr<MockedMusicPaymentPreview> Ptr;

  MockedMusicPaymentPreview(dash::Preview::Ptr preview_model)
  : MusicPaymentPreview(preview_model)
  {}

  using MusicPaymentPreview::image_;
  using MusicPaymentPreview::intro_;
  using MusicPaymentPreview::title_;
  using MusicPaymentPreview::subtitle_;
  using MusicPaymentPreview::email_label_;
  using MusicPaymentPreview::email_;
  using MusicPaymentPreview::payment_label_;
  using MusicPaymentPreview::payment_;
  using MusicPaymentPreview::password_label_;
  using MusicPaymentPreview::password_entry_;
  using MusicPaymentPreview::purchase_hint_;
  using MusicPaymentPreview::purchase_prize_;
  using MusicPaymentPreview::purchase_type_;
  using MusicPaymentPreview::change_payment_;
  using MusicPaymentPreview::forgotten_password_;
  using MusicPaymentPreview::error_label_;
  using MusicPaymentPreview::form_layout_;
  using MusicPaymentPreview::SetupViews;
};

class TestMusicPaymentPreview : public ::testing::Test
{
  protected:
  TestMusicPaymentPreview() :
    Test(),
    parent_window_(new nux::BaseWindow("TestPreviewMusicPayment"))
  {
    title = "Turning Japanese";
    subtitle = "The vapors";
    header = "Hi test, you purchased in the past from Ubuntu One.";
    email = "test@canonical.com";
    payment_method = "*** *** ** 12";
    purchase_prize = "65$";
    purchase_type = "Mp3";
    preview_type = UNITY_PROTOCOL_PREVIEW_PAYMENT_TYPE_MUSIC;  

    glib::Object<UnityProtocolPreview> proto_obj(UNITY_PROTOCOL_PREVIEW(unity_protocol_payment_preview_new()));

    unity_protocol_preview_set_title(proto_obj, title.c_str());
    unity_protocol_preview_set_subtitle(proto_obj, subtitle.c_str());
    unity_protocol_preview_add_action(proto_obj, "change_payment_method", "Change payment", NULL, 0);
    unity_protocol_preview_add_action(proto_obj, "forgot_password", "Forgot password", NULL, 0);
    unity_protocol_preview_add_action(proto_obj, "cancel_purchase", "Cancel", NULL, 0);
    unity_protocol_preview_add_action(proto_obj, "purchase_album", "Purchase", NULL, 0);


    unity_protocol_payment_preview_set_header(UNITY_PROTOCOL_PAYMENT_PREVIEW(proto_obj.RawPtr()), header.c_str());
    unity_protocol_payment_preview_set_email(UNITY_PROTOCOL_PAYMENT_PREVIEW(proto_obj.RawPtr()), email.c_str());
    unity_protocol_payment_preview_set_payment_method(UNITY_PROTOCOL_PAYMENT_PREVIEW(proto_obj.RawPtr()), payment_method.c_str());
    unity_protocol_payment_preview_set_purchase_prize(UNITY_PROTOCOL_PAYMENT_PREVIEW(proto_obj.RawPtr()), purchase_prize.c_str());
    unity_protocol_payment_preview_set_purchase_type(UNITY_PROTOCOL_PAYMENT_PREVIEW(proto_obj.RawPtr()), purchase_type.c_str());
    unity_protocol_payment_preview_set_preview_type(UNITY_PROTOCOL_PAYMENT_PREVIEW(proto_obj.RawPtr()), preview_type);

    glib::Variant v(dee_serializable_serialize(DEE_SERIALIZABLE(proto_obj.RawPtr())), glib::StealRef());

    preview_model = dash::Preview::PreviewForVariant(v);
  }

  nux::ObjectPtr<nux::BaseWindow> parent_window_;
  dash::Preview::Ptr preview_model;

  // testing data
  std::string title;
  std::string subtitle;
  std::string header;
  std::string email;
  std::string payment_method;
  std::string purchase_prize;
  std::string purchase_type;
  UnityProtocolPreviewPaymentType preview_type;

  // needed for styles
  unity::Settings settings;
  dash::Style dash_style;

};

TEST_F(TestMusicPaymentPreview, TestContentLoading)
{
  MockedMusicPaymentPreview::Ptr preview_view(new MockedMusicPaymentPreview(preview_model));

  EXPECT_EQ(preview_view->title_->GetText(), title);
  EXPECT_EQ(preview_view->subtitle_->GetText(), subtitle);
  EXPECT_EQ(preview_view->intro_->GetText(), header);
  EXPECT_EQ(preview_view->email_->GetText(), email);
  EXPECT_EQ(preview_view->payment_->GetText(), payment_method);
  EXPECT_EQ(preview_view->purchase_type_->GetText(), purchase_type);
  EXPECT_EQ(preview_view->purchase_prize_->GetText(), purchase_prize);
}


} // previews

} // dash

} // unity
