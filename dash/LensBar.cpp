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

#include <NuxCore/Logger.h>
#include "config.h"

#include "unity-shared/CairoTexture.h"
#include "LensBar.h"

namespace unity
{
namespace dash
{
namespace
{

nux::logging::Logger logger("unity.dash.lensbar");

const int LENSBAR_HEIGHT = 44;

}

NUX_IMPLEMENT_OBJECT_TYPE(LensBar);

LensBar::LensBar()
  : nux::View(NUX_TRACKER_LOCATION)
{
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
  layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
  layout_->SetContentDistribution(nux::MAJOR_POSITION_CENTER);
  SetLayout(layout_);

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

void LensBar::Draw(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();

  gfx_context.PushClippingRectangle(base);
  nux::GetPainter().PaintBackground(gfx_context, base);

  bg_layer_->SetGeometry(base);
  nux::GetPainter().RenderSinglePaintLayer(gfx_context, base, bg_layer_.get());

  gfx_context.PopClippingRectangle();
}

void LensBar::DrawContent(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();

  gfx_context.PushClippingRectangle(base);

  if (!IsFullRedraw())
    nux::GetPainter().PushLayer(gfx_context, bg_layer_->GetGeometry(), bg_layer_.get());

  layout_->ProcessDraw(gfx_context, force_draw);

  if (!IsFullRedraw())
    nux::GetPainter().PopBackground();

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

      nux::GetPainter().Draw2DTriangleColor(gfx_context,
                                            middle - size, y,
                                            middle, y + size,
                                            middle + size, y,
                                            nux::color::White);

      break;
    }
  }

  gfx_context.PopClippingRectangle();
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
