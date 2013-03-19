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
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 */

#include <gmock/gmock.h>
#include "SessionButton.h"

using namespace unity::session;

namespace unity
{
namespace session
{

struct TestSessionButton : testing::Test
{
  struct ButtonWrap : Button
  {
    ButtonWrap() : Button(Action::LOCK) {}

    using Button::AcceptKeyNavFocusOnMouseEnter;
    using Button::AcceptKeyNavFocusOnMouseDown;
    using Button::image_view_;
    using Button::highlight_tex_;
    using Button::normal_tex_;
    using Button::label_view_;
  };

  ButtonWrap button;
};

TEST_F(TestSessionButton, Construct)
{
  EXPECT_EQ(button.action(), Button::Action::LOCK);
  EXPECT_FALSE(button.highlighted());
  EXPECT_TRUE(button.AcceptKeyNavFocusOnMouseEnter());
  EXPECT_FALSE(button.AcceptKeyNavFocusOnMouseDown());
}

TEST_F(TestSessionButton, HighlightUpdatesTextures)
{
  button.highlighted = true;
  EXPECT_EQ(button.image_view_->texture(), button.highlight_tex_);
}

TEST_F(TestSessionButton, HighlightShowsText)
{
  button.highlighted = true;
  EXPECT_NE(button.label_view_->GetTextColor(), nux::color::Transparent);
}

TEST_F(TestSessionButton, UnHighlightUpdatesTextures)
{
  button.highlighted = true;
  button.highlighted = false;
  EXPECT_EQ(button.image_view_->texture(), button.normal_tex_);
}

TEST_F(TestSessionButton, UnHighlightHidesText)
{
  button.highlighted = true;
  button.highlighted = false;
  EXPECT_EQ(button.label_view_->GetTextColor(), nux::color::Transparent);
}

TEST_F(TestSessionButton, MouseEnterHighlights)
{
  button.mouse_enter.emit(0, 0, 0, 0);
  EXPECT_TRUE(button.highlighted());
}

TEST_F(TestSessionButton, MouseLeaveUnhighlights)
{
  button.highlighted = true;
  button.mouse_leave.emit(0, 0, 0, 0);
  EXPECT_FALSE(button.highlighted());
}

TEST_F(TestSessionButton, MouseClickActivatesIt)
{
  bool activated = false;
  button.activated.connect([&activated] { activated = true; });
  button.mouse_click.emit(0, 0, 0, 0);
  EXPECT_TRUE(activated);
}

TEST_F(TestSessionButton, KeyFocusBeginHighlights)
{
  button.begin_key_focus.emit();
  EXPECT_TRUE(button.highlighted());
}

TEST_F(TestSessionButton, KeyFocusEndUnhighlights)
{
  button.highlighted = true;
  button.end_key_focus.emit();
  EXPECT_FALSE(button.highlighted());
}

TEST_F(TestSessionButton, KeyFocusActivatesIt)
{
  bool activated = false;
  button.activated.connect([&activated] { activated = true; });
  button.key_nav_focus_activate.emit(&button);
  EXPECT_TRUE(activated);
}

// Action typed buttons tests

struct ActionButton : public testing::TestWithParam<Button::Action> {
  ActionButton()
    : button(GetParam())
  {}

  std::string GetExpectedLabel()
  {
    switch (GetParam())
    {
      case Button::Action::LOCK:
        return "Lock";
      case Button::Action::LOGOUT:
        return "Log Out";
      case Button::Action::SUSPEND:
        return "Suspend";
      case Button::Action::HIBERNATE:
        return "Hibernate";
      case Button::Action::SHUTDOWN:
        return "Shut Down";
      case Button::Action::REBOOT:
        return "Restart";
    }

    return "";
  }

  Button button;
};

INSTANTIATE_TEST_CASE_P(TestSessionButtonTypes, ActionButton,
  testing::Values(Button::Action::LOCK, Button::Action::LOGOUT, Button::Action::SUSPEND,
                  Button::Action::HIBERNATE, Button::Action::SHUTDOWN, Button::Action::REBOOT));

TEST_P(/*TestSessionButtonTypes*/ActionButton, Label)
{
  EXPECT_EQ(button.label(), GetExpectedLabel());
}

TEST_P(/*TestSessionButtonTypes*/ActionButton, Action)
{
  EXPECT_EQ(button.action(), GetParam());
}

} // session
} // unity