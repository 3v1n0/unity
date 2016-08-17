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
#include <glib/gi18n-lib.h>
#include "unity-shared/ThemeSettings.h"

namespace unity
{
namespace session
{

namespace style
{
  std::string const FONT = "Ubuntu Light 12";
  RawPixel const BUTTON_SPACE = 9_em;
}

NUX_IMPLEMENT_OBJECT_TYPE(Button);

Button::Button(Action action, NUX_FILE_LINE_DECL)
  : nux::View(NUX_FILE_LINE_PARAM)
  , scale(1.0)
  , highlighted(false)
  , pressed(false)
  , action([this] { return action_; })
  , label([this] { return label_view_->GetText(); })
  , action_(action)
{
  SetAcceptKeyNavFocusOnMouseDown(false);
  SetAcceptKeyNavFocusOnMouseEnter(true);

  std::string texture_prefix;
  std::string label;

  switch (action_)
  {
    case Action::LOCK:
      texture_prefix = "lockscreen";
      label = _("Lock");
      break;
    case Action::LOGOUT:
      texture_prefix = "logout";
      label = _("Log Out");
      break;
    case Action::SUSPEND:
      texture_prefix = "suspend";
      label = _("Suspend");
      break;
    case Action::HIBERNATE:
      texture_prefix = "hibernate";
      label = _("Hibernate");
      break;
    case Action::SHUTDOWN:
      texture_prefix = "shutdown";
      label = _("Shut Down");
      break;
    case Action::REBOOT:
      texture_prefix = "restart";
      label = _("Restart");
      break;
  }

  UpdateTextures(texture_prefix);

  auto main_layout = new nux::VLayout();
  main_layout->SetContentDistribution(nux::MAJOR_POSITION_CENTER);
  main_layout->SetSpaceBetweenChildren(style::BUTTON_SPACE);
  SetLayout(main_layout);

  scale.changed.connect([this, main_layout, texture_prefix] (double new_scale) {
    main_layout->SetSpaceBetweenChildren(style::BUTTON_SPACE.CP(new_scale));
    label_view_->SetScale(new_scale);

    UpdateTextures(texture_prefix);
    image_view_->SetTexture(highlighted ? highlight_tex_ : normal_tex_);
  });

  theme::Settings::Get()->theme.changed.connect(sigc::hide(sigc::bind(sigc::mem_fun(this, &Button::UpdateTextures), texture_prefix)));

  image_view_ = new IconTexture(normal_tex_);
  image_view_->SetInputEventSensitivity(false);
  main_layout->AddView(image_view_, 1, nux::MINOR_POSITION_CENTER);

  label_view_ = new StaticCairoText(label);
  label_view_->SetFont(style::FONT);
  label_view_->SetTextAlignment(StaticCairoText::AlignState::NUX_ALIGN_CENTRE);
  label_view_->SetTextColor(nux::color::Transparent);
  label_view_->SetInputEventSensitivity(false);
  main_layout->AddView(label_view_, 1, nux::MINOR_POSITION_CENTER);

  mouse_enter.connect([this] (int, int, unsigned long, unsigned long) { highlighted = true; });
  mouse_leave.connect([this] (int, int, unsigned long, unsigned long) { highlighted = false; });
  mouse_click.connect([this] (int, int, unsigned long, unsigned long) { activated.emit(); });
  mouse_down.connect([this] (int, int, unsigned long, unsigned long) { pressed = true; });
  mouse_up.connect([this] (int, int, unsigned long, unsigned long) { pressed = false; });

  begin_key_focus.connect([this] { highlighted = true; });
  end_key_focus.connect([this] { highlighted = false; });
  key_nav_focus_activate.connect([this] (Area*) { activated.emit(); });

  highlighted.changed.connect([this] (bool value) {
    image_view_->SetTexture(value ? highlight_tex_ : normal_tex_);
    label_view_->SetTextColor(value ? nux::color::White : nux::color::Transparent);
  });

  pressed.changed.connect([this] (bool value) {
    image_view_->SetOpacity(value ? 0.75 : 1.0);
  });
}

void Button::UpdateTextures(std::string const& texture_prefix)
{
  auto const& theme = theme::Settings::Get();
  auto texture_path = theme->ThemedFilePath(texture_prefix, {PKGDATADIR});
  RawPixel const texture_size = GetDefaultMaxTextureSize(texture_path);
  normal_tex_.Adopt(nux::CreateTexture2DFromFile(texture_path.c_str(), texture_size.CP(scale), true));

  auto texture_highlight_path = theme->ThemedFilePath(texture_prefix + "_highlight", {PKGDATADIR});
  RawPixel const texture_highlight_size = GetDefaultMaxTextureSize(texture_path);
  highlight_tex_.Adopt(nux::CreateTexture2DFromFile(texture_highlight_path.c_str(), texture_highlight_size.CP(scale), true));
}

RawPixel Button::GetDefaultMaxTextureSize(std::string const& texture_path) const
{
  nux::Size size;
  gdk_pixbuf_get_file_info(texture_path.c_str(), &size.width, &size.height);
  RawPixel max_size = std::max(size.width, size.height);

  return max_size;
}

void Button::Draw(nux::GraphicsEngine& ctx, bool force)
{
  GetLayout()->ProcessDraw(ctx, force);
}

//
// Introspectable methods
//
std::string Button::GetName() const
{
  return "SessionButton";
}

void Button::AddProperties(debug::IntrospectionData& introspection)
{
  introspection
    .add("highlighted", highlighted())
    .add("label", label())
    .add("label_color", label_view_->GetTextColor())
    .add("label_visible", label_view_->GetTextColor() != nux::color::Transparent);
}

} // namespace session
} // namespace unity
