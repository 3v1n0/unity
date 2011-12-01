/*
 * Copyright (C) 2010 Canonical Ltd
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
 * Authored by: Gord Allott <gord.allott@canonical.com>
 */

#include "HudView.h"

#include <math.h>

#include <gio/gdesktopappinfo.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <Nux/Button.h>
#include <iostream>
#include <sstream>

#include <NuxCore/Logger.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/RadioOptionFilter.h>
#include <Nux/HLayout.h>
#include <Nux/LayeredLayout.h>

#include <NuxCore/Logger.h>
#include "HudButton.h"
#include "UBusMessages.h"

namespace unity
{
namespace hud
{

namespace
{
nux::logging::Logger logger("unity.hud.view");
}

NUX_IMPLEMENT_OBJECT_TYPE(View);

View::View()
  : nux::View(NUX_TRACKER_LOCATION)
  , button_views_(NULL)
{

  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  bg_layer_ = new nux::ColorLayer(nux::Color(0.0f, 0.0f, 0.0f, 0.9), true, rop);

  SetupViews();
  search_bar_->key_down.connect (sigc::mem_fun (this, &View::OnKeyDown));

  search_bar_->activated.connect ([&]() {
    search_activated.emit(search_bar_->search_string);
  });

}

View::~View()
{
}

void View::ResetToDefault()
{
  search_bar_->search_string = "";
}

void View::Relayout()
{
  nux::Geometry geo = GetGeometry();
  content_geo_ = GetBestFitGeometry(geo);
  layout_->SetMinMaxSize(content_geo_.width, content_geo_.height);

  QueueDraw();
}

nux::View* View::default_focus() const
{
  return search_bar_->text_entry();
}

void View::SetSuggestions(Hud::Suggestions suggestions)
{
  suggestions_ = suggestions;
  button_views_->Clear();
  int found_items = 0;
  for (auto suggestion = suggestions.begin(); suggestion != suggestions.end(); suggestion++)
  {
    if (found_items > 5)
      break;

    HudButton *button = new HudButton();
    button->SetSuggestion(*suggestion);
    button_views_->AddView(button, 0, nux::MINOR_POSITION_LEFT);

    button->click.connect([&](nux::View* view) {
      search_activated.emit(dynamic_cast<HudButton*>(view)->label);
    });

    found_items++;
  }
}

void View::SetIcon(std::string icon_name)
{
  icon_->SetByIconName(icon_name.c_str(), 64);
}

// Gives us the width and height of the contents that will give us the best "fit",
// which means that the icons/views will not have uneccessary padding, everything will
// look tight
nux::Geometry View::GetBestFitGeometry(nux::Geometry const& for_geo)
{
  return nux::Geometry(0, 0, 940, 240);
}


void View::SetupViews()
{
  layout_ = new nux::HLayout();

  icon_ = new unity::IconTexture("", 64, 64);
  icon_->SetBaseSize(64, 64);
  icon_->SetMinMaxSize(64, 64);
  layout_->AddView(icon_, 0, nux::MINOR_POSITION_TOP, nux::MINOR_SIZE_MATCHCONTENT);

  content_layout_ = new nux::VLayout();
  layout_->AddLayout(content_layout_, 1, nux::MINOR_POSITION_TOP);
  SetLayout(layout_);

  nux::LayeredLayout* search_composite = new nux::LayeredLayout();
  search_composite->SetPaintAll(true);

  // add the search bar to the composite
  search_bar_ = new unity::hud::SearchBar();
  search_bar_->search_hint = "Type your command";
  search_bar_->search_changed.connect(sigc::mem_fun(this, &View::OnSearchChanged));

  // add the search composite to the overall layout
  search_composite->SetMinimumHeight(50);
  content_layout_->AddView(search_bar_, 0, nux::MINOR_POSITION_LEFT);

  button_views_ = new nux::VLayout();
  button_views_->SetHorizontalExternalMargin(12);
  content_layout_->AddLayout(button_views_, 1, nux::MINOR_POSITION_LEFT);
}

void View::OnSearchChanged(std::string const& search_string)
{
  LOG_DEBUG(logger) << "got search change";
  search_changed.emit(search_string);
  search_bar_->search_hint = "";
}


void View::OnKeyDown (unsigned long event_type, unsigned long keysym,
                      unsigned long event_state, const TCHAR* character,
                      unsigned short key_repeat_count)

//bool View::InspectKeyEvent(unsigned int eventType, unsigned int keysym, const char* character)
{
  if (keysym == NUX_VK_ESCAPE)
  {
    LOG_DEBUG(logger) << "got escape key";
    ubus.SendMessage(UBUS_HUD_CLOSE_REQUEST);
  }
}

void View::Draw(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  // set up our texture mode
  //~ nux::TexCoordXForm texxform;
  //~ texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  //~ texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

  // clear what is behind us
  //~ nux::t_u32 alpha = 0, src = 0, dest = 0;
  //~ gfx_context.GetRenderStates().GetBlend(alpha, src, dest);
  //~ gfx_context.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
//~
  //~ nux::Color col = nux::color::Black;
  //~ col.alpha = 0.75;
  //~ gfx_context.QRP_Color(GetGeometry().x,
                       //~ GetGeometry().y,
                       //~ GetGeometry().width,
                       //~ GetGeometry().height,
                       //~ col);
  bg_layer_->SetGeometry(GetGeometry());
  nux::GetPainter().RenderSinglePaintLayer(gfx_context, content_geo_, bg_layer_);
}

void View::DrawContent(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  int bgs = 0;

  gfx_context.PushClippingRectangle(GetGeometry());

  nux::GetPainter().PushLayer(gfx_context, bg_layer_->GetGeometry(), bg_layer_);
  bgs++;

  if (IsFullRedraw())
  {
    nux::GetPainter().PushBackgroundStack();
    layout_->ProcessDraw(gfx_context, force_draw);
    nux::GetPainter().PopBackgroundStack();
  }
  else
  {
    layout_->ProcessDraw(gfx_context, force_draw);
  }

  nux::GetPainter().PopBackground(bgs);

  gfx_context.GetRenderStates().SetBlend(false);
  gfx_context.PopClippingRectangle();
}

// Keyboard navigation
bool View::AcceptKeyNavFocus()
{
  return false;
}

// Introspectable
const gchar* View::GetName()
{
  return "unity.hud.View";
}

void View::AddProperties(GVariantBuilder* builder)
{}

nux::Area* View::FindKeyFocusArea(unsigned int key_symbol,
      unsigned long x11_key_code,
      unsigned long special_keys_state)
{
  // Do what nux::View does, but if the event isn't a key navigation,
  // designate the text entry to process it.

  nux::KeyNavDirection direction = nux::KEY_NAV_NONE;
  switch (x11_key_code)
  {
  case NUX_VK_UP:
    direction = nux::KEY_NAV_UP;
    break;
  case NUX_VK_DOWN:
    direction = nux::KEY_NAV_DOWN;
    break;
  case NUX_VK_LEFT:
    direction = nux::KEY_NAV_LEFT;
    break;
  case NUX_VK_RIGHT:
    direction = nux::KEY_NAV_RIGHT;
    break;
  case NUX_VK_LEFT_TAB:
    direction = nux::KEY_NAV_TAB_PREVIOUS;
    break;
  case NUX_VK_TAB:
    direction = nux::KEY_NAV_TAB_NEXT;
    break;
  case NUX_VK_ENTER:
  case NUX_KP_ENTER:
    // Not sure if Enter should be a navigation key
    direction = nux::KEY_NAV_ENTER;
    break;
  default:
    direction = nux::KEY_NAV_NONE;
    break;
  }

  if (has_key_focus_)
  {
    return this;
  }
  else if (direction == nux::KEY_NAV_NONE)
  {
    // then send the event to the search entry
    return search_bar_->text_entry();
  }
  else if (next_object_to_key_focus_area_)
  {
    return next_object_to_key_focus_area_->FindKeyFocusArea(key_symbol, x11_key_code, special_keys_state);
  }
  return NULL;
}

}
}
