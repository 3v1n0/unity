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

#include "PreviewGeneric.h"

namespace unity {

    PreviewGeneric::PreviewGeneric (dash::Preview::Ptr preview, NUX_FILE_LINE_DECL)
      : PreviewBase(NUX_FILE_LINE_PARAM)
    {
      SetPreview (preview);
    }

    PreviewGeneric::~PreviewGeneric ()
    {
      //ROOOARRRR
    }

    void PreviewGeneric::SetPreview(dash::Preview::Ptr preview)
    {
      preview_ = std::static_pointer_cast<dash::GenericPreview>(preview);
      BuildLayout ();
    }

    void PreviewGeneric::BuildLayout()
    {
      IconTexture *icon = new IconTexture (preview_->icon_hint.c_str(), 300);
      nux::StaticCairoText *name = new nux::StaticCairoText (preview_->name.c_str(), NUX_TRACKER_LOCATION);
      name->SetFont("Ubuntu 25");

      //get date
      GDate *date = g_date_new_julian(preview_->date_modified);
      gchar string_buff[256];
      g_date_strftime(string_buff, 256, "%d %b %Y %H:%M", date);
      g_free (date);

      nux::StaticCairoText *date_modified = new nux::StaticCairoText(string_buff, NUX_TRACKER_LOCATION);

      // format the bytes
      gchar *formatted_size = g_format_size(preview_->size);
      std::stringstream size_and_type_str;
      size_and_type_str << formatted_size << ", " << preview_->type;

      nux::StaticCairoText *size_and_type = new nux::StaticCairoText(size_and_type_str.str().c_str(), NUX_TRACKER_LOCATION);
      g_free (formatted_size);

      nux::StaticCairoText *description = new nux::StaticCairoText ("", NUX_TRACKER_LOCATION);
      description->SetBaseWidth(400);
      description->SetMaximumWidth(400);
      description->SetLines(99999999);
      description->SetText(preview_->description.c_str());

      nux::HLayout *large_container = new nux::HLayout(NUX_TRACKER_LOCATION);
      nux::VLayout *screenshot_container = new nux::VLayout(NUX_TRACKER_LOCATION);
      nux::VLayout *content_container = new nux::VLayout(NUX_TRACKER_LOCATION);
      nux::HLayout *button_container = new nux::HLayout(NUX_TRACKER_LOCATION);

      // create the action buttons
      if (preview_->tertiary_action_name.empty() == false)
      {
        PreviewBasicButton* tertiary_button = new PreviewBasicButton(preview_->tertiary_action_name.c_str(), NUX_TRACKER_LOCATION);
        tertiary_button->state_change.connect ([&] (nux::View *view) { UriActivated.emit (preview_->tertiary_action_uri); });
        button_container->AddLayout (new nux::SpaceLayout(6,6,6,6), 0);
        button_container->AddView (tertiary_button, 1);
      }

      if (preview_->secondary_action_name.empty() == false)
      {
        PreviewBasicButton* secondary_button = new PreviewBasicButton(preview_->secondary_action_name.c_str(), NUX_TRACKER_LOCATION);
        secondary_button->state_change.connect ([&] (nux::View *view) { UriActivated.emit (preview_->secondary_action_uri); });
        button_container->AddLayout (new nux::SpaceLayout(6,6,6,6), 0);
        button_container->AddView (secondary_button, 1);
      }

      if (preview_->primary_action_name.empty() == false)
      {
        PreviewBasicButton* primary_button = new PreviewBasicButton(preview_->primary_action_name.c_str(), NUX_TRACKER_LOCATION);
        primary_button->state_change.connect ([&] (nux::View *view) { UriActivated.emit (preview_->primary_action_uri); });
        button_container->AddLayout (new nux::SpaceLayout(6,6,6,6), 0);
        button_container->AddView (primary_button, 1);
      }

      button_container->AddLayout (new nux::SpaceLayout(6,6,6,6), 0);

      // create the description
      nux::VLayout *description_container = new nux::VLayout (NUX_TRACKER_LOCATION);
      description_container->AddView (description, 1, nux::MINOR_POSITION_LEFT);
      nux::ScrollView* description_scroller = new nux::ScrollView (NUX_TRACKER_LOCATION);
      description_scroller->SetLayout(description_container);

      // create the title containers
      content_container->AddView (name, 0, nux::MINOR_POSITION_LEFT);
      content_container->AddLayout (new nux::SpaceLayout(12,12,12,12), 0);
      content_container->AddView (date_modified, 0, nux::MINOR_POSITION_LEFT);
      content_container->AddLayout(new nux::SpaceLayout(12, 12, 12,12), 0);
      content_container->AddView (size_and_type, 0, nux::MINOR_POSITION_LEFT);

      content_container->AddView (description_scroller, 1, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);
      content_container->AddLayout (button_container, 0);

      // build the overall container
      screenshot_container->AddView (icon, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
      large_container->AddLayout(screenshot_container, 1);
      large_container->AddLayout (new nux::SpaceLayout(12,12,12,12), 0);
      large_container->AddLayout(content_container, 1);
      large_container->AddLayout (new nux::SpaceLayout(6,6,6,6), 0);

      SetLayout(large_container);
    }

  void PreviewGeneric::Draw (nux::GraphicsEngine &GfxContext, bool force_draw) {
  }

  void PreviewGeneric::DrawContent (nux::GraphicsEngine &GfxContent, bool force_draw) {
    nux::Geometry base = GetGeometry ();
    GfxContent.PushClippingRectangle (base);

    if (GetCompositionLayout ())
      GetCompositionLayout ()->ProcessDraw (GfxContent, force_draw);

    GfxContent.PopClippingRectangle();
  }

  long int PreviewGeneric::ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo) {
    return PreviewBase::ProcessEvent(ievent, TraverseInfo, ProcessEventInfo);
  }

  void PreviewGeneric::PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    PreviewBase::PostDraw(GfxContext, force_draw);
  }
}
