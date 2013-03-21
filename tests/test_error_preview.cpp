/*
 * Copyright 2012-2013 Canonical Ltd.
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
 * Authored by: Diego Sarmentero <diego.sarmentero@canonical.com>
 *
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
#include "unity-shared/UnitySettings.h"

#include <unity-protocol.h>
#include "dash/previews/ErrorPreview.h"
#include "test_utils.h"

namespace unity
{

namespace dash
{

namespace previews
{

class ErrorPreviewMock : public ErrorPreview
{
  public:
    ErrorPreviewMock(dash::Preview::Ptr preview_model)
      : ErrorPreview(preview_model){}
    ~ErrorPreviewMock(){}

  using ErrorPreview::intro_;
  using ErrorPreview::title_;
  using ErrorPreview::subtitle_;
  using ErrorPreview::purchase_hint_;
  using ErrorPreview::purchase_prize_;
  using ErrorPreview::purchase_type_;
};

class TestErrorPreview : public Test
{
  protected:
    TestErrorPreview() :
      Test(),
      parent_window_(new nux::BaseWindow("TestErrorPayment"))
    {
        title = "Turning Japanese";
        subtitle = "The vapors";
        header = "Hi test, you purchased in the past from Ubuntu One.";
        purchase_prize = "65$";
        purchase_type = "Mp3";
        preview_type = UNITY_PROTOCOL_PREVIEW_PAYMENT_TYPE_ERROR;  

        glib::Object<UnityProtocolPreview> proto_obj(UNITY_PROTOCOL_PREVIEW(unity_protocol_payment_preview_new()));

        unity_protocol_preview_set_title(proto_obj, title.c_str());
        unity_protocol_preview_set_subtitle(proto_obj, subtitle.c_str());

        unity_protocol_payment_preview_set_header(UNITY_PROTOCOL_PAYMENT_PREVIEW(proto_obj.RawPtr()), header.c_str());
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
  std::string purchase_prize;
  std::string purchase_type;
  UnityProtocolPreviewPaymentType preview_type;

  // needed for styles
  unity::Settings settings;
  dash::Style dash_style;
};

} // previews

} // dash

} // unity
