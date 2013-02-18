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
#include "SessionButton.h"

#include <Nux/VLayout.h>
#include <UnityCore/GLibWrapper.h>
#include <glib/gi18n-lib.h>

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
  const unsigned BUTTONS_SPACE = 20;
}

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

  auto const& real_name = manager->RealName();
  auto const& name = (real_name.empty() ? manager->UserName() : real_name);
  auto header = glib::String(g_strdup_printf(_("Goodbye %s! Would you like to…"), name.c_str())).Str();
  auto* header_view = new StaticCairoText(header);
  header_view->SetFont(theme::FONT);
  header_view->SetTextAlignment(StaticCairoText::AlignState::NUX_ALIGN_LEFT);
  main_layout->AddView(header_view);

  buttons_layout_ = new nux::HLayout();
  buttons_layout_->SetSpaceBetweenChildren(theme::BUTTONS_SPACE);
  main_layout->AddLayout(buttons_layout_);

  auto button = new Button(_("Lock"), "lockscreen", NUX_TRACKER_LOCATION);
  button->activated.connect(sigc::mem_fun(manager_.get(), &Manager::LockScreen));
  AddButton(button);

  if (manager_->CanSuspend())
  {
    button = new Button(_("Suspend"), "suspend", NUX_TRACKER_LOCATION);
    button->activated.connect(sigc::mem_fun(manager_.get(), &Manager::Suspend));
    AddButton(button);
  }

  if (manager_->CanHibernate())
  {
    button = new Button(_("Hibernate"), "hibernate", NUX_TRACKER_LOCATION);
    button->activated.connect(sigc::mem_fun(manager_.get(), &Manager::Hibernate));
    AddButton(button);
  }

  button = new Button(_("Shutdown"), "shutdown", NUX_TRACKER_LOCATION);
  button->activated.connect(sigc::mem_fun(manager_.get(), &Manager::Shutdown));
  AddButton(button);

  button = new Button(_("Restart"), "restart", NUX_TRACKER_LOCATION);
  button->activated.connect(sigc::mem_fun(manager_.get(), &Manager::Reboot));
  AddButton(button);
}

void View::AddButton(Button* button)
{
  button->activated.connect([this] {request_hide.emit();});
  buttons_layout_->AddView(button);

  // This resets back the keyboard focus to the view when a button is unselected
  button->highlighted.changed.connect([this] (bool value) {
    if (!value)
      nux::GetWindowCompositor().SetKeyFocusArea(this);
  });

  // This function ensures that when an item is activated, the button state
  // is reset as soon as the parent window has been closed.
  button->activated.connect([this, button] {
    auto* top_win = static_cast<nux::BaseWindow*>(GetTopLevelViewWindow());
    if (top_win && top_win->IsVisible())
    {
      auto conn = std::make_shared<sigc::connection>();
      *conn = top_win->sigHidden.connect([this, button, conn] (nux::BaseWindow*) {
        button->highlighted = false;
        conn->disconnect();
      });
    }
    else
    {
      button->highlighted = false;
    }
  });
}

nux::Geometry View::GetBackgroundGeometry()
{
  return GetGeometry();
}

void View::DrawOverlay(nux::GraphicsEngine& GfxContext, bool force_draw, nux::Geometry const& clip)
{
  view_layout_->ProcessDraw(GfxContext, force_draw);
}

nux::Area* View::FindKeyFocusArea(unsigned etype, unsigned long key_code, unsigned long modifiers)
{
  if (etype != nux::NUX_KEYDOWN)
    return UnityWindowView::FindKeyFocusArea(etype, key_code, modifiers);

  if (key_code == NUX_VK_LEFT || key_code == NUX_VK_RIGHT)
  {
    nux::InputArea* focused = nux::GetWindowCompositor().GetKeyFocusArea();

    if (!focused || focused == this || !focused->IsChildOf(this))
    {
      if (key_code == NUX_VK_LEFT)
      {
        return buttons_layout_->GetChildren().front();
      }
      else if (key_code == NUX_VK_RIGHT)
      {
        return buttons_layout_->GetChildren().back();
      }
    }
  }
  else if (key_code == NUX_VK_ESCAPE)
  {
    nux::InputArea* focused = nux::GetWindowCompositor().GetKeyFocusArea();

    if (!focused || !focused->IsMouseInside())
      return this;
  }

  return nux::View::FindKeyFocusArea(etype, key_code, modifiers);
}

nux::Area* View::KeyNavIteration(nux::KeyNavDirection direction)
{
  if (direction == nux::KEY_NAV_LEFT)
    return buttons_layout_->GetChildren().back();
  else if (direction == nux::KEY_NAV_RIGHT)
    return buttons_layout_->GetChildren().front();

  return nux::View::KeyNavIteration(direction);
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
