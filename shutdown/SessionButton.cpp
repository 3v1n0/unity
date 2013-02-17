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

#include "config.h"
#include "SessionButton.h"

#include <Nux/VLayout.h>


namespace unity
{
namespace session
{

namespace theme
{
  const std::string FONT = "Ubuntu Light 12";

  const unsigned BUTTON_SPACE = 9;
}

NUX_IMPLEMENT_OBJECT_TYPE(Button);

Button::Button(std::string const& label, std::string const& texture_name, NUX_FILE_LINE_DECL)
    : nux::View(NUX_FILE_LINE_PARAM)
    , highlighted(false)
{
  std::string texture_prefix = PKGDATADIR"/" + texture_name;
  normal_tex_.Adopt(nux::CreateTexture2DFromFile((texture_prefix + ".png").c_str(), -1, true));
  highlight_tex_.Adopt(nux::CreateTexture2DFromFile((texture_prefix + "_highlight.png").c_str(), -1, true));

  auto main_layout = new nux::VLayout();
  main_layout->SetContentDistribution(nux::MAJOR_POSITION_CENTER);
  main_layout->SetSpaceBetweenChildren(theme::BUTTON_SPACE);
  SetLayout(main_layout);

  image_view_ = new IconTexture(normal_tex_);
  image_view_->SetInputEventSensitivity(false);
  main_layout->AddView(image_view_, 1, nux::MINOR_POSITION_CENTER);

  label_view_ = new StaticCairoText(label);
  label_view_->SetFont(theme::FONT);
  label_view_->SetTextAlignment(StaticCairoText::AlignState::NUX_ALIGN_CENTRE);
  label_view_->SetTextColor(nux::color::Transparent);
  label_view_->SetInputEventSensitivity(false);
  main_layout->AddView(label_view_, 1, nux::MINOR_POSITION_CENTER);

  mouse_enter.connect([this] (int, int, unsigned long, unsigned long) { highlighted = true; });
  mouse_leave.connect([this] (int, int, unsigned long, unsigned long) { highlighted = false; });
  mouse_click.connect([this] (int, int, unsigned long, unsigned long) { activated.emit(); });

  begin_key_focus.connect([this] { highlighted = true; });
  end_key_focus.connect([this] { highlighted = false; });
  key_nav_focus_activate.connect([this] (Area*) { activated.emit(); });

  highlighted.changed.connect([this] (bool value) {
    image_view_->SetTexture(value ? highlight_tex_ : normal_tex_);
    label_view_->SetTextColor(value ? nux::color::White : nux::color::Transparent);

    if (value)
      nux::GetWindowCompositor().SetKeyFocusArea(this);
  });

  // This function ensures that when an item is activated, the button state
  // is reset as soon as the parent window has been closed.
  activated.connect([this] {
    auto* top_win = static_cast<nux::BaseWindow*>(GetTopLevelViewWindow());
    if (top_win && top_win->IsVisible())
    {
      auto conn = std::make_shared<sigc::connection>();
      *conn = top_win->sigHidden.connect([this, conn] (nux::BaseWindow*) {
        highlighted = false;
        conn->disconnect();
      });
    }
    else
    {
      highlighted = false;
    }
  });
}

void Button::Draw(nux::GraphicsEngine& ctx, bool force)
{
  GetLayout()->ProcessDraw(ctx, force);
}

} // namespace session
} // namespace unity
