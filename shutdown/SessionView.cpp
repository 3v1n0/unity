// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
* Authored by: Marco Trevisan (Treviño) <marco@ubuntu.com>
*/

#include "SessionView.h"

#include <Nux/VLayout.h>
#include <UnityCore/GLibWrapper.h>
#include <glib/gi18n-lib.h>
#include "unity-shared/StaticCairoText.h"

namespace unity
{
namespace session
{
NUX_IMPLEMENT_OBJECT_TYPE(View);

View::View(Manager::Ptr const& manager)
  : manager_(manager)
{
  closable = true;
  auto main_layout = new nux::VLayout();
  int offset = style()->GetInternalOffset();
  main_layout->SetPadding(offset + 20, offset + 28);
  SetLayout(main_layout);

  auto header = glib::String(g_strdup_printf(_("Goodbye %s! Would you like to…"), manager->RealName().c_str())).Str();
  auto* header_view = new StaticCairoText(header);
  header_view->SetFont("Ubuntu Light 12");
  main_layout->AddView(header_view, 1 , nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
}

nux::Geometry View::GetBackgroundGeometry()
{
  return GetGeometry();
}

void View::DrawOverlay(nux::GraphicsEngine& GfxContext, bool force_draw, nux::Geometry clip)
{
  view_layout_->ProcessDraw(GfxContext, force_draw);
}

//
// Introspectable methods
//
std::string View::GetName() const
{
  return "SessionView";
}

} // namespace session
} // namespace unity
