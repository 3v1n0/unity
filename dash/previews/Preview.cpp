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
#include "unity-shared/CoverArt.h"
#include <NuxCore/Logger.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include "ActionButton.h"

#include "GenericPreview.h"
#include "ApplicationPreview.h"
#include "MusicPreview.h"
#include "MoviePreview.h"
#include "SocialPreview.h"

namespace unity
{
namespace dash
{
namespace previews
{
DECLARE_LOGGER(logger, "unity.dash.preview.view");

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
  else if (model->renderer_name == "preview-social")
  {
    return Preview::Ptr(new SocialPreview(model));
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

class TabIterator
{
public:
  TabIterator() {}

  void AddArea(nux::InputArea* area)
  {
    areas_.push_back(area);
  }

  std::list<nux::InputArea*> const& GetTabAreas() const { return areas_; }

  nux::InputArea* DefaultFocus() const
  {
    if (areas_.empty())
      return nullptr;

    return *areas_.begin();
  }

  nux::InputArea* FindKeyFocusArea(unsigned int key_symbol,
                                      unsigned long x11_key_code,
                                      unsigned long special_keys_state)
  {
    if (areas_.empty())
      return nullptr;

    nux::InputArea* current_focus_area = nux::GetWindowCompositor().GetKeyFocusArea();
    auto it = std::find(areas_.begin(), areas_.end(), current_focus_area);
    if (it != areas_.end())
      return current_focus_area;

    return *areas_.begin();
  }

  nux::Area* KeyNavIteration(nux::KeyNavDirection direction)
  {
    if (areas_.empty())
      return nullptr;

    if (direction != nux::KEY_NAV_TAB_PREVIOUS && direction != nux::KEY_NAV_TAB_NEXT)
    {
      return nullptr;
    }

    nux::InputArea* current_focus_area = nux::GetWindowCompositor().GetKeyFocusArea();

    if (current_focus_area)
    {
      auto it = std::find(areas_.begin(), areas_.end(), current_focus_area);
      if (direction == nux::KEY_NAV_TAB_PREVIOUS)
      {
        if (it == areas_.begin())
            return *areas_.end();
        else
        {
          it--; 
          if (it == areas_.begin())
            return *areas_.end();
          return *it;
        }
      }
      else if (direction == nux::KEY_NAV_TAB_NEXT)
      {
        if (it == areas_.end())
        {
          return *areas_.begin();
        }
        else
        {
          it++; 
          if (it == areas_.end())
          {
            return *areas_.begin();
          }
          return *it;
        }
      }
    }
    else
    {
      if (direction == nux::KEY_NAV_TAB_PREVIOUS)
      {
        return *areas_.end();
      }
      else if (direction == nux::KEY_NAV_TAB_NEXT)
      {
        return *areas_.begin();
      }
    }

    return nullptr;
  }

  std::list<nux::InputArea*> areas_;
};

class TabIteratorHLayout  : public nux::HLayout
{
public:
  TabIteratorHLayout(TabIterator* iterator)
  :tab_iterator_(iterator)
  {
  }

  nux::Area* KeyNavIteration(nux::KeyNavDirection direction)
  {
    return tab_iterator_->KeyNavIteration(direction);
  }

private:
  TabIterator* tab_iterator_;
};

class TabIteratorVLayout  : public nux::VLayout
{
public:
  TabIteratorVLayout(TabIterator* iterator)
  :tab_iterator_(iterator)
  {
  }

  nux::Area* KeyNavIteration(nux::KeyNavDirection direction)
  {
    return tab_iterator_->KeyNavIteration(direction);
  }

private:
  TabIterator* tab_iterator_;
};

Preview::Preview(dash::Preview::Ptr preview_model)
  : View(NUX_TRACKER_LOCATION)
  , preview_model_(preview_model)
  , tab_iterator_(new TabIterator())
{
}

Preview::~Preview()
{
  if (preview_model_)
    preview_model_->EmitClosed();

  delete tab_iterator_;
}

std::string Preview::GetName() const
{
  return "Preview";
}

void Preview::AddProperties(GVariantBuilder* builder)
{
  variant::BuilderWrapper(builder)
    .add(GetAbsoluteGeometry())
    .add("uri", preview_model_->preview_uri.Get());
}

void Preview::OnActionActivated(ActionButton* button, std::string const& id)
{
  if (preview_model_)
    preview_model_->PerformAction(id);
}

nux::Layout* Preview::BuildGridActionsLayout(dash::Preview::ActionPtrList actions, std::list<nux::AbstractButton*>& buttons)
{
  previews::Style& style = dash::previews::Style::Instance();

  nux::VLayout* actions_layout_v = new nux::VLayout();
  actions_layout_v->SetSpaceBetweenChildren(style.GetSpaceBetweenActions());
  uint rows = actions.size() / 2 + ((actions.size() % 2 > 0) ? 1 : 0);
  uint action_iter = 0;
  for (uint i = 0; i < rows; i++)
  {
    nux::HLayout* actions_layout_h = new TabIteratorHLayout(tab_iterator_);
    actions_layout_h->SetSpaceBetweenChildren(style.GetSpaceBetweenActions());
 
    for (uint j = 0; j < 2 && action_iter < actions.size(); j++, action_iter++)
    {
        dash::Preview::ActionPtr action = actions[action_iter];

        ActionButton* button = new ActionButton(action->id, action->display_name, action->icon_hint, NUX_TRACKER_LOCATION);
        tab_iterator_->AddArea(button);
        AddChild(button);
        button->SetFont(style.action_font());
        button->SetExtraHint(action->extra_text, style.action_extra_font());
        button->activate.connect(sigc::mem_fun(this, &Preview::OnActionActivated));
        buttons.push_back(button);

        actions_layout_h->AddView(button, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL, 100.0f, nux::NUX_LAYOUT_BEGIN);
    }

    actions_layout_v->AddLayout(actions_layout_h, 0, nux::MINOR_POSITION_RIGHT, nux::MINOR_SIZE_FULL, 100.0f, nux::NUX_LAYOUT_BEGIN);
  }

  return actions_layout_v;
}

nux::Layout* Preview::BuildVerticalActionsLayout(dash::Preview::ActionPtrList actions, std::list<nux::AbstractButton*>& buttons)
{
  previews::Style& style = dash::previews::Style::Instance();

  nux::VLayout* actions_layout_v = new TabIteratorVLayout(tab_iterator_);
  actions_layout_v->SetSpaceBetweenChildren(style.GetSpaceBetweenActions());

  uint action_iter = 0;
  for (uint i = 0; i < actions.size(); i++)
  {
      dash::Preview::ActionPtr action = actions[action_iter++];

      ActionButton* button = new ActionButton(action->id, action->display_name, action->icon_hint, NUX_TRACKER_LOCATION);
      tab_iterator_->AddArea(button);
      AddChild(button);
      button->SetFont(style.action_font());
      button->SetExtraHint(action->extra_text, style.action_extra_font());
      button->activate.connect(sigc::mem_fun(this, &Preview::OnActionActivated));
      buttons.push_back(button);

      actions_layout_v->AddView(button, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL, 100.0f, nux::NUX_LAYOUT_BEGIN);
  }

  return actions_layout_v;
}

void Preview::UpdateCoverArtImage(CoverArt* cover_art)
{
  if (!preview_model_)
    return;
  
  previews::Style& style = dash::previews::Style::Instance();

  std::string image_hint;
  if (preview_model_->image.Get())
  {
    glib::String tmp_icon(g_icon_to_string(preview_model_->image.Get()));
    image_hint = tmp_icon.Str();
  }
  if (!image_hint.empty())
    cover_art->SetImage(image_hint);
  else if (!preview_model_->image_source_uri.Get().empty())
    cover_art->GenerateImage(preview_model_->image_source_uri);
  else
    cover_art->SetNoImageAvailable();
  cover_art->SetFont(style.no_preview_image_font());
  
  cover_art->mouse_click.connect([this] (int x, int y, unsigned long button_flags, unsigned long key_flags) 
  {
    if (nux::GetEventButton(button_flags) == nux::MOUSE_BUTTON1 || nux::GetEventButton(button_flags) == nux::MOUSE_BUTTON3)
    {
      request_close.emit();
    }
  });
}

nux::Area* Preview::FindKeyFocusArea(unsigned int key_symbol,
                                      unsigned long x11_key_code,
                                      unsigned long special_keys_state)
{
  nux::Area* tab_area = tab_iterator_->FindKeyFocusArea(key_symbol, x11_key_code, special_keys_state);
  if (tab_area)
  {
    return tab_area;
  }

  return nullptr;
}

nux::Area* Preview::KeyNavIteration(nux::KeyNavDirection direction)
{
  return tab_iterator_->KeyNavIteration(direction);
}

void Preview::OnNavigateIn()
{
  nux::InputArea* default_focus = tab_iterator_->DefaultFocus();
  if (default_focus)
    nux::GetWindowCompositor().SetKeyFocusArea(default_focus);
}

}
}
}
