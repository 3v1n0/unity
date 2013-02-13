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
//"Hi %s, you have open files that you might want to save before shutting down. Would you like to…";

namespace theme
{
  const std::string FONT = "Ubuntu Light 12";
  const unsigned LEFT_RIGHT_PADDING = 30;
  const unsigned TOP_PADDING = 19;
  const unsigned BOTTOM_PADDING = 12;

  const unsigned MAIN_SPACE = 10;
  const unsigned BUTTON_SPACE = 9;
  const unsigned BUTTONS_SPACE = 20;
}

class ActionButton : public nux::View
{
public:
  ActionButton(std::string const& label, std::string const& texture_name, NUX_FILE_LINE_DECL)
    : nux::View(NUX_FILE_LINE_PARAM)
  {
    std::string texture_prefix = PKGDATADIR"/" + texture_name;
    normal_tex_.Adopt(nux::CreateTexture2DFromFile((texture_prefix + ".png").c_str(), -1, true));
    highlight_tex_.Adopt(nux::CreateTexture2DFromFile((texture_prefix + "_highlight.png").c_str(), -1, true));

    auto main_layout = new nux::VLayout();
    main_layout->SetContentDistribution(nux::MAJOR_POSITION_CENTER);
    main_layout->SetSpaceBetweenChildren(theme::BUTTON_SPACE);

    image_view_ = new IconTexture(normal_tex_);
    image_view_->SetInputEventSensitivity(false);
    main_layout->AddView(image_view_, 1, nux::MINOR_POSITION_CENTER);

    label_view_ = new StaticCairoText(label);
    label_view_->SetFont(theme::FONT);
    label_view_->SetTextAlignment(StaticCairoText::AlignState::NUX_ALIGN_CENTRE);
    label_view_->SetInputEventSensitivity(false);
    main_layout->AddView(label_view_, 1, nux::MINOR_POSITION_CENTER);

    mouse_enter.connect([this] (int, int, unsigned long, unsigned long) { SetHighlighted(true); });
    mouse_leave.connect([this] (int, int, unsigned long, unsigned long) { SetHighlighted(false); });
    mouse_click.connect([this] (int, int, unsigned long, unsigned long) { activated.emit(); });

    SetLayout(main_layout);
  }

  void SetHighlighted(bool highlighted)
  {
    image_view_->SetTexture(highlighted ? highlight_tex_ : normal_tex_);
    label_view_->SetTextColor(highlighted ? nux::color::White : nux::color::Transparent);
  }

  void Draw(nux::GraphicsEngine& ctx, bool force)
  {
    GetLayout()->ProcessDraw(ctx, force);
  }

  sigc::signal<void> activated;

private:
  IconTexture* image_view_;
  StaticCairoText* label_view_;
  nux::ObjectPtr<nux::BaseTexture> normal_tex_;
  nux::ObjectPtr<nux::BaseTexture> highlight_tex_;
};

NUX_IMPLEMENT_OBJECT_TYPE(View);

View::View(Manager::Ptr const& manager)
  : manager_(manager)
{
  closable = true;
  auto main_layout = new nux::VLayout();
  int offset = style()->GetInternalOffset();
  main_layout->SetTopAndBottomPadding(offset + theme::TOP_PADDING, offset + theme::BOTTOM_PADDING);
  main_layout->SetLeftAndRightPadding(offset + theme::LEFT_RIGHT_PADDING);
  main_layout->SetSpaceBetweenChildren(theme::MAIN_SPACE);
  SetLayout(main_layout);

  auto header = glib::String(g_strdup_printf(_("Goodbye %s! Would you like to…"), manager->RealName().c_str())).Str();
  auto* header_view = new StaticCairoText(header);
  header_view->SetFont("Ubuntu Light 12");
  header_view->SetTextAlignment(StaticCairoText::AlignState::NUX_ALIGN_LEFT);
  main_layout->AddView(header_view);

  buttons_layout_ = new nux::HLayout();
  buttons_layout_->SetSpaceBetweenChildren(theme::BUTTONS_SPACE);
  main_layout->AddLayout(buttons_layout_);

  auto button = new ActionButton(_("Lock"), "lockscreen", NUX_TRACKER_LOCATION);
  button->activated.connect(sigc::mem_fun(manager_.get(), &Manager::LockScreen));
  button->activated.connect([this] {request_hide.emit();});
  buttons_layout_->AddView(button);

  if (manager_->CanSuspend())
  {
    button = new ActionButton(_("Suspend"), "suspend", NUX_TRACKER_LOCATION);
    button->activated.connect(sigc::mem_fun(manager_.get(), &Manager::Suspend));
    button->activated.connect([this] {request_hide.emit();});
    buttons_layout_->AddView(button);
  }

  if (manager_->CanHibernate())
  {
    button = new ActionButton(_("Hibernate"), "hibernate", NUX_TRACKER_LOCATION);
    button->activated.connect(sigc::mem_fun(manager_.get(), &Manager::Hibernate));
    button->activated.connect([this] {request_hide.emit();});
    buttons_layout_->AddView(button);
  }

  button = new ActionButton(_("Shutdown"), "shutdown", NUX_TRACKER_LOCATION);
  button->activated.connect(sigc::mem_fun(manager_.get(), &Manager::Shutdown));
  button->activated.connect([this] {request_hide.emit();});
  buttons_layout_->AddView(button);

  button = new ActionButton(_("Restart"), "restart", NUX_TRACKER_LOCATION);
  button->activated.connect(sigc::mem_fun(manager_.get(), &Manager::Reboot));
  button->activated.connect([this] {request_hide.emit();});
  buttons_layout_->AddView(button);
}

nux::Geometry View::GetBackgroundGeometry()
{
  return GetGeometry();
}

void View::DrawOverlay(nux::GraphicsEngine& GfxContext, bool force_draw, nux::Geometry const& clip)
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
