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

TEST_F(TestDecorationInputMixer, RemoveItem)
{
  auto item1 = RandomMockItem();
  mixer.PushToFront(item1);

  auto item2 = RandomMockItem();
  mixer.PushToFront(item2);

  auto item3 = RandomMockItem();
  mixer.PushToFront(item3);

  auto const& items = mixer.Items();
  ASSERT_EQ(3, items.size());

  mixer.Remove(item2);
  ASSERT_EQ(2, items.size());
  EXPECT_EQ(item3, *std::next(items.begin(), 0));
  EXPECT_EQ(item1, *std::next(items.begin(), 1));

  mixer.Remove(item1);
  ASSERT_EQ(1, items.size());
  EXPECT_EQ(item3, *std::next(items.begin(), 0));

  mixer.EnterEvent(CompPoint(item3->Geometry().x2(), item3->Geometry().y1()));
  ASSERT_EQ(item3, mixer.GetMouseOwner());

  mixer.Remove(item3);
  ASSERT_TRUE(items.empty());
  ASSERT_EQ(nullptr, mixer.GetMouseOwner());
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
  CompPoint point(5, 5);
  Time timestamp = g_random_int();
  EXPECT_CALL(sig1, MouseOwnerChanged(_)).Times(0);
  EXPECT_CALL(sig2, MouseOwnerChanged(_)).Times(0);
  EXPECT_CALL(sig3, MouseOwnerChanged(true));
  EXPECT_CALL(*item1, MotionEvent(_, _)).Times(0);
  EXPECT_CALL(*item2, MotionEvent(_, _)).Times(0);
  EXPECT_CALL(*item3, MotionEvent(point, timestamp));

  mixer.MotionEvent(point, timestamp);

  ASSERT_TRUE(item3->mouse_owner());
  ASSERT_EQ(item3, mixer.GetMouseOwner());
  }

  {
  CompPoint point(15, 15);
  Time timestamp = g_random_int();
  EXPECT_CALL(sig1, MouseOwnerChanged(_)).Times(0);
  EXPECT_CALL(sig2, MouseOwnerChanged(true));
  EXPECT_CALL(sig3, MouseOwnerChanged(false));
  EXPECT_CALL(*item1, MotionEvent(_, _)).Times(0);
  EXPECT_CALL(*item2, MotionEvent(point, timestamp));
  EXPECT_CALL(*item3, MotionEvent(_, _)).Times(0);

  mixer.MotionEvent(point, timestamp);

  ASSERT_FALSE(item3->mouse_owner());
  ASSERT_TRUE(item2->mouse_owner());
  ASSERT_EQ(item2, mixer.GetMouseOwner());
  }

  {
  CompPoint point(25, 25);
  Time timestamp = g_random_int();
  EXPECT_CALL(sig1, MouseOwnerChanged(true));
  EXPECT_CALL(sig2, MouseOwnerChanged(false));
  EXPECT_CALL(sig3, MouseOwnerChanged(_)).Times(0);
  EXPECT_CALL(*item1, MotionEvent(point, timestamp));
  EXPECT_CALL(*item2, MotionEvent(_, _)).Times(0);
  EXPECT_CALL(*item3, MotionEvent(_, _)).Times(0);

  mixer.MotionEvent(point, timestamp);

  ASSERT_FALSE(item2->mouse_owner());
  ASSERT_TRUE(item1->mouse_owner());
  ASSERT_EQ(item1, mixer.GetMouseOwner());
  }
}

TEST_F(TestDecorationInputMixer, LeaveEvent)
{
  auto item1 = SizedMockItem(30, 30);
  mixer.PushToFront(item1);

  auto item2 = SizedMockItem(20, 20);
  mixer.PushToFront(item2);

  auto item3 = SizedMockItem(10, 10);
  mixer.PushToFront(item3);

  {
  mixer.EnterEvent(CompPoint(item1->Geometry().x2(), item1->Geometry().y2()));
  SigReceiver::Nice sig1(*item1);
  SigReceiver::Nice sig2(*item2);
  SigReceiver::Nice sig3(*item3);

  EXPECT_CALL(sig1, MouseOwnerChanged(false));
  EXPECT_CALL(sig2, MouseOwnerChanged(_)).Times(0);
  EXPECT_CALL(sig3, MouseOwnerChanged(_)).Times(0);

  mixer.LeaveEvent(CompPoint());

  ASSERT_FALSE(item1->mouse_owner());
  ASSERT_EQ(nullptr, mixer.GetMouseOwner());
  }

  {
  mixer.EnterEvent(CompPoint(item2->Geometry().x2(), item2->Geometry().y2()));
  SigReceiver::Nice sig1(*item1);
  SigReceiver::Nice sig2(*item2);
  SigReceiver::Nice sig3(*item3);

  EXPECT_CALL(sig1, MouseOwnerChanged(_)).Times(0);
  EXPECT_CALL(sig2, MouseOwnerChanged(false));
  EXPECT_CALL(sig3, MouseOwnerChanged(_)).Times(0);

  mixer.LeaveEvent(CompPoint());

  ASSERT_FALSE(item2->mouse_owner());
  ASSERT_EQ(nullptr, mixer.GetMouseOwner());
  }

  {
  mixer.EnterEvent(CompPoint(item3->Geometry().x2(), item3->Geometry().y2()));
  SigReceiver::Nice sig1(*item1);
  SigReceiver::Nice sig2(*item2);
  SigReceiver::Nice sig3(*item3);

  EXPECT_CALL(sig1, MouseOwnerChanged(_)).Times(0);
  EXPECT_CALL(sig2, MouseOwnerChanged(_)).Times(0);
  EXPECT_CALL(sig3, MouseOwnerChanged(false));

  mixer.LeaveEvent(CompPoint());

  ASSERT_FALSE(item3->mouse_owner());
  ASSERT_EQ(nullptr, mixer.GetMouseOwner());
  }
}

TEST_F(TestDecorationInputMixer, ButtonDownEvent)
{
  auto item1 = SizedMockItem(30, 30);
  mixer.PushToFront(item1);

  auto item2 = SizedMockItem(20, 20);
  mixer.PushToFront(item2);

  auto item3 = SizedMockItem(10, 10);
  mixer.PushToFront(item3);

  {
  CompPoint point(5, 5);
  Time timestamp = g_random_int();
  EXPECT_CALL(*item1, ButtonDownEvent(_, _, _)).Times(0);
  EXPECT_CALL(*item2, ButtonDownEvent(_, _, _)).Times(0);
  EXPECT_CALL(*item3, ButtonDownEvent(point, 1, timestamp));

  mixer.EnterEvent(point);
  mixer.ButtonDownEvent(point, 1, timestamp);
  mixer.ButtonUpEvent(point, 1, timestamp);
  }

  {
  CompPoint point(15, 15);
  Time timestamp = g_random_int();
  EXPECT_CALL(*item1, ButtonDownEvent(_, _, _)).Times(0);
  EXPECT_CALL(*item2, ButtonDownEvent(point, 2, timestamp));
  EXPECT_CALL(*item3, ButtonDownEvent(_, _, timestamp)).Times(0);

  mixer.EnterEvent(point);
  mixer.ButtonDownEvent(point, 2, timestamp);
  mixer.ButtonUpEvent(point, 2, timestamp);
  }

  {
  CompPoint point(25, 25);
  Time timestamp = g_random_int();
  EXPECT_CALL(*item1, ButtonDownEvent(point, 3, timestamp));
  EXPECT_CALL(*item2, ButtonDownEvent(_, _, _)).Times(0);
  EXPECT_CALL(*item3, ButtonDownEvent(_, _, _)).Times(0);

  mixer.EnterEvent(point);
  mixer.ButtonDownEvent(point, 3, timestamp);
  mixer.ButtonUpEvent(point, 3, timestamp);
  }
}

TEST_F(TestDecorationInputMixer, ButtonUpEvent)
{
  auto item1 = SizedMockItem(30, 30);
  mixer.PushToFront(item1);

  auto item2 = SizedMockItem(20, 20);
  mixer.PushToFront(item2);

  auto item3 = SizedMockItem(10, 10);
  mixer.PushToFront(item3);

  {
  CompPoint point(5, 5);
  Time timestamp = g_random_int();
  EXPECT_CALL(*item1, ButtonUpEvent(_, _, _)).Times(0);
  EXPECT_CALL(*item2, ButtonUpEvent(_, _, _)).Times(0);
  EXPECT_CALL(*item3, ButtonUpEvent(point, 1, timestamp));

  mixer.EnterEvent(point);
  mixer.ButtonUpEvent(point, 1, timestamp);
  }

  {
  CompPoint point(15, 15);
  Time timestamp = g_random_int();
  EXPECT_CALL(*item1, ButtonUpEvent(_, _, _)).Times(0);
  EXPECT_CALL(*item2, ButtonUpEvent(point, 2, timestamp));
  EXPECT_CALL(*item3, ButtonUpEvent(_, _, _)).Times(0);

  mixer.EnterEvent(point);
  mixer.ButtonUpEvent(point, 2, timestamp);
  }

  {
  CompPoint point(25, 25);
  Time timestamp = g_random_int();
  EXPECT_CALL(*item1, ButtonUpEvent(point, 3, timestamp));
  EXPECT_CALL(*item2, ButtonUpEvent(_, _, _)).Times(0);
  EXPECT_CALL(*item3, ButtonUpEvent(_, _, _)).Times(0);

  mixer.EnterEvent(point);
  mixer.ButtonUpEvent(point, 3, timestamp);
  }
}

TEST_F(TestDecorationInputMixer, ButtonDownEventGrab)
{
  auto item1 = SizedMockItem(30, 30);
  mixer.PushToFront(item1);

  auto item2 = SizedMockItem(20, 20);
  mixer.PushToFront(item2);

  auto item3 = SizedMockItem(10, 10);
  mixer.PushToFront(item3);

  {
  CompPoint point(5, 5);
  Time timestamp = g_random_int();
  EXPECT_CALL(*item1, ButtonDownEvent(_, _, _)).Times(0);
  EXPECT_CALL(*item2, ButtonDownEvent(_, _, _)).Times(0);
  EXPECT_CALL(*item3, ButtonDownEvent(point, 1, timestamp));

  mixer.EnterEvent(point);
  mixer.ButtonDownEvent(point, 1, timestamp);
  ASSERT_TRUE(item3->mouse_owner());
  }

  {
  CompPoint point(15, 15);
  Time timestamp = g_random_int();
  EXPECT_CALL(*item1, MotionEvent(_, _)).Times(0);
  EXPECT_CALL(*item2, MotionEvent(_, _)).Times(0);
  EXPECT_CALL(*item3, MotionEvent(point, timestamp));
  mixer.MotionEvent(point, timestamp);
  ASSERT_TRUE(item3->mouse_owner());
  }

  {
  CompPoint point(25, 25);
  Time timestamp = g_random_int();
  EXPECT_CALL(*item1, MotionEvent(_, _)).Times(0);
  EXPECT_CALL(*item2, MotionEvent(_, _)).Times(0);
  EXPECT_CALL(*item3, MotionEvent(point, timestamp));
  mixer.MotionEvent(point, timestamp);
  ASSERT_TRUE(item3->mouse_owner());
  }

  {
  CompPoint point(15, 15);
  Time timestamp = g_random_int();
  EXPECT_CALL(*item1, ButtonUpEvent(_, _, _)).Times(0);
  EXPECT_CALL(*item2, ButtonUpEvent(_, _, _)).Times(0);
  EXPECT_CALL(*item3, ButtonUpEvent(point, 1, timestamp));
  mixer.ButtonUpEvent(point, 1, timestamp);

  EXPECT_FALSE(item3->mouse_owner());
  EXPECT_TRUE(item2->mouse_owner());
  }
}

TEST_F(TestDecorationInputMixer, ParentRemovalFromChildrenButtonDown)
{
  struct BadItem : SimpleItem
  {
    void ButtonDownEvent(CompPoint const&, unsigned button, Time) override { mx_->reset(); }
    InputMixer::Ptr* mx_;
  };

  auto mx = std::make_shared<InputMixer>();
  std::weak_ptr<InputMixer> weak_mixer(mx);
  std::weak_ptr<BadItem> weak_bad_item;

  {
  auto bad_item = std::make_shared<BadItem>();
  weak_bad_item = bad_item;
  bad_item->SetSize(10, 10);
  bad_item->mx_ = &mx;
  mx->PushToFront(bad_item);
  }

  ASSERT_FALSE(weak_bad_item.expired());
  CompPoint point(1, 1);
  Time timestamp = g_random_int();
  mx->EnterEvent(point);
  mx->ButtonDownEvent(point, 1, timestamp);

  EXPECT_TRUE(weak_bad_item.expired());
  EXPECT_TRUE(weak_mixer.expired());
}

TEST_F(TestDecorationInputMixer, ParentRemovalFromChildrenButtonUp)
{
  struct BadItem : SimpleItem
  {
    void ButtonUpEvent(CompPoint const&, unsigned button, Time) override { mx_->reset(); }
    InputMixer::Ptr* mx_;
  };

  auto mx = std::make_shared<InputMixer>();
  std::weak_ptr<InputMixer> weak_mixer(mx);
  std::weak_ptr<BadItem> weak_bad_item;

  {
  auto bad_item = std::make_shared<BadItem>();
  weak_bad_item = bad_item;
  bad_item->SetSize(10, 10);
  bad_item->mx_ = &mx;
  mx->PushToFront(bad_item);
  }

  ASSERT_FALSE(weak_bad_item.expired());
  CompPoint point(1, 1);
  Time timestamp = g_random_int();
  mx->EnterEvent(point);
  mx->ButtonDownEvent(point, 1, timestamp);
  mx->ButtonUpEvent(point, 1, timestamp);

  EXPECT_TRUE(weak_bad_item.expired());
  EXPECT_TRUE(weak_mixer.expired());
}

}
