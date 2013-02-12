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
#include "config.h"

#include <Nux/Button.h>

namespace unity
{
namespace session
{

class ActionButton : public nux::View
{
public:

  ActionButton(std::string const& label, std::string const& texture_name, NUX_FILE_LINE_DECL)
    : nux::View(NUX_FILE_LINE_PARAM)
  {
    auto main_layout = new nux::VLayout();
    main_layout->SetContentDistribution(nux::MAJOR_POSITION_CENTER);
    main_layout->SetSpaceBetweenChildren(9);

    auto image_view = new nux::TextureArea();
    image_view->LoadImageFile(PKGDATADIR"/" + texture_name + ".png");
    image_view->SetMinMaxSize(168, 168);
    main_layout->AddView(image_view, 1, nux::MINOR_POSITION_CENTER);

    label_view_ = new StaticCairoText(label);
    label_view_->SetFont("Ubuntu Light 12");
    label_view_->SetTextAlignment(StaticCairoText::AlignState::NUX_ALIGN_CENTRE);
    main_layout->AddView(label_view_, 1, nux::MINOR_POSITION_CENTER);

    SetLayout(main_layout);
  }

  void Draw(nux::GraphicsEngine& ctx, bool force)
  {
    GetLayout()->ProcessDraw(ctx, force);
  }

  StaticCairoText* label_view_;
  nux::VLayout* layout_;
  std::string label_;
  std::string image_;
};

NUX_IMPLEMENT_OBJECT_TYPE(View);

View::View(Manager::Ptr const& manager)
  : manager_(manager)
{
  closable = true;
  auto main_layout = new nux::VLayout();
  int offset = style()->GetInternalOffset();
  main_layout->SetTopAndBottomPadding(offset + 19, offset + 12);
  main_layout->SetLeftAndRightPadding(offset + 30);
  main_layout->SetSpaceBetweenChildren(10);
  SetLayout(main_layout);

  auto header = glib::String(g_strdup_printf(_("Goodbye %s! Would you like to…"), manager->RealName().c_str())).Str();
  auto* header_view = new StaticCairoText(header);
  header_view->SetFont("Ubuntu Light 12");
  header_view->SetTextAlignment(StaticCairoText::AlignState::NUX_ALIGN_LEFT);
  main_layout->AddView(header_view);

  buttons_layout_ = new nux::HLayout();
  buttons_layout_->SetSpaceBetweenChildren(20);
  main_layout->AddLayout(buttons_layout_);

  auto button = new ActionButton(_("Lock"), "lockscreen", NUX_TRACKER_LOCATION);
  buttons_layout_->AddView(button);

  button = new ActionButton(_("Suspend"), "suspend", NUX_TRACKER_LOCATION);
  buttons_layout_->AddView(button);

  button = new ActionButton(_("Hibernate"), "hibernate", NUX_TRACKER_LOCATION);
  buttons_layout_->AddView(button);

  button = new ActionButton(_("Shutdown"), "shutdown", NUX_TRACKER_LOCATION);
  buttons_layout_->AddView(button);

  button = new ActionButton(_("Restart"), "restart", NUX_TRACKER_LOCATION);
  buttons_layout_->AddView(button);
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
