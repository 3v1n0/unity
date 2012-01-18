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
#include "DashStyle.h"

namespace unity
{
namespace hud
{

namespace
{
nux::logging::Logger logger("unity.hud.view");
int icon_size = 42;
}

NUX_IMPLEMENT_OBJECT_TYPE(View);

View::View()
  : nux::View(NUX_TRACKER_LOCATION)
  , button_views_(NULL)
{
  renderer_.SetOwner(this);
  renderer_.need_redraw.connect([this] () { 
    QueueDraw();
  });

  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  bg_layer_ = new nux::ColorLayer(nux::Color(0.0f, 0.0f, 0.0f, 1.0), false, rop);

  SetupViews();
  search_bar_->key_down.connect (sigc::mem_fun (this, &View::OnKeyDown));

  search_bar_->activated.connect ([&]() {
    search_activated.emit(search_bar_->search_string);
  });

  Relayout();

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
  LOG_DEBUG(logger) << "content_geo: " << content_geo_.width << "x" << content_geo_.height;

  layout_->SetMinMaxSize(content_geo_.width, content_geo_.height);

  QueueDraw();
}

long View::PostLayoutManagement(long LayoutResult)
{
  Relayout();
  return LayoutResult;
}


nux::View* View::default_focus() const
{
  return search_bar_->text_entry();
}

void View::SetQueries(Hud::Queries queries)
{
  queries_ = queries_;
  button_views_->Clear();
  int found_items = 0;
  for (auto query = queries.begin(); query != queries.end(); query++)
  {
    if (found_items > 5)
      break;

    HudButton *button = new HudButton();
    button->SetQuery(*query);
    button_views_->AddView(button, 0, nux::MINOR_POSITION_LEFT);

    button->click.connect([&](nux::View* view) {
      query_activated.emit(dynamic_cast<HudButton*>(view)->GetQuery());
    });
    
    button->OnKeyNavFocusActivate.connect([&](nux::Area *area) {
      query_activated.emit(dynamic_cast<HudButton*>(area)->GetQuery());
    });
    
    found_items++;
  }
  
  QueueRelayout();
  QueueDraw();
}

void View::SetIcon(std::string icon_name)
{
  LOG_DEBUG(logger) << "Setting icon to " << icon_name;
  icon_->SetByIconName(icon_name.c_str(), icon_size);
}

// Gives us the width and height of the contents that will give us the best "fit",
// which means that the icons/views will not have uneccessary padding, everything will
// look tight
nux::Geometry View::GetBestFitGeometry(nux::Geometry const& for_geo)
{
  dash::Style& style = dash::Style::Instance();
  int width = 0, height = 0;
  int tile_width = style.GetTileWidth();
  int half = for_geo.width / 2;

  while ((width += tile_width) + (19 * 2) < half)
    ;

  width = MAX(width, tile_width * 6);

  width += 19 + 32;

  height = search_bar_->GetGeometry().height;
  height += 6;
  height += (style.GetTextLineHeight() + 12) * 6;
  height += 6;

  LOG_DEBUG (logger) << "best fit is, " << width << ", " << height;

  return nux::Geometry(0, 0, width, height);
}

void View::AboutToShow()
{
  renderer_.AboutToShow();
}

void View::AboutToHide()
{
  renderer_.AboutToHide();
}

void View::SetWindowGeometry(nux::Geometry const& absolute_geo, nux::Geometry const& geo)
{
  window_geometry_ = geo;
  window_geometry_.x = 0;
  window_geometry_.y = 0;
  absolute_window_geometry_ = absolute_geo;
}

void View::SetupViews()
{
  layout_ = new nux::HLayout();
  layout_->AddLayout(new nux::SpaceLayout(8,8,8,8), 0);
  
  icon_ = new unity::IconTexture("unity", icon_size, icon_size);
  icon_->SetBaseSize(icon_size, icon_size);
  icon_->SetMinMaxSize(icon_size, icon_size);
  
  nux::Layout* icon_layout = new nux::VLayout();
  icon_layout->SetVerticalExternalMargin(12);
  icon_layout->AddView(icon_, 0, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);
  layout_->AddLayout(icon_layout, 0, nux::MINOR_POSITION_TOP, nux::MINOR_SIZE_MATCHCONTENT);

  
  content_layout_ = new nux::VLayout();
  layout_->AddLayout(content_layout_, 1, nux::MINOR_POSITION_TOP);
  SetLayout(layout_);

  // add the search bar to the composite
  search_bar_ = new unity::hud::SearchBar();
  search_bar_->search_hint = "Type your command";
  search_bar_->search_changed.connect(sigc::mem_fun(this, &View::OnSearchChanged));
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
{
  if (keysym == NUX_VK_ESCAPE)
  {
    LOG_DEBUG(logger) << "got escape key";
    ubus.SendMessage(UBUS_HUD_CLOSE_REQUEST);
  }
}

void View::Draw(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  renderer_.DrawFull(gfx_context, layout_->GetGeometry(), absolute_window_geometry_, window_geometry_);
}

void View::DrawContent(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  renderer_.DrawInner(gfx_context, layout_->GetGeometry(), absolute_window_geometry_, window_geometry_);
  
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

  renderer_.DrawInnerCleanup(gfx_context, layout_->GetGeometry(), absolute_window_geometry_, window_geometry_);
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
