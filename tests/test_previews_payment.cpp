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
#include <Nux/Layout.h>
#include <Nux/VLayout.h>
#include <Nux/BaseWindow.h>
#include <unity-shared/StaticCairoText.h>
#include <unity-shared/DashStyle.h>
#include <unity-shared/PreviewStyle.h>
#include <unity-shared/ThumbnailGenerator.h>
#include <unity-shared/CoverArt.h>
#include "unity-shared/UnitySettings.h"

#include <unity-protocol.h>
#include "dash/previews/PaymentPreview.h"
#include "dash/previews/ActionButton.h"
#include "dash/previews/ActionLink.h"
#include "test_utils.h"

using namespace testing;
using ::testing::Return;

namespace unity
{

namespace dash
{

namespace previews
{

class NonAbstractPreview : public PaymentPreview
{
public:
  NonAbstractPreview(dash::Preview::Ptr preview_model)
  : PaymentPreview(preview_model)
  {}

  virtual nux::Layout* GetTitle()
  {
  	return new nux::VLayout();
  }

  virtual nux::Layout* GetPrice()
  {
  	return new nux::VLayout();
  }

  virtual nux::Layout* GetBody()
  {
  	return new nux::VLayout();
  }

  virtual nux::Layout* GetFooter()
  {
  	return new nux::VLayout();
  }

  virtual void OnActionActivated(ActionButton* button, std::string const& id)
  {
  	// do nothing
  }

  virtual void OnActionLinkActivated(ActionLink* link, std::string const& id)
  {
  	// do nothing
  }

  virtual void PreLayoutManagement()
  {
  	// do nothing
  }

  virtual void LoadActions()
  {
  	// do nothing
  }

  using PaymentPreview::GetHeader;
  using PaymentPreview::full_data_layout_;
  using PaymentPreview::content_data_layout_;
  using PaymentPreview::overlay_layout_;
  using PaymentPreview::header_layout_;
  using PaymentPreview::body_layout_;
  using PaymentPreview::footer_layout_;
  using PaymentPreview::SetupViews;

};

class MockedPaymentPreview : public NonAbstractPreview
{
public:
  typedef nux::ObjectPtr<MockedPaymentPreview> Ptr;

  MockedPaymentPreview(dash::Preview::Ptr preview_model)
  : NonAbstractPreview(preview_model)
  {}

  // Mock methods that should be implemented so that we can assert that they are
  // called in the correct moments.
  MOCK_METHOD0(GetTitle, nux::Layout*());
  MOCK_METHOD0(GetPrice, nux::Layout*());
  MOCK_METHOD0(GetBody, nux::Layout*());
  MOCK_METHOD0(GetFooter, nux::Layout*());
  MOCK_METHOD2(OnActionActivated, void(unity::dash::ActionButton*, std::string));
  MOCK_METHOD2(OnActionLinkActivated, void(unity::dash::ActionLink*, std::string));
  MOCK_METHOD0(PreLayoutManagement, void());
  MOCK_METHOD0(LoadActions, void());

};

class TestPaymentPreview : public ::testing::Test
{
  protected:
  TestPaymentPreview() : Test()
  {
    glib::Object<UnityProtocolPreview> proto_obj(UNITY_PROTOCOL_PREVIEW(unity_protocol_payment_preview_new()));
    // we are not testing how the info is really used is more asserting the method calls, we do not add any data then
    glib::Variant v(dee_serializable_serialize(DEE_SERIALIZABLE(proto_obj.RawPtr())), glib::StealRef());

    preview_model = dash::Preview::PreviewForVariant(v);

    preview = new MockedPaymentPreview(preview_model);

  }
  nux::ObjectPtr<MockedPaymentPreview> preview;
  dash::Preview::Ptr preview_model;

  // needed for styles
  unity::Settings settings;
  dash::Style dash_style;

};

TEST_F(TestPaymentPreview, GetHeaderCallsCorrectMethods)
{
  ON_CALL(*preview.GetPointer(), GetTitle()).WillByDefault(Return(new nux::VLayout()));
  EXPECT_CALL(*preview.GetPointer(), GetTitle()).Times(1);

  ON_CALL(*preview.GetPointer(), GetPrice()).WillByDefault(Return(new nux::VLayout()));
  EXPECT_CALL(*preview.GetPointer(), GetPrice()).Times(1);

  preview->GetHeader();
}

TEST_F(TestPaymentPreview, SetupViewsCallCorrectMethods)
{
  ON_CALL(*preview.GetPointer(), GetTitle()).WillByDefault(Return(new nux::VLayout()));
  EXPECT_CALL(*preview.GetPointer(), GetTitle()).Times(1);

  ON_CALL(*preview.GetPointer(), GetPrice()).WillByDefault(Return(new nux::VLayout()));
  EXPECT_CALL(*preview.GetPointer(), GetPrice()).Times(1);

  ON_CALL(*preview.GetPointer(), GetBody()).WillByDefault(Return(new nux::VLayout()));
  EXPECT_CALL(*preview.GetPointer(), GetBody()).Times(1);

  ON_CALL(*preview.GetPointer(), GetFooter()).WillByDefault(Return(new nux::VLayout()));
  EXPECT_CALL(*preview.GetPointer(), GetFooter()).Times(1);

  preview->SetupViews();
}

} // previews

} // dash

} // unity
