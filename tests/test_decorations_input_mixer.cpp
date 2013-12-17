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

#include "decoration_mock_item.h"
#include "DecorationsInputMixer.h"

namespace
{
using namespace testing;

struct SigReceiver : sigc::trackable
{
  typedef NiceMock<SigReceiver> Nice;

  SigReceiver(MockItem const& item)
  {
    const_cast<MockItem&>(item).mouse_owner.changed.connect(sigc::mem_fun(this, &SigReceiver::MouseOwnerChanged));
  }

  MOCK_METHOD1(MouseOwnerChanged, void(bool));
};

struct TestDecorationInputMixer : Test
{
  InputMixer mixer;
};

TEST_F(TestDecorationInputMixer, PushToFrontItem)
{
  auto item1 = SizedMockItem(20, 20);
  mixer.PushToFront(item1);

  auto item2 = SizedMockItem(20, 20);
  mixer.PushToFront(item2);

  auto item3 = SizedMockItem(20, 20);
  mixer.PushToFront(item3);

  EXPECT_EQ(*std::next(mixer.Items().begin(), 0), item3);
  EXPECT_EQ(*std::next(mixer.Items().begin(), 1), item2);
  EXPECT_EQ(*std::next(mixer.Items().begin(), 2), item1);
}

TEST_F(TestDecorationInputMixer, PushToBackItem)
{
  auto item1 = SizedMockItem(20, 20);
  mixer.PushToBack(item1);

  auto item2 = SizedMockItem(20, 20);
  mixer.PushToBack(item2);

  auto item3 = SizedMockItem(20, 20);
  mixer.PushToBack(item3);

  EXPECT_EQ(*std::next(mixer.Items().begin(), 0), item1);
  EXPECT_EQ(*std::next(mixer.Items().begin(), 1), item2);
  EXPECT_EQ(*std::next(mixer.Items().begin(), 2), item3);
}

}
