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
#include "LensBar.h"
#include <NuxCore/Logger.h>
#include "config.h"
#include <Nux/HLayout.h>
#include <Nux/LayeredLayout.h>

#include "unity-shared/DashStyle.h"
#include "unity-shared/StaticCairoText.h"
#include "unity-shared/CairoTexture.h"
#include "unity-shared/UBusMessages.h"

namespace unity
{
namespace dash
{
namespace
{

nux::logging::Logger logger("unity.dash.lensbar");

// according to Q design the inner area of the lensbar should be 40px
// (without any borders)
const int LENSBAR_HEIGHT = 41;

}

NUX_IMPLEMENT_OBJECT_TYPE(LensBar);

LensBar::LensBar()
  : nux::View(NUX_TRACKER_LOCATION)
  , info_previously_shown_(false)
{
  glib::String cachedir(g_strdup(g_get_user_cache_dir())); 
  legal_seen_file_path_ = cachedir.Str() + "/unitydashlegalseen";
  info_previously_shown_ = (g_file_test(legal_seen_file_path_.c_str(), G_FILE_TEST_EXISTS)) ? true : false;

  SetupBackground();
  SetupLayout();
  SetupHomeLens();
}

void LensBar::SetupBackground()
{
  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  bg_layer_.reset(new nux::ColorLayer(nux::Color(0.0f, 0.0f, 0.0f, 0.2f), true, rop));
}

void LensBar::SetupLayout()
{
  legal_layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
  std::string legal_text("<span underline='single'>");
  legal_text.append(g_dgettext("credentials-control-center", "Legal notice"));
  legal_text.append("</span>");
  legal_ = new nux::StaticCairoText(legal_text);
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
  
  SetMinimumHeight(LENSBAR_HEIGHT);
  SetMaximumHeight(LENSBAR_HEIGHT);
}

void LensBar::SetupHomeLens()
{
  LensBarIcon* icon = new LensBarIcon("home.lens", PKGDATADIR"/lens-nav-home.svg");
  icon->SetVisible(true);
  icon->active = true;
  icons_.push_back(icon);
  layout_->AddView(icon, 0, nux::eCenter, nux::MINOR_SIZE_FULL);
  AddChild(icon);

  icon->mouse_click.connect([&, icon] (int x, int y, unsigned long button, unsigned long keyboard) { SetActive(icon); });
  icon->key_nav_focus_activate.connect([&, icon](nux::Area*){ SetActive(icon); });
}

void LensBar::DoOpenLegalise()
{
  glib::Error error;
  g_app_info_launch_default_for_uri("file:///tmp/foo.html", NULL, &error);
  if (error)
  {
    LOG_ERROR(logger) << "Could not open legal uri: " << error.Message();
  }

  g_creat(legal_seen_file_path_.c_str(), O_CREAT);

  ubus_.SendMessage(UBUS_PLACE_VIEW_CLOSE_REQUEST);
}

void LensBar::AddLens(Lens::Ptr& lens)
{
  LensBarIcon* icon = new LensBarIcon(lens->id, lens->icon_hint);
  icon->SetVisible(lens->visible);
  lens->visible.changed.connect([icon](bool visible) { icon->SetVisible(visible); } );
  icons_.push_back(icon);
  layout_->AddView(icon, 0, nux::eCenter, nux::eFix);
  AddChild(icon);

  icon->mouse_click.connect([&, icon] (int x, int y, unsigned long button, unsigned long keyboard) { SetActive(icon); });
  icon->key_nav_focus_activate.connect([&, icon](nux::Area*){ SetActive(icon); });
}

void LensBar::Activate(std::string id)
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

void LensBar::Draw(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();

  graphics_engine.PushClippingRectangle(base);

  bg_layer_->SetGeometry(base);
  nux::GetPainter().RenderSinglePaintLayer(graphics_engine, base, bg_layer_.get());
 
  graphics_engine.PopClippingRectangle();
}

void LensBar::DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();

  graphics_engine.PushClippingRectangle(base);

  int pushed_paint_layers = 0;
  if(RedirectedAncestor())
  {
    {
      unsigned int alpha = 0, src = 0, dest = 0;
      graphics_engine.GetRenderStates().GetBlend(alpha, src, dest);
      // This is necessary when doing redirected rendering.
      // Clean the area below this view before drawing anything.
      graphics_engine.GetRenderStates().SetBlend(false);
      graphics_engine.QRP_Color(GetX(), GetY(), GetWidth(), GetHeight(), nux::Color(0.0f, 0.0f, 0.0f, 0.0f));
      graphics_engine.GetRenderStates().SetBlend(alpha, src, dest);
    }

    nux::GetPainter().RenderSinglePaintLayer(graphics_engine, bg_layer_->GetGeometry(), bg_layer_.get());
  }
  else if (!IsFullRedraw())
  {
    pushed_paint_layers += 2;
    nux::GetPainter().PushLayer(graphics_engine, bg_layer_->GetGeometry(), bg_layer_.get());
  }

  GetLayout()->ProcessDraw(graphics_engine, true);

  if (pushed_paint_layers)
    nux::GetPainter().PopBackground(pushed_paint_layers);

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

nux::Area* LensBar::FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type)
{
  //LayeredLayout is acting a little screwy, events are not passing past the first layout like instructed,
  //so we manually override if the cursor is on the right hand side of the lensbar 
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

void LensBar::SetActive(LensBarIcon* activated)
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
    lens_activated.emit(activated->id);
}

void LensBar::ActivateNext()
{
  // Special case when switching from the command lens.
  if (GetActiveLensId() == "commands.lens")
  {
    SetActive(icons_[0]);
    return;
  }

  bool activate_next = false;
  for (auto it = icons_.begin();
       it < icons_.end();
       it++)
  {
    LensBarIcon *icon = *it;

    if (activate_next && icon->IsVisible())
    {
      SetActive(icon);
      return;
    }
    if (icon->active)
      activate_next = true;
  }
  SetActive(icons_[0]);

}

void LensBar::ActivatePrevious()
{
  // Special case when switching from the command lens.
  if (GetActiveLensId() == "commands.lens")
  {
    SetActive(icons_.back());
    return;
  }

  bool activate_previous = false;
  for (auto it = icons_.rbegin();
       it < icons_.rend();
       ++it)
  {
    LensBarIcon *icon = *it;

    if (activate_previous && icon->IsVisible())
    {
	SetActive(icon);
	return;
    }
    if (icon->active)
      activate_previous = true;
  }
  SetActive(icons_.back());

}

// Keyboard navigation
bool LensBar::AcceptKeyNavFocus()
{
  return false;
}

// Introspectable
std::string LensBar::GetName() const
{
  return "LensBar";
}

void LensBar::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper wrapper(builder);

  wrapper.add("focused-lens-icon", "");

  for( auto icon : icons_)
  {
    if (icon->active)
      wrapper.add("active-lens", icon->id.Get());

    if (icon->HasKeyFocus())
      wrapper.add("focused-lens-icon", icon->id.Get());
  }
}

std::string LensBar::GetActiveLensId() const
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
