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

  auto const& items = mixer.Items();
  ASSERT_EQ(3, items.size());
  EXPECT_EQ(item3, *std::next(items.begin(), 0));
  EXPECT_EQ(item2, *std::next(items.begin(), 1));
  EXPECT_EQ(item1, *std::next(items.begin(), 2));

  mixer.PushToFront(item2);
  ASSERT_EQ(3, mixer.Items().size());
  EXPECT_EQ(item2, *std::next(items.begin(), 0));
  EXPECT_EQ(item3, *std::next(items.begin(), 1));
  EXPECT_EQ(item1, *std::next(items.begin(), 2));
}

TEST_F(TestDecorationInputMixer, PushToBackItem)
{
  auto item1 = SizedMockItem(20, 20);
  mixer.PushToBack(item1);

  auto item2 = SizedMockItem(20, 20);
  mixer.PushToBack(item2);

  auto item3 = SizedMockItem(20, 20);
  mixer.PushToBack(item3);

  auto const& items = mixer.Items();
  ASSERT_EQ(3, items.size());
  EXPECT_EQ(item1, *std::next(items.begin(), 0));
  EXPECT_EQ(item2, *std::next(items.begin(), 1));
  EXPECT_EQ(item3, *std::next(items.begin(), 2));

  mixer.PushToBack(item2);
  ASSERT_EQ(3, items.size());
  EXPECT_EQ(item1, *std::next(items.begin(), 0));
  EXPECT_EQ(item3, *std::next(items.begin(), 1));
  EXPECT_EQ(item2, *std::next(items.begin(), 2));
}


TEST_F(TestDecorationInputMixer, EnterEvent)
{
  auto item1 = SizedMockItem(30, 30);
  SigReceiver sig1(*item1);
  mixer.PushToFront(item1);

  auto item2 = SizedMockItem(20, 20);
  SigReceiver sig2(*item2);
  mixer.PushToFront(item2);

  auto item3 = SizedMockItem(10, 10);
  SigReceiver sig3(*item3);
  mixer.PushToFront(item3);

  {
  EXPECT_CALL(sig1, MouseOwnerChanged(_)).Times(0);
  EXPECT_CALL(sig2, MouseOwnerChanged(_)).Times(0);
  EXPECT_CALL(sig3, MouseOwnerChanged(true));

  mixer.EnterEvent(CompPoint(5, 5));

  ASSERT_TRUE(item3->mouse_owner());
  ASSERT_EQ(item3, mixer.GetMouseOwner());
  }

  {
  EXPECT_CALL(sig1, MouseOwnerChanged(_)).Times(0);
  EXPECT_CALL(sig2, MouseOwnerChanged(true));
  EXPECT_CALL(sig3, MouseOwnerChanged(false));

  mixer.EnterEvent(CompPoint(15, 15));

  ASSERT_FALSE(item3->mouse_owner());
  ASSERT_TRUE(item2->mouse_owner());
  ASSERT_EQ(item2, mixer.GetMouseOwner());
  }

  {
  EXPECT_CALL(sig1, MouseOwnerChanged(true));
  EXPECT_CALL(sig2, MouseOwnerChanged(false));
  EXPECT_CALL(sig3, MouseOwnerChanged(_)).Times(0);

  mixer.EnterEvent(CompPoint(25, 25));

  ASSERT_FALSE(item2->mouse_owner());
  ASSERT_TRUE(item1->mouse_owner());
  ASSERT_EQ(item1, mixer.GetMouseOwner());
  }
}

TEST_F(TestDecorationInputMixer, MotionEvent)
{
  auto item1 = SizedMockItem(30, 30);
  SigReceiver sig1(*item1);
  mixer.PushToFront(item1);

  auto item2 = SizedMockItem(20, 20);
  SigReceiver sig2(*item2);
  mixer.PushToFront(item2);

  auto item3 = SizedMockItem(10, 10);
  SigReceiver sig3(*item3);
  mixer.PushToFront(item3);

  {
  EXPECT_CALL(sig1, MouseOwnerChanged(_)).Times(0);
  EXPECT_CALL(sig2, MouseOwnerChanged(_)).Times(0);
  EXPECT_CALL(sig3, MouseOwnerChanged(true));
  EXPECT_CALL(*item1, MotionEvent(_)).Times(0);
  EXPECT_CALL(*item2, MotionEvent(_)).Times(0);
  EXPECT_CALL(*item3, MotionEvent(CompPoint(5, 5)));

  mixer.MotionEvent(CompPoint(5, 5));

  ASSERT_TRUE(item3->mouse_owner());
  ASSERT_EQ(item3, mixer.GetMouseOwner());
  }

  {
  EXPECT_CALL(sig1, MouseOwnerChanged(_)).Times(0);
  EXPECT_CALL(sig2, MouseOwnerChanged(true));
  EXPECT_CALL(sig3, MouseOwnerChanged(false));
  EXPECT_CALL(*item1, MotionEvent(_)).Times(0);
  EXPECT_CALL(*item2, MotionEvent(CompPoint(15, 15)));
  EXPECT_CALL(*item3, MotionEvent(_)).Times(0);

  mixer.MotionEvent(CompPoint(15, 15));

  ASSERT_FALSE(item3->mouse_owner());
  ASSERT_TRUE(item2->mouse_owner());
  ASSERT_EQ(item2, mixer.GetMouseOwner());
  }

  {
  EXPECT_CALL(sig1, MouseOwnerChanged(true));
  EXPECT_CALL(sig2, MouseOwnerChanged(false));
  EXPECT_CALL(sig3, MouseOwnerChanged(_)).Times(0);
  EXPECT_CALL(*item1, MotionEvent(CompPoint(25, 25)));
  EXPECT_CALL(*item2, MotionEvent(_)).Times(0);
  EXPECT_CALL(*item3, MotionEvent(_)).Times(0);

  mixer.MotionEvent(CompPoint(25, 25));

  ASSERT_FALSE(item2->mouse_owner());
  ASSERT_TRUE(item1->mouse_owner());
  ASSERT_EQ(item1, mixer.GetMouseOwner());
  }
}

}
