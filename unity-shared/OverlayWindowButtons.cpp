/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 *
 */

#include "OverlayWindowButtons.h"
#include "PanelStyle.h"
#include "UScreen.h"

namespace
{
  const int MAIN_LEFT_PADDING = 4;
  const int MENUBAR_PADDING = 4;
}

namespace unity
{

NUX_IMPLEMENT_OBJECT_TYPE(OverlayWindowButtons);

OverlayWindowButtons::OverlayWindowButtons()
  : nux::BaseWindow("OverlayWindowButtons")
  , window_buttons_(new WindowButtons())
{
  window_buttons_->queue_draw.connect([this] (nux::Layout* /*layout*/) {
    QueueDraw();
  });

  AddChild(window_buttons_.GetPointer());
  UpdateGeometry();
  SetBackgroundColor(nux::color::Transparent);
}

bool OverlayWindowButtons::IsVisibleOnMonitor(unsigned int monitor) const
{
  if (window_buttons_->monitor == monitor)
    return true;
  
  return false;
}

void OverlayWindowButtons::UpdateGeometry()
{
  int monitor = unity::UScreen::GetDefault()->GetMonitorWithMouse();
  nux::Geometry const& geo = unity::UScreen::GetDefault()->GetMonitorGeometry(monitor);

  SetX(geo.x + MAIN_LEFT_PADDING);
  SetY(geo.y + MENUBAR_PADDING);
  SetHeight(panel::Style::Instance().PanelHeight(monitor));

  window_buttons_->monitor = monitor;
}

void OverlayWindowButtons::Show()
{
  UpdateGeometry();
  ShowWindow(true);
  PushToFront();
  QueueDraw();
}

void OverlayWindowButtons::Hide()
{
  ShowWindow(false);
  PushToBack();
  QueueDraw();
}

nux::Point GetRelativeMousePosition(nux::Point const& pos)
{
  int monitor = unity::UScreen::GetDefault()->GetMonitorWithMouse();
  nux::Geometry const& geo = unity::UScreen::GetDefault()->GetMonitorGeometry(monitor);

  return nux::Point(pos.x - geo.x, pos.y - geo.y);
}

nux::Area* OverlayWindowButtons::FindAreaUnderMouse(nux::Point const& mouse_position,
                                                    nux::NuxEventType event_type)
{
  return window_buttons_->FindAreaUnderMouse(GetRelativeMousePosition(mouse_position), event_type);
}

void OverlayWindowButtons::Draw(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  window_buttons_->ProcessDraw(gfx_context, true);
}


// Introspection
std::string OverlayWindowButtons::GetName() const
{
  return "OverlayWindowButtons";
}

void OverlayWindowButtons::AddProperties(debug::IntrospectionData& introspection)
{
  introspection.add(GetAbsoluteGeometry());
}

} // namespace unity
