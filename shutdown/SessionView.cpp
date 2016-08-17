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

#include <UnityCore/GLibWrapper.h>
#include <glib/gi18n-lib.h>

#include "unity-shared/RawPixel.h"

namespace unity
{
namespace session
{

namespace style
{
  std::string const FONT = "Ubuntu Light";
  std::string const TITLE_FONT = FONT+" 15";
  std::string const SUBTITLE_FONT = FONT+" 12";

  RawPixel const LEFT_RIGHT_PADDING = 30_em;
  RawPixel const TOP_PADDING        = 19_em;
  RawPixel const BOTTOM_PADDING     = 12_em;
  RawPixel const MAIN_SPACE         = 10_em;
  RawPixel const BUTTONS_SPACE      = 20_em;
}

NUX_IMPLEMENT_OBJECT_TYPE(View);

View::View(Manager::Ptr const& manager)
  : mode(Mode::FULL)
  , key_focus_area([this] { return key_focus_area_; })
  , manager_(manager)
  , key_focus_area_(this)
{
  closable = true;
  main_layout_ = new nux::VLayout();
  SetLayout(main_layout_);

  title_ = new StaticCairoText("");
  title_->SetFont(style::TITLE_FONT);
  title_->SetTextAlignment(StaticCairoText::AlignState::NUX_ALIGN_LEFT);
  title_->SetInputEventSensitivity(false);
  title_->SetVisible(false);
  main_layout_->AddView(title_);

  subtitle_ = new StaticCairoText("");
  subtitle_->SetFont(style::SUBTITLE_FONT);
  subtitle_->SetTextAlignment(StaticCairoText::AlignState::NUX_ALIGN_LEFT);
  subtitle_->SetInputEventSensitivity(false);
  subtitle_->SetLines(std::numeric_limits<int>::min());
  subtitle_->SetLineSpacing(2);
  main_layout_->AddView(subtitle_);

  buttons_layout_ = new nux::HLayout();
  main_layout_->AddLayout(buttons_layout_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_PERCENTAGE, 0.0f);

  GetBoundingArea()->mouse_click.connect([this] (int, int, unsigned long, unsigned long) { request_close.emit(); });

  have_inhibitors.changed.connect(sigc::hide(sigc::mem_fun(this, &View::UpdateText)));
  manager_->have_other_open_sessions.changed.connect(sigc::hide(sigc::mem_fun(this, &View::UpdateText)));

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

  mode.changed.connect(sigc::hide(sigc::mem_fun(this, &View::UpdateContents)));
  scale.changed.connect(sigc::hide(sigc::mem_fun(this, &View::UpdateViewSize)));
  UpdateContents();
}

void View::UpdateContents()
{
  SetVisible(true);
  PopulateButtons();
  UpdateText();
  UpdateViewSize();
}

void View::UpdateViewSize()
{
  main_layout_->SetTopAndBottomPadding(style::TOP_PADDING.CP(scale()), style::BOTTOM_PADDING.CP(scale()));
  main_layout_->SetLeftAndRightPadding(style::LEFT_RIGHT_PADDING.CP(scale()));
  main_layout_->SetSpaceBetweenChildren(style::MAIN_SPACE.CP(scale()));

  title_->SetScale(scale());
  subtitle_->SetScale(scale());

  ReloadCloseButtonTexture();

  buttons_layout_->SetSpaceBetweenChildren(style::BUTTONS_SPACE.CP(scale()));
  auto const& buttons = buttons_layout_->GetChildren();

  for (auto* area : buttons)
    static_cast<Button*>(area)->scale = scale();

  if (buttons.size() == 1)
  {
    auto* button = buttons.front();
    button->ComputeContentSize();
    int padding = button->GetWidth()/2 + style::MAIN_SPACE.CP(scale())/2;
    buttons_layout_->SetLeftAndRightPadding(padding, padding);
  }
}

void View::UpdateText()
{
  std::string message;
  std::string other_users_msg;
  auto const& real_name = manager_->RealName();
  auto const& name = (real_name.empty() ? manager_->UserName() : real_name);

  other_users_msg = _("Other users are logged in. Restarting or shutting down will close their open applications and may cause them to lose work.\n\n");

  if (mode() == Mode::SHUTDOWN)
  {
    title_->SetText(_("Shut Down"));
    title_->SetVisible(true);

    if (manager_->have_other_open_sessions())
    {
      message += other_users_msg;
    }

    if (have_inhibitors())
    {
      message += _("Hi %s, you have open files that you might want to save " \
                   "before shutting down. Are you sure you want to continue?");
    }
    else
    {
      message += _("Goodbye, %s. Are you sure you want to close all programs " \
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
      message = _("Goodbye, %s. Are you sure you want to close all programs " \
                  "and log out from your account?");
    }
  }
  else
  {
    title_->SetVisible(false);

    if (manager_->have_other_open_sessions())
    {
      message += other_users_msg;
    }

    if (have_inhibitors())
    {
      if (buttons_layout_->GetChildren().size() > 3)
      {
        // We have enough buttons to show the message without a new line.
        message += _("Hi %s, you have open files you might want to save. " \
                    "Would you like to…");
      }
      else
      {
        message += _("Hi %s, you have open files you might want to save.\n" \
                    "Would you like to…");
      }
    }
    else
    {
      message += _("Goodbye, %s. Would you like to…");
    }
  }

  subtitle_->SetText(glib::String(g_strdup_printf(message.c_str(), name.c_str())).Str());
}

void View::PopulateButtons()
{
  debug::Introspectable::RemoveAllChildren();
  buttons_layout_->Clear();
  buttons_layout_->SetLeftAndRightPadding(0, 0);
  key_focus_area_ = this;

  if (mode() == Mode::LOGOUT)
  {
    if (manager_->is_locked())
      return;

    if (manager_->CanLock())
    {
      auto* button = new Button(Button::Action::LOCK, NUX_TRACKER_LOCATION);
      button->activated.connect(sigc::mem_fun(manager_.get(), &Manager::LockScreen));
      AddButton(button);
    }

    auto* button = new Button(Button::Action::LOGOUT, NUX_TRACKER_LOCATION);
    button->activated.connect(sigc::mem_fun(manager_.get(), &Manager::Logout));
    key_focus_area_ = button;
    AddButton(button);
  }
  else
  {
    if (mode() == Mode::FULL)
    {
      if (manager_->CanLock() && !manager_->is_locked())
      {
        auto* button = new Button(Button::Action::LOCK, NUX_TRACKER_LOCATION);
        button->activated.connect(sigc::mem_fun(manager_.get(), &Manager::LockScreen));
        AddButton(button);
      }

      if (manager_->CanSuspend())
      {
        auto* button = new Button(Button::Action::SUSPEND, NUX_TRACKER_LOCATION);
        button->activated.connect(sigc::mem_fun(manager_.get(), &Manager::Suspend));
        AddButton(button);
      }

      if (manager_->CanHibernate())
      {
        auto* button = new Button(Button::Action::HIBERNATE, NUX_TRACKER_LOCATION);
        button->activated.connect(sigc::mem_fun(manager_.get(), &Manager::Hibernate));
        AddButton(button);
      }
    }

    if (manager_->CanShutdown())
    {
      auto *button = new Button(Button::Action::REBOOT, NUX_TRACKER_LOCATION);
      button->activated.connect(sigc::mem_fun(manager_.get(), &Manager::Reboot));
      AddButton(button);

      button = new Button(Button::Action::SHUTDOWN, NUX_TRACKER_LOCATION);
      button->activated.connect(sigc::mem_fun(manager_.get(), &Manager::Shutdown));
      key_focus_area_ = (mode() == Mode::SHUTDOWN) ? button : key_focus_area_;
      AddButton(button);
    }
    else if (mode() == Mode::FULL && !manager_->is_locked())
    {
      auto* button = new Button(Button::Action::LOGOUT, NUX_TRACKER_LOCATION);
      button->activated.connect(sigc::mem_fun(manager_.get(), &Manager::Logout));
      AddButton(button);
    }
  }

  cancel_idle_.reset();
  if (buttons_layout_->GetChildren().empty())
  {
    // There's nothing to show here, let's cancel the action and hide
    SetVisible(false);
    cancel_idle_.reset(new glib::Idle([this] { request_close.emit(); return false; }));
  }
}

void View::AddButton(Button* button)
{
  button->scale = scale();
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

void View::AddProperties(debug::IntrospectionData& introspection)
{
  UnityWindowView::AddProperties(introspection);
  introspection
    .add("mode", static_cast<int>(mode()))
    .add("inhibitors", have_inhibitors())
    .add("title",title_->GetText())
    .add("subtitle",subtitle_->GetText());
}

} // namespace session
} // namespace unity
