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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include <glib/gstdio.h>
#include "ScopeBar.h"
#include <NuxCore/Logger.h>
#include "config.h"
#include <Nux/HLayout.h>
#include <Nux/LayeredLayout.h>

#include "unity-shared/DashStyle.h"
#include "unity-shared/StaticCairoText.h"
#include "unity-shared/CairoTexture.h"
#include "unity-shared/GraphicsUtils.h"
#include "unity-shared/UBusMessages.h"

namespace unity
{
namespace dash
{
DECLARE_LOGGER(logger, "unity.dash.scopebar");
namespace
{
// according to Q design the inner area of the scopebar should be 40px
// (without any borders)
const int SCOPEBAR_HEIGHT = 41;

}

NUX_IMPLEMENT_OBJECT_TYPE(ScopeBar);

ScopeBar::ScopeBar()
  : nux::View(NUX_TRACKER_LOCATION)
  , info_previously_shown_(false)
{
  glib::String cachedir(g_strdup(g_get_user_cache_dir())); 
  legal_seen_file_path_ = cachedir.Str() + "/unitydashlegalseen";
  info_previously_shown_ = (g_file_test(legal_seen_file_path_.c_str(), G_FILE_TEST_EXISTS)) ? true : false;

  SetupBackground();
  SetupLayout();
}

void ScopeBar::SetupBackground()
{
  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  bg_layer_.reset(new nux::ColorLayer(nux::Color(0.0f, 0.0f, 0.0f, 0.2f), true, rop));
}

void ScopeBar::SetupLayout()
{
  legal_layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
  std::string legal_text("<span underline='single'>");
  legal_text.append(g_dgettext("credentials-control-center", "Legal notice"));
  legal_text.append("</span>");
  legal_ = new StaticCairoText(legal_text);
  legal_->SetFont("Ubuntu 14px");
  legal_layout_->AddSpace(1, 1);
  legal_layout_->SetLeftAndRightPadding(0, 10);
  info_icon_ = new IconTexture(Style::Instance().GetInformationTexture(), 22, 22); 
  legal_layout_->AddView(info_icon_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);
  legal_layout_->AddView(legal_,  0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);
  
  info_icon_->SetVisible(info_previously_shown_);
  legal_->SetVisible(!info_previously_shown_);
  
  info_icon_->mouse_click.connect([&] (int a, int b, unsigned long c, unsigned long d) 
  {
    DoOpenLegalise();
  });

  legal_->mouse_click.connect([&] (int a, int b, unsigned long c, unsigned long d) 
  {
    info_previously_shown_ = true;

    info_icon_->SetVisible(info_previously_shown_);
    legal_->SetVisible(!info_previously_shown_);

    DoOpenLegalise();
    QueueRelayout();
    QueueDraw();
  });


  layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
  layout_->SetContentDistribution(nux::MAJOR_POSITION_CENTER);
  
  layered_layout_ = new nux::LayeredLayout();
  layered_layout_->AddLayer(layout_);
  layered_layout_->AddLayout(legal_layout_);
  layered_layout_->SetPaintAll(true);
  layered_layout_->SetInputMode(nux::LayeredLayout::InputMode::INPUT_MODE_COMPOSITE);

  SetLayout(layered_layout_);
  
  SetMinimumHeight(SCOPEBAR_HEIGHT);
  SetMaximumHeight(SCOPEBAR_HEIGHT);
}

void ScopeBar::DoOpenLegalise()
{
  glib::Error error;
  std::string legal_file_path = "file://";
  legal_file_path.append(PKGDATADIR);
  legal_file_path.append("/searchingthedashlegalnotice.html");
  g_app_info_launch_default_for_uri(legal_file_path.c_str(), NULL, &error);
  if (error)
  {
    LOG_ERROR(logger) << "Could not open legal uri: " << error.Message();
  }

  g_creat(legal_seen_file_path_.c_str(), S_IRWXU);

  ubus_.SendMessage(UBUS_OVERLAY_CLOSE_REQUEST);
}

void ScopeBar::AddScope(Scope::Ptr const& scope)
{
  ScopeBarIcon* icon = new ScopeBarIcon(scope->id, scope->icon_hint);

  icon->SetVisible(scope->visible);
  scope->visible.changed.connect([icon](bool visible) { icon->SetVisible(visible); } );
  icons_.push_back(icon);
  layout_->AddView(icon, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FIX);
  AddChild(icon);

  icon->mouse_click.connect([&, icon] (int x, int y, unsigned long button, unsigned long keyboard) { SetActive(icon); });
  icon->key_nav_focus_activate.connect([&, icon](nux::Area*){ SetActive(icon); });
}

void ScopeBar::Activate(std::string id)
{
  for (auto icon: icons_)
  {
    if (icon->id == id)
    {
      SetActive(icon);
      break;
    }
  }
}

void ScopeBar::Draw(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();

  graphics_engine.PushClippingRectangle(base);

  if (RedirectedAncestor())
    graphics::ClearGeometry(base);

  if (bg_layer_)
  {
    bg_layer_->SetGeometry(base);
    bg_layer_->Renderlayer(graphics_engine);
  }

  graphics_engine.PopClippingRectangle();
}

void ScopeBar::DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();

  graphics_engine.PushClippingRectangle(base);

  int pushed_paint_layers = 0;
  if (!IsFullRedraw())
  {
    if (RedirectedAncestor())
    { 
      // Whole Scope bar needs to be cleared because the PaintAll forces redraw.
      graphics::ClearGeometry(base);
    }

    if (bg_layer_)
    {
      nux::GetPainter().PushLayer(graphics_engine, bg_layer_->GetGeometry(), bg_layer_.get());
      pushed_paint_layers++;
    }
  }
  else
  {
    nux::GetPainter().PushPaintLayerStack();    
  }

  GetLayout()->ProcessDraw(graphics_engine, true);

  if (IsFullRedraw())
  {
    nux::GetPainter().PopPaintLayerStack();      
  }
  else if (pushed_paint_layers > 0)
  {
    nux::GetPainter().PopBackground(pushed_paint_layers);
  }

  for (auto icon: icons_)
  {
    if (icon->active && icon->IsVisible())
    {
      nux::Geometry const& geo = icon->GetGeometry();
      int middle = geo.x + geo.width/2;
      int size = 5;
      // Nux doesn't draw too well the small triangles, so let's draw a
      // bigger one and clip part of them using the "-1".
      int y = base.y - 1;

      nux::GetPainter().Draw2DTriangleColor(graphics_engine,
                                            middle - size, y,
                                            middle, y + size,
                                            middle + size, y,
                                            nux::color::White);

      break;
    }
  }

  graphics_engine.PopClippingRectangle();
}

nux::Area* ScopeBar::FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type)
{
  //LayeredLayout is acting a little screwy, events are not passing past the first layout like instructed,
  //so we manually override if the cursor is on the right hand side of the scopebar 
  auto geo = GetAbsoluteGeometry();
  int info_width = (info_previously_shown_) ? info_icon_->GetGeometry().width : legal_->GetGeometry().width;
  
  if (mouse_position.x - geo.x < geo.width - info_width - 10)
  {
    return nux::View::FindAreaUnderMouse(mouse_position, event_type);
  }
  else
  {
    if (info_previously_shown_)
      return dynamic_cast<nux::Area*>(info_icon_);
    else
      return dynamic_cast<nux::Area*>(legal_);
  }
  
}

void ScopeBar::SetActive(ScopeBarIcon* activated)
{
  bool state_changed = false;

  for (auto icon: icons_)
  {
    bool state = icon == activated;

    if (icon->active != state)
      state_changed = true;

    icon->active = state;
  }

  if (state_changed)
    scope_activated.emit(activated->id);
}

void ScopeBar::ActivateNext()
{
  bool activate_next = false;
  for (auto it = icons_.begin();
       it < icons_.end();
       it++)
  {
    ScopeBarIcon *icon = *it;

    if (activate_next && icon->IsVisible())
    {
      SetActive(icon);
      return;
    }
    if (icon->active)
      activate_next = true;
  }

  // fallback. select first visible icon.
  for (auto it = icons_.begin(); it != icons_.end(); ++it)
  {
    ScopeBarIcon *icon = *it;
    if (icon->IsVisible())
    {
      SetActive(icon);
      break;
    }
  }
}

void ScopeBar::ActivatePrevious()
{
  bool activate_previous = false;
  for (auto rit = icons_.rbegin();
       rit < icons_.rend();
       ++rit)
  {
    ScopeBarIcon *icon = *rit;

    if (activate_previous && icon->IsVisible())
    {
      SetActive(icon);
      return;
    }
    if (icon->active)
      activate_previous = true;
  }

  // fallback. select last visible icon.
  for (auto rit = icons_.rbegin(); rit != icons_.rend(); ++rit)
  {
    ScopeBarIcon *icon = *rit;
    if (icon->IsVisible())
    {
      SetActive(icon);
      break;
    }
  }
}

// Keyboard navigation
bool ScopeBar::AcceptKeyNavFocus()
{
  return false;
}

// Introspectable
std::string ScopeBar::GetName() const
{
  return "ScopeBar";
}

void ScopeBar::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper wrapper(builder);

  wrapper.add("focused-scope-icon", "");

  for( auto icon : icons_)
  {
    if (icon->active)
      wrapper.add("active-scope", icon->id.Get());

    if (icon->HasKeyFocus())
      wrapper.add("focused-scope-icon", icon->id.Get());
  }
}

std::string ScopeBar::GetActiveScopeId() const
{
  for (auto icon : icons_)
  {
    if (icon->active)
      return icon->id;
  }
  return "";
}

}
}
