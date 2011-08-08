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

#include "DashView.h"

#include "NuxCore/Logger.h"

#include "UBusMessages.h"

namespace unity
{
namespace dash
{

namespace
{
nux::logging::Logger logger("unity.dash.view");
}

DashView::DashView()
  : nux::View(NUX_TRACKER_LOCATION)
{

}

DashView::~DashView()
{}

long DashView::ProcessEvent(nux::IEvent& ievent, long traverse_info, long event_info)
{
  return traverse_info;
}

void DashView::Draw(nux::GraphicsEngine& gfx_context, bool force_draw)
{

}

void DashView::DrawContent(nux::GraphicsEngine& gfx_context, bool force_draw)
{

}

// Introspectable
const gchar* DashView::GetName()
{
  return "DashView";
}

void DashView::AddProperties(GVariantBuilder* builder)
{}


}
}
