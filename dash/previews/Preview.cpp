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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *
 */

#include "Preview.h"
#include "unity-shared/IntrospectableWrappers.h"
#include <NuxCore/Logger.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include "ActionButton.h"

 #include "GenericPreview.h"
 #include "ApplicationPreview.h"
 #include "MusicPreview.h"
 #include "MoviePreview.h"
 //#include "SeriesPreview.h"

namespace unity
{
namespace dash
{
namespace previews
{

namespace
{
nux::logging::Logger logger("unity.dash.previews.preview");
}

previews::Preview::Ptr Preview::PreviewForModel(dash::Preview::Ptr model)
{
  if (!model)
  {
    LOG_WARN(logger) << "Unable to create Preview object";
    return previews::Preview::Ptr();
  }
 
  if (model->renderer_name == "preview-generic")
  {
    return Preview::Ptr(new GenericPreview(model));
  }
  else if (model->renderer_name == "preview-application")
  {
    return Preview::Ptr(new ApplicationPreview(model));
  }
  else if (model->renderer_name == "preview-music")
  {
    return Preview::Ptr(new MusicPreview(model));
  }
  else if (model->renderer_name == "preview-movie")
  {
    return Preview::Ptr(new MoviePreview(model));
  }
  // else if (renderer_name == "preview-series")
  // {
  //   return Preview::Ptr(new SeriesPreview(model));
  // }
  else
  {
    LOG_WARN(logger) << "Unable to create Preview for renderer: " << model->renderer_name.Get() << "; using generic";
  }

  return Preview::Ptr(new GenericPreview(model));
}

NUX_IMPLEMENT_OBJECT_TYPE(Preview);

Preview::Preview(dash::Preview::Ptr preview_model)
  : View(NUX_TRACKER_LOCATION)
  , preview_model_(preview_model)
{
}

Preview::~Preview()
{
}

std::string Preview::GetName() const
{
  return "Preview";
}

void Preview::AddProperties(GVariantBuilder* builder)
{
}

void Preview::OnActionActivated(ActionButton* button, std::string const& id)
{
  if (preview_model_)
    preview_model_->PerformAction(id);
}

nux::Layout* Preview::BuildGridActionsLayout(dash::Preview::ActionPtrList actions, std::list<nux::AbstractButton*>& buttons)
{
  previews::Style& style = dash::previews::Style::Instance();

  nux::VLayout* actions_buffer_v = new nux::VLayout();
  actions_buffer_v->AddSpace(0, 1);
  nux::VLayout* actions_layout_v = new nux::VLayout();
  actions_layout_v->SetSpaceBetweenChildren(style.GetSpaceBetweenActions());

  uint rows = actions.size() / 2 + ((actions.size() % 2 > 0) ? 1 : 0);
  uint action_iter = 0;
  for (uint i = 0; i < rows; i++)
  {
    nux::HLayout* actions_buffer_h = new nux::HLayout();
    actions_buffer_h->AddSpace(0, 1);
    nux::HLayout* actions_layout_h = new nux::HLayout();
    actions_layout_h->SetSpaceBetweenChildren(style.GetSpaceBetweenActions());
 
    for (uint j = 0; j < 2 && action_iter < actions.size(); j++, action_iter++)
    {
        dash::Preview::ActionPtr action = actions[action_iter];

        ActionButton* button = new ActionButton(action->display_name, action->icon_hint, NUX_TRACKER_LOCATION);
        button->click.connect(sigc::bind(sigc::mem_fun(this, &Preview::OnActionActivated), action->id));
        buttons.push_back(button);

        actions_layout_h->AddView(button, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL, 100.0f, nux::NUX_LAYOUT_BEGIN);
    }
    actions_buffer_h->AddLayout(actions_layout_h, 0);
    actions_layout_v->AddLayout(actions_buffer_h, 0, nux::MINOR_POSITION_TOP, nux::MINOR_SIZE_FULL, 100.0f, nux::NUX_LAYOUT_BEGIN);
  }

  actions_buffer_v->AddLayout(actions_layout_v, 0);
  return actions_buffer_v;
}

nux::Layout* Preview::BuildVerticalActionsLayout(dash::Preview::ActionPtrList actions, std::list<nux::AbstractButton*>& buttons)
{
  previews::Style& style = dash::previews::Style::Instance();

  nux::HLayout* actions_buffer_h = new nux::HLayout();
  actions_buffer_h->AddSpace(0, 1);

  nux::VLayout* actions_buffer_v = new nux::VLayout();
  actions_buffer_v->AddSpace(0, 1);
  nux::VLayout* actions_layout_v = new nux::VLayout();
  actions_layout_v->SetSpaceBetweenChildren(style.GetSpaceBetweenActions());

  uint action_iter = 0;
  for (uint i = 0; i < actions.size(); i++)
  {
      dash::Preview::ActionPtr action = actions[action_iter];

      ActionButton* button = new ActionButton(action->display_name, action->icon_hint, NUX_TRACKER_LOCATION);
      button->click.connect(sigc::bind(sigc::mem_fun(this, &Preview::OnActionActivated), action->id));
      buttons.push_back(button);

      actions_layout_v->AddView(button, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL, 100.0f, nux::NUX_LAYOUT_BEGIN);
  }
  actions_buffer_v->AddLayout(actions_layout_v, 0);
  actions_buffer_h->AddLayout(actions_buffer_v, 0);

  return actions_buffer_h;
}

}
}
}
