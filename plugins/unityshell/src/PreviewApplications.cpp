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
#include "config.h"
#include <Nux/Nux.h>
#include <Nux/StaticText.h>
#include <Nux/VLayout.h>
#include <Nux/HLayout.h>
#include <Nux/ScrollView.h>
#include <sstream>

#include "PreviewBasicButton.h"
#include "PreviewBase.h"
#include "IconTexture.h"
#include "StaticCairoText.h"

#include "PreviewApplications.h"

namespace unity {

    PreviewApplications::PreviewApplications (dash::Preview::Ptr preview, NUX_FILE_LINE_DECL)
      : PreviewBase(NUX_FILE_LINE_PARAM)
    {
      SetPreview (preview);
    }

    PreviewApplications::~PreviewApplications ()
    {
      //ROOOARRRR
    }

    void PreviewApplications::SetPreview(dash::Preview::Ptr preview)
    {
      preview_ = std::static_pointer_cast<dash::ApplicationPreview>(preview);
      BuildLayout ();
    }

    void PreviewApplications::BuildLayout()
    {
      IconTexture *screenshot = new IconTexture (preview_->screenshot_icon_hint.c_str(), 420);
      IconTexture *icon = new IconTexture (preview_->icon_hint.c_str(), 80);
      nux::StaticCairoText *name = new nux::StaticCairoText (preview_->name.c_str(), NUX_TRACKER_LOCATION);
      name->SetFont("Ubuntu 25");

      nux::StaticCairoText *version = new nux::StaticCairoText (preview_->version.c_str(), NUX_TRACKER_LOCATION);
      version->SetFont("Ubuntu 15");

      nux::StaticCairoText *size = new nux::StaticCairoText (preview_->size.c_str(), NUX_TRACKER_LOCATION);
      size->SetFont("Ubuntu 15");

      nux::StaticCairoText *licence = new nux::StaticCairoText (preview_->license.c_str(), NUX_TRACKER_LOCATION);
      nux::StaticCairoText *last_updated = new nux::StaticCairoText (preview_->last_updated.c_str(), NUX_TRACKER_LOCATION);
      description = new nux::StaticCairoText ("", NUX_TRACKER_LOCATION);

      //description->SetBaseWidth(350);
      description->SetMaximumWidth(350);
      description->SetLines(99999999);
      description->SetText(preview_->description.c_str());

      std::ostringstream number_of_reviews;
      number_of_reviews << preview_->n_ratings << " Reviews";
      nux::StaticCairoText *review_total = new nux::StaticCairoText (number_of_reviews.str().c_str(), NUX_TRACKER_LOCATION);

      nux::HLayout *large_container = new nux::HLayout(NUX_TRACKER_LOCATION);
      nux::VLayout *screenshot_container = new nux::VLayout(NUX_TRACKER_LOCATION);
      nux::VLayout *content_container = new nux::VLayout(NUX_TRACKER_LOCATION);
      nux::HLayout *title_icon_container = new nux::HLayout(NUX_TRACKER_LOCATION);
      nux::VLayout *icon_container = new nux::VLayout(NUX_TRACKER_LOCATION);
      nux::VLayout *title_container = new nux::VLayout(NUX_TRACKER_LOCATION);
      nux::HLayout *button_container = new nux::HLayout(NUX_TRACKER_LOCATION);

      // create the action buttons
      PreviewBasicButton* primary_button = new PreviewBasicButton(preview_->primary_action_name.c_str(), NUX_TRACKER_LOCATION);
      //FIXME - add secondary action when we have the backend for it
      primary_button->changed.connect ([&] (nux::View *view) { UriActivated.emit (preview_->primary_action_uri); });
      button_container->AddLayout (new nux::SpaceLayout(6,6,6,6), 0);
      button_container->AddView (primary_button, 1);
      button_container->AddLayout (new nux::SpaceLayout(6,6,6,6), 0);

      // create the description
      nux::VLayout *description_container = new nux::VLayout (NUX_TRACKER_LOCATION);
      description_container->AddView (description, 1, nux::MINOR_POSITION_LEFT);
      nux::ScrollView* description_scroller = new nux::ScrollView (NUX_TRACKER_LOCATION);
      description_scroller->EnableHorizontalScrollBar (false);
      description_scroller->SetLayout(description_container);

      // create the title containers
      title_container->AddView (name, 0, nux::MINOR_POSITION_LEFT);
      title_container->AddLayout (new nux::SpaceLayout(6,6,6,6), 0);

      nux::HLayout* version_size_container = new nux::HLayout (NUX_TRACKER_LOCATION);
      version_size_container->AddView (version, 0, nux::MINOR_POSITION_LEFT);
      version_size_container->AddLayout (new nux::SpaceLayout(6,6,6,6), 0);
      version_size_container->AddView(size, 1, nux::MINOR_POSITION_LEFT);

      title_container->AddLayout (version_size_container, 0, nux::MINOR_POSITION_LEFT);
      title_container->AddLayout (new nux::SpaceLayout(12,12,12,12), 0);
      title_container->AddView (licence, 0, nux::MINOR_POSITION_LEFT);
      title_container->AddLayout (new nux::SpaceLayout(6,6,6,6), 0);
      title_container->AddView (last_updated, 0, nux::MINOR_POSITION_LEFT);

      // create the icon container
      icon_container->AddLayout (new nux::SpaceLayout(6,6,6,6), 0);
      icon_container->AddView (icon, 1, nux::MINOR_POSITION_TOP, nux::MINOR_SIZE_FULL);
      icon_container->AddLayout (new nux::SpaceLayout(12,12,12,12), 0);
      //FIXME - add rating widget
      icon_container->AddView (review_total, 0, nux::MINOR_POSITION_TOP, nux::MINOR_SIZE_MATCHCONTENT);

      // build the content container
      title_icon_container->AddLayout (new nux::SpaceLayout(6,6,6,6), 0);
      title_icon_container->AddLayout (icon_container, 0, nux::MINOR_POSITION_BOTTOM, nux::MINOR_SIZE_FULL);
      title_icon_container->AddLayout (new nux::SpaceLayout(24,24,24,24), 0);
      title_icon_container->AddLayout (title_container, 0, nux::MINOR_POSITION_BOTTOM, nux::MINOR_SIZE_FULL);
      title_icon_container->AddLayout (new nux::SpaceLayout(6,6,6,6), 0);

      content_container->AddLayout (title_icon_container, 0, nux::MINOR_POSITION_TOP, nux::MINOR_SIZE_FULL);
      content_container->AddLayout (new nux::SpaceLayout(24,24,24,24), 0);
      content_container->AddView (description_scroller, 1);
      content_container->AddLayout (new nux::SpaceLayout(24,24,24,24), 0);
      content_container->AddView (button_container, 0);
      content_container->AddLayout (new nux::SpaceLayout(6,6,6,6), 0);

      // build the overall container
      screenshot_container->AddView (screenshot, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
      large_container->AddLayout(screenshot_container, 1);
      large_container->AddLayout (new nux::SpaceLayout(32,32,32,32), 0);
      large_container->AddLayout(content_container, 1);
      large_container->AddLayout (new nux::SpaceLayout(6,6,6,6), 0);

      SetLayout(large_container);
    }

  long PreviewApplications::ComputeLayout2 ()
  {
    return PreviewBase::ComputeLayout2();
    g_debug ("layout recomputing");
    description->SetBaseWidth((GetGeometry().width / 2) - 16 - 12 );
    description->SetMaximumWidth((GetGeometry().width / 2) - 16 - 12 );
    description->SetText(preview_->description.c_str());
  }

  void PreviewApplications::Draw (nux::GraphicsEngine &GfxContext, bool force_draw) {
  }

  void PreviewApplications::DrawContent (nux::GraphicsEngine &GfxContent, bool force_draw) {
    nux::Geometry base = GetGeometry ();
    GfxContent.PushClippingRectangle (base);

    if (GetCompositionLayout ())
      GetCompositionLayout ()->ProcessDraw (GfxContent, force_draw);

    GfxContent.PopClippingRectangle();
  }

  long int PreviewApplications::ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo) {
    return PreviewBase::ProcessEvent(ievent, TraverseInfo, ProcessEventInfo);
  }

  void PreviewApplications::PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    PreviewBase::PostDraw(GfxContext, force_draw);
  }
}
