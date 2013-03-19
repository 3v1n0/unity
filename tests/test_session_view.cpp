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
#include "test_mock_session_manager.h"
#include "SessionButton.h"
#include "SessionView.h"
#include "UnitySettings.h"

namespace unity
{
namespace session
{

struct TestSessionView : testing::Test
{
  TestSessionView()
    : manager(std::make_shared<testing::NiceMock<MockManager>>())
    , view(manager)
  {}

  struct ViewWrap : View
  {
    ViewWrap(Manager::Ptr const& manager) : View(manager) {}

    std::string GetTitle() const { return title_->IsVisible() ? title_->GetText() : ""; }
    std::string GetSubTitle() const { return subtitle_->GetText(); }

    std::list<Button*> GetButtons() const
    {
      std::list<Button*> buttons;

      for (auto const& button : buttons_layout_->GetChildren())
      {
        auto session_button = dynamic_cast<Button*>(button);
        EXPECT_NE(session_button, nullptr);

        if (!session_button)
          return std::list<Button*>();

        buttons.push_back(session_button);
      }

      return buttons;
    }

    Button* GetButtonByLabel(std::string const& label) const
    {
      for (auto const& button : GetButtons())
      {
        if (button->label() == label)
          return button;
      }

      return nullptr;
    }
  };

  void TearDown()
  {
    nux::GetWindowCompositor().SetKeyFocusArea(nullptr);
  }

  unity::Settings settings;
  MockManager::Ptr manager;
  ViewWrap view;
};

TEST_F(TestSessionView, Construct)
{
  EXPECT_TRUE(view.closable());
  EXPECT_FALSE(view.have_inhibitors());
  EXPECT_EQ(view.mode(), View::Mode::FULL);
}

TEST_F(TestSessionView, RequestCloseOnBoundingAreaClick)
{
  bool request_close = false;
  view.request_close.connect([&request_close] { request_close = true; });
  view.GetBoundingArea()->mouse_click.emit(0, 0, 0, 0);
  EXPECT_TRUE(request_close);
}

TEST_F(TestSessionView, ModeChange)
{
  view.mode = View::Mode::LOGOUT;
  EXPECT_EQ(view.mode, View::Mode::LOGOUT);

  view.mode = View::Mode::FULL;
  EXPECT_EQ(view.mode, View::Mode::FULL);
}

TEST_F(TestSessionView, ModeChangeOnShutdownSupported)
{
  ON_CALL(*manager, CanShutdown()).WillByDefault(testing::Return(true));

  view.mode = View::Mode::SHUTDOWN;
  EXPECT_EQ(view.mode, View::Mode::SHUTDOWN);
}

TEST_F(TestSessionView, ModeChangeOnShutdownUnsupported)
{
  ON_CALL(*manager, CanShutdown()).WillByDefault(testing::Return(false));

  view.mode = View::Mode::SHUTDOWN;
  EXPECT_EQ(view.mode, View::Mode::LOGOUT);
}

TEST_F(TestSessionView, FullModeButtons)
{
  ON_CALL(*manager, CanShutdown()).WillByDefault(testing::Return(true));
  ON_CALL(*manager, CanSuspend()).WillByDefault(testing::Return(true));
  ON_CALL(*manager, CanHibernate()).WillByDefault(testing::Return(true));
  view.mode.changed.emit(View::Mode::FULL);

  EXPECT_EQ(view.GetButtonByLabel("Log Out"), nullptr);
  EXPECT_NE(view.GetButtonByLabel("Lock"), nullptr);
  EXPECT_NE(view.GetButtonByLabel("Suspend"), nullptr);
  EXPECT_NE(view.GetButtonByLabel("Hibernate"), nullptr);
  EXPECT_NE(view.GetButtonByLabel("Shut Down"), nullptr);
  EXPECT_NE(view.GetButtonByLabel("Restart"), nullptr);

  ON_CALL(*manager, CanShutdown()).WillByDefault(testing::Return(false));
  view.mode.changed.emit(View::Mode::FULL);

  EXPECT_NE(view.GetButtonByLabel("Log Out"), nullptr);
  EXPECT_EQ(view.GetButtonByLabel("Shut Down"), nullptr);
  EXPECT_EQ(view.GetButtonByLabel("Restart"), nullptr);

  ON_CALL(*manager, CanSuspend()).WillByDefault(testing::Return(false));
  view.mode.changed.emit(View::Mode::FULL);

  EXPECT_EQ(view.GetButtonByLabel("Suspend"), nullptr);

  ON_CALL(*manager, CanHibernate()).WillByDefault(testing::Return(false));
  view.mode.changed.emit(View::Mode::FULL);

  EXPECT_EQ(view.GetButtonByLabel("Hibernate"), nullptr);
}

TEST_F(TestSessionView, ShutdownModeButtons)
{
  ON_CALL(*manager, CanShutdown()).WillByDefault(testing::Return(true));
  view.mode = View::Mode::SHUTDOWN;

  EXPECT_EQ(view.GetButtons().size(), 2);
  EXPECT_NE(view.GetButtonByLabel("Shut Down"), nullptr);
  EXPECT_NE(view.GetButtonByLabel("Restart"), nullptr);
}

TEST_F(TestSessionView, LogoutModeButtons)
{
  view.mode = View::Mode::LOGOUT;

  EXPECT_EQ(view.GetButtons().size(), 2);
  EXPECT_NE(view.GetButtonByLabel("Log Out"), nullptr);
  EXPECT_NE(view.GetButtonByLabel("Lock"), nullptr);
}

TEST_F(TestSessionView, FullModeTitle)
{
  EXPECT_TRUE(view.GetTitle().empty());
}

TEST_F(TestSessionView, ShutdownModeTitle)
{
  ON_CALL(*manager, CanShutdown()).WillByDefault(testing::Return(true));
  view.mode = View::Mode::SHUTDOWN;

  EXPECT_EQ(view.GetTitle(), "Shut Down");
}

TEST_F(TestSessionView, LogoutModeTitle)
{
  view.mode = View::Mode::LOGOUT;

  EXPECT_EQ(view.GetTitle(), "Log Out");
}

TEST_F(TestSessionView, ButtonsActivateRequestsHide)
{
  bool request_hide = false;
  view.request_hide.connect([&request_hide] { request_hide = true; });

  auto button = view.GetButtonByLabel("Lock");
  ASSERT_NE(button, nullptr);
  button->activated.emit();

  EXPECT_TRUE(request_hide);
}

TEST_F(TestSessionView, ButtonsActivateDeselectButton)
{
  auto button = view.GetButtonByLabel("Lock");
  ASSERT_NE(button, nullptr);
  button->highlighted = true;
  button->activated.emit();

  EXPECT_FALSE(button->highlighted());
}

TEST_F(TestSessionView, LockButtonActivateLocks)
{
  EXPECT_CALL(*manager, LockScreen());
  auto button = view.GetButtonByLabel("Lock");
  ASSERT_NE(button, nullptr);
  button->activated.emit();
}

TEST_F(TestSessionView, LogoutButtonActivateLogouts)
{
  view.mode = View::Mode::LOGOUT;
  EXPECT_CALL(*manager, Logout());
  auto button = view.GetButtonByLabel("Log Out");
  ASSERT_NE(button, nullptr);
  button->activated.emit();
}

TEST_F(TestSessionView, SuspendButtonActivateSuspends)
{
  ON_CALL(*manager, CanSuspend()).WillByDefault(testing::Return(true));
  view.mode.changed.emit(View::Mode::FULL);

  EXPECT_CALL(*manager, Suspend());
  auto button = view.GetButtonByLabel("Suspend");
  ASSERT_NE(button, nullptr);
  button->activated.emit();
}

TEST_F(TestSessionView, HibernateButtonActivateHibernates)
{
  ON_CALL(*manager, CanHibernate()).WillByDefault(testing::Return(true));
  view.mode.changed.emit(View::Mode::FULL);

  EXPECT_CALL(*manager, Hibernate());
  auto button = view.GetButtonByLabel("Hibernate");
  ASSERT_NE(button, nullptr);
  button->activated.emit();
}

TEST_F(TestSessionView, ShutdownButtonActivateShutsdown)
{
  ON_CALL(*manager, CanShutdown()).WillByDefault(testing::Return(true));
  view.mode = View::Mode::SHUTDOWN;

  EXPECT_CALL(*manager, Shutdown());
  auto button = view.GetButtonByLabel("Shut Down");
  ASSERT_NE(button, nullptr);
  button->activated.emit();
}

TEST_F(TestSessionView, RebootButtonActivateReboots)
{
  ON_CALL(*manager, CanShutdown()).WillByDefault(testing::Return(true));
  view.mode = View::Mode::SHUTDOWN;

  EXPECT_CALL(*manager, Reboot());
  auto button = view.GetButtonByLabel("Restart");
  ASSERT_NE(button, nullptr);
  button->activated.emit();
}

} // session
} // unity