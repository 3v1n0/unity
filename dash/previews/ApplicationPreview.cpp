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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

#include "ApplicationPreview.h"
#include "ActionButton.h"
#include "unity-shared/IntrospectableWrappers.h"
#include "unity-shared/PreviewStyle.h"
#include <NuxCore/Logger.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include <Nux/Button.h>
#include <PreviewFactory.h>


namespace unity
{
namespace dash
{
namespace previews
{

#define TMP_VIEW(name, colour) \
class TmpView_##name : public nux::View \
{ \
public: \
  TmpView_##name():View(NUX_TRACKER_LOCATION) {} \
  ~TmpView_##name() {} \
  virtual void Draw(nux::GraphicsEngine& gfx_engine, bool force_draw) { \
    gPainter.Paint2DQuadColor(gfx_engine, GetGeometry(), colour); \
  } \
  virtual void DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw) {} \
};

TMP_VIEW(AppImage, nux::Color(0xAA, 0x00, 0x00))
TMP_VIEW(AppIcon, nux::Color(0x00, 0x00, 0xAA))
TMP_VIEW(UserRating, nux::Color(0x00, 0xAA, 0x00))
TMP_VIEW(Actions, nux::Color(0x00, 0xAA, 0xAA))

namespace
{
nux::logging::Logger logger("unity.dash.previews.applicationpreview");

}

NUX_IMPLEMENT_OBJECT_TYPE(ApplicationPreview);

ApplicationPreview::ApplicationPreview(dash::Preview::Ptr preview_model)
: Preview(preview_model)
{
  SetupViews();
}

ApplicationPreview::~ApplicationPreview()
{
}

void ApplicationPreview::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  Preview::Draw(gfx_engine, force_draw);  
}

void ApplicationPreview::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  Preview::DrawContent(gfx_engine, force_draw);
}

std::string ApplicationPreview::GetName() const
{
  return "ApplicationPreview";
}

void ApplicationPreview::AddProperties(GVariantBuilder* builder)
{
  Preview::AddProperties(builder);
}


void ApplicationPreview::SetupViews()
{
  previews::Style& style = dash::previews::Style::Instance();

  nux::HLayout* image_data_layout = new nux::HLayout();

  app_image_ = new TmpView_AppImage();

    /////////////////////
    // App Data Panel
    nux::VLayout* full_data_layout = new nux::VLayout();
    full_data_layout->SetPadding(10);
    full_data_layout->SetSpaceBetweenChildren(12);

      /////////////////////
      // Main App Info
      nux::HLayout* main_app_info = new nux::HLayout();
      main_app_info->SetSpaceBetweenChildren(16);

        /////////////////////
        // Icon Layout
        nux::VLayout* icon_layout = new nux::VLayout();
        app_icon_ = new TmpView_AppIcon();
        app_icon_->SetMinimumSize(128,110);
        app_icon_->SetMaximumSize(128,110);
        icon_layout->AddView(app_icon_, 0);

        app_rating_ = new TmpView_UserRating();
        app_rating_->SetMinimumHeight(36);
        icon_layout->AddView(app_rating_, 0);

        // buffer space
        icon_layout->AddSpace(0,0);      
        /////////////////////

        /////////////////////
        // App Data
        nux::VLayout* app_data_layout = new nux::VLayout();
        app_data_layout->SetSpaceBetweenChildren(16);

        nux::VLayout* app_name_version_layout = new nux::VLayout();
        app_name_version_layout->SetSpaceBetweenChildren(8);

        app_name_ = new nux::StaticCairoText("Skype");
        app_name_->SetFont(style.app_name_font().c_str());

        version_size_ = new nux::StaticCairoText("Version 3.2, Size 32 MB");
        version_size_->SetFont(style.version_size_font().c_str());

        nux::VLayout* app_updated_copywrite_layout = new nux::VLayout();
        app_updated_copywrite_layout->SetSpaceBetweenChildren(8);

        last_update_ = new nux::StaticCairoText("Last updated");
        last_update_->SetFont(style.app_last_update_font().c_str());

        copywrite_ = new nux::StaticCairoText("Copywrite");
        copywrite_->SetFont(style.app_copywrite_font().c_str());

        app_name_version_layout->AddView(app_name_, 1);
        app_name_version_layout->AddView(version_size_, 1);
        app_updated_copywrite_layout->AddView(last_update_, 1);
        app_updated_copywrite_layout->AddView(copywrite_, 1);

        app_data_layout->AddLayout(app_name_version_layout);
        app_data_layout->AddLayout(app_updated_copywrite_layout);

        // buffer space
        app_data_layout->AddSpace(0,0);      
        /////////////////////

      main_app_info->AddLayout(icon_layout, 0);
      main_app_info->AddLayout(app_data_layout, 1);
      /////////////////////

      /////////////////////
      // App Description
      app_description_ = new nux::StaticCairoText("");
      app_description_->SetFont(style.app_description_font().c_str());
      app_description_->SetTextAlignment(nux::StaticCairoText::NUX_ALIGN_TOP);
      app_description_->SetLines(-10);
      app_description_->SetLineSpacing(1);
      app_description_->SetMaximumWidth(400);
      app_description_->SetText("Skype is a proprietary voice-over-Internet Protocol service and software application originally \
created by Niklas Zennström and Janus Friis in 2003, and owned by Microsoft since 2011. \
The service allows users to communicate with peers by voice, video, and instant messaging over the Internet.");
      /////////////////////

      /////////////////////
      // App Actions
      nux::HLayout* actions_layout = new nux::HLayout();
      actions_layout->SetSpaceBetweenChildren(12);
      actions_layout->AddSpace(0, 1);

      for (dash::Preview::ActionPtr action : preview_model_->GetActions())
      {
        actions_layout->AddView(new ActionButton(action->display_name, NUX_TRACKER_LOCATION), 0, nux::MINOR_POSITION_END, nux::MINOR_SIZE_MATCHCONTENT);
      }
      /////////////////////

    full_data_layout->AddLayout(main_app_info, 0);
    full_data_layout->AddView(app_description_, 0);
    full_data_layout->AddLayout(actions_layout, 1);
    /////////////////////
  
  image_data_layout->AddView(app_image_, 1);
  image_data_layout->AddLayout(full_data_layout, 1);

  SetLayout(image_data_layout);
}

}
}
}
