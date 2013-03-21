/*
 * Copyright 2012-2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 */

#include <gmock/gmock.h>

#include "TrashLauncherIcon.h"

using namespace unity;
using namespace unity::launcher;

namespace
{
struct MockFileManager : FileManager
{
  MOCK_METHOD2(Open, void(std::string const& uri, unsigned long long time));
};

struct TestTrashLauncherIcon : testing::Test
{
  TestTrashLauncherIcon()
    : fm_(std::make_shared<MockFileManager>())
    , icon(fm_)
  {}

  std::shared_ptr<MockFileManager> fm_;
  TrashLauncherIcon icon;
};

TEST_F(TestTrashLauncherIcon, Position)
{
  EXPECT_EQ(icon.position(), AbstractLauncherIcon::Position::END);
}

TEST_F(TestTrashLauncherIcon, Activate)
{
  unsigned long long time = g_random_int();
  EXPECT_CALL(*fm_, Open("trash:", time));
  icon.Activate(ActionArg(ActionArg::Source::LAUNCHER, 0, time));
}

}
