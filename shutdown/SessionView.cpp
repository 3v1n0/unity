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
#include <UnityCore/Variant.h>
#include <glib/gi18n-lib.h>

namespace unity
{
namespace session
{

namespace style
{
  const std::string FONT = "Ubuntu Light";
  const std::string TITLE_FONT = FONT+" 15";
  const std::string SUBTITLE_FONT = FONT+" 12";

  const unsigned LEFT_RIGHT_PADDING = 30;
  const unsigned TOP_PADDING = 19;
  const unsigned BOTTOM_PADDING = 12;
  const unsigned MAIN_SPACE = 10;
  const unsigned BUTTONS_SPACE = 20;
}

NUX_IMPLEMENT_OBJECT_TYPE(View);

View::View(Manager::Ptr const& manager)
  : mode(Mode::FULL)
  , manager_(manager)
{
  closable = true;
  auto main_layout = new nux::VLayout();
  main_layout->SetTopAndBottomPadding(style::TOP_PADDING, style::BOTTOM_PADDING);
  main_layout->SetLeftAndRightPadding(style::LEFT_RIGHT_PADDING);
  main_layout->SetSpaceBetweenChildren(style::MAIN_SPACE);
  SetLayout(main_layout);

  title_ = new StaticCairoText("");
  title_->SetFont(style::TITLE_FONT);
  title_->SetTextAlignment(StaticCairoText::AlignState::NUX_ALIGN_LEFT);
  title_->SetInputEventSensitivity(false);
  title_->SetVisible(false);
  main_layout->AddView(title_);

  subtitle_ = new StaticCairoText("");
  subtitle_->SetFont(style::SUBTITLE_FONT);
  subtitle_->SetTextAlignment(StaticCairoText::AlignState::NUX_ALIGN_LEFT);
  subtitle_->SetInputEventSensitivity(false);
  subtitle_->SetLines(std::numeric_limits<int>::min());
  subtitle_->SetLineSpacing(2);
  main_layout->AddView(subtitle_);

  buttons_layout_ = new nux::HLayout();
  buttons_layout_->SetSpaceBetweenChildren(style::BUTTONS_SPACE);
  main_layout->AddLayout(buttons_layout_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_PERCENTAGE, 0.0f);

  GetBoundingArea()->mouse_click.connect([this] (int, int, unsigned long, unsigned long) { request_close.emit(); });

  have_inhibitors.changed.connect(sigc::hide(sigc::mem_fun(this, &View::UpdateText)));

  mode.SetSetterFunction([this] (Mode& target, Mode new_mode) {
    if (new_mode == Mode::SHUTDOWN && !manager_->CanShutdown())
      new_mode = Mode::LOGOUT;

    if (target != new_mode)
    {
      target = new_mode;
      return true;
    }

    return false;
  });

  mode.changed.connect([this] (Mode m) {
    UpdateText();
    Populate();
  });

  UpdateText();
  Populate();
}

void View::UpdateText()
{
  const char* message = nullptr;
  auto const& real_name = manager_->RealName();
  auto const& name = (real_name.empty() ? manager_->UserName() : real_name);

  if (mode() == Mode::SHUTDOWN)
  {
    title_->SetText(_("Shut Down"));
    title_->SetVisible(true);

    if (have_inhibitors())
    {
      message = _("Hi %s, you have open files that you might want to save " \
                  "before shutting down. Are you sure you want to continue?");
    }
    else
    {
      message = _("Goodbye %s! Are you sure you want to close all programs " \
                  "and shut down the computer?");
    }
  }
  else if (mode() == Mode::LOGOUT)
  {
    title_->SetText(_("Log Out"));
    title_->SetVisible(true);

    if (have_inhibitors())
    {
      message = _("Hi %s, you have open files that you might want to save " \
                  "before logging out. Are you sure you want to continue?");
    }
    else
    {
      message = _("Goodbye %s! Are you sure you want to close all programs " \
                  "and log out from your account?");
    }
  }
  else
  {
    title_->SetVisible(false);

    if (have_inhibitors())
    {
      if (buttons_layout_->GetChildren().size() > 3)
      {
        // We have enough buttons to show the message without a new line.
        message = _("Hi %s, you have open files you might want to save. " \
                    "Would you like to...");
      }
      else
      {
        message = _("Hi %s, you have open files you might want to save.\n" \
                    "Would you like to...");
      }
    }
    else
    {
      message = _("Goodbye %s! Would you like to...");
    }
  }

  subtitle_->SetText(glib::String(g_strdup_printf(message, name.c_str())).Str());
}

void View::Populate()
{
  debug::Introspectable::RemoveAllChildren();
  buttons_layout_->Clear();

  if (mode() == Mode::LOGOUT)
  {
    auto* button = new Button(_("Lock"), "lockscreen", NUX_TRACKER_LOCATION);
    button->activated.connect(sigc::mem_fun(manager_.get(), &Manager::LockScreen));
    AddButton(button);

    button = new Button(_("Logout"), "logout", NUX_TRACKER_LOCATION);
    button->activated.connect(sigc::mem_fun(manager_.get(), &Manager::Logout));
    AddButton(button);
  }
  else
  {
    if (mode() == Mode::FULL)
    {
      auto* button = new Button(_("Lock"), "lockscreen", NUX_TRACKER_LOCATION);
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
    }

    if (manager_->CanShutdown())
    {
      auto* button = new Button(_("Shutdown"), "shutdown", NUX_TRACKER_LOCATION);
      button->activated.connect(sigc::mem_fun(manager_.get(), &Manager::Shutdown));
      AddButton(button);

      button = new Button(_("Restart"), "restart", NUX_TRACKER_LOCATION);
      button->activated.connect(sigc::mem_fun(manager_.get(), &Manager::Reboot));
      AddButton(button);
    }
    else if (mode() == Mode::FULL)
    {
      auto* button = new Button(_("Logout"), "logout", NUX_TRACKER_LOCATION);
      button->activated.connect(sigc::mem_fun(manager_.get(), &Manager::Logout));
      AddButton(button);
    }
  }
}

void View::AddButton(Button* button)
{
  button->activated.connect([this] {request_hide.emit();});
  buttons_layout_->AddView(button);
  debug::Introspectable::AddChild(button);

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

void View::PreLayoutManagement()
{
  subtitle_->SetMaximumWidth(buttons_layout_->GetContentWidth());

  nux::View::PreLayoutManagement();
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

    // Let's reset the focused area if we're in keyboard-navigation mode.
    if (focused && focused->IsChildOf(buttons_layout_) && !focused->IsMouseInside())
      return this;
  }

  return UnityWindowView::FindKeyFocusArea(etype, key_code, modifiers);
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

void View::AddProperties(GVariantBuilder* builder)
{
  UnityWindowView::AddProperties(builder);
  variant::BuilderWrapper(builder)
    .add("mode", static_cast<int>(mode()))
    .add("inhibitors", have_inhibitors())
    .add("title",title_->GetText())
    .add("subtitle",subtitle_->GetText());
}

} // namespace session
} // namespace unity
