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
#include <glib.h>
#include "DecorationsWidgets.h"

namespace
{
using namespace unity::decoration;
using namespace testing;

int random_positive_int()
{
  return g_random_int_range(0, G_MAXSHORT/2);
}

int random_int()
{
  return g_random_int_range(G_MINSHORT/2, G_MAXSHORT/2);
}

struct MockItem : public SimpleItem
{
  typedef NiceMock<MockItem> Nice;
  typedef std::shared_ptr<MockItem> Ptr;

  MockItem()
  {
    visible = true;
    ON_CALL(*this, GetNaturalWidth()).WillByDefault(Invoke([this] { return SimpleItem::GetNaturalWidth(); }));
    ON_CALL(*this, GetNaturalHeight()).WillByDefault(Invoke([this] { return SimpleItem::GetNaturalHeight(); }));
    ON_CALL(*this, SetCoords(_, _)).WillByDefault(Invoke([this] (int x, int y) { SimpleItem::SetCoords(x, y); }));
    ON_CALL(*this, SetX(_)).WillByDefault(Invoke([this] (int x) { SimpleItem::SetX(x); }));
    ON_CALL(*this, SetY(_)).WillByDefault(Invoke([this] (int y) { SimpleItem::SetY(y); }));
    ON_CALL(*this, SetSize(_, _)).WillByDefault(Invoke([this] (int w, int h) { SimpleItem::SetSize(w, h); }));
    ON_CALL(*this, SetWidth(_)).WillByDefault(Invoke([this] (int w) { SimpleItem::SetWidth(w); }));
    ON_CALL(*this, SetHeight(_)).WillByDefault(Invoke([this] (int h) { SimpleItem::SetHeight(h); }));
    ON_CALL(*this, SetMaxWidth(_)).WillByDefault(Invoke([this] (int mw) { SimpleItem::SetMaxWidth(mw); }));
    ON_CALL(*this, SetMaxHeight(_)).WillByDefault(Invoke([this] (int mh) { SimpleItem::SetMaxHeight(mh); }));
    ON_CALL(*this, SetMinWidth(_)).WillByDefault(Invoke([this] (int mw) { SimpleItem::SetMinWidth(mw); }));
    ON_CALL(*this, SetMinHeight(_)).WillByDefault(Invoke([this] (int mh) { SimpleItem::SetMinHeight(mh); }));
  }

  MOCK_CONST_METHOD0(GetNaturalWidth, int());
  MOCK_CONST_METHOD0(GetNaturalHeight, int());
  MOCK_METHOD2(SetCoords, void(int, int));
  MOCK_METHOD1(SetX, void(int));
  MOCK_METHOD1(SetY, void(int));
  MOCK_METHOD2(SetSize, void(int, int));
  MOCK_METHOD1(SetWidth, void(int));
  MOCK_METHOD1(SetHeight, void(int));
  MOCK_METHOD1(SetMaxWidth, void(int));
  MOCK_METHOD1(SetMaxHeight, void(int));
  MOCK_METHOD1(SetMinWidth, void(int));
  MOCK_METHOD1(SetMinHeight, void(int));

  using SimpleItem::geo_parameters_changed;
  using SimpleItem::rect_;
  using SimpleItem::natural_;
  using SimpleItem::max_;
  using SimpleItem::min_;
};

struct SigReceiver : sigc::trackable
{
  typedef NiceMock<SigReceiver> Nice;

  SigReceiver(MockItem const& item)
  {
    const_cast<MockItem&>(item).geo_parameters_changed.connect(sigc::mem_fun(this, &SigReceiver::GeoChanged));
  }

  MOCK_METHOD0(GeoChanged, void());
};

struct TestDecorationItem : Test
{
  TestDecorationItem()
   : sig_receiver(item)
  {
    item.SetSize(g_random_int_range(10, 100), g_random_int_range(10, 30));
  }

  MockItem::Nice item;
  SigReceiver::Nice sig_receiver;
};

TEST_F(TestDecorationItem, DefaultMaxSize)
{
  MockItem item;
  EXPECT_EQ(item.max_, nux::Size(std::numeric_limits<int>::max(), std::numeric_limits<int>::max()));
}

TEST_F(TestDecorationItem, DefaultMinSize)
{
  MockItem item;
  EXPECT_EQ(item.min_, nux::Size(0, 0));
}

TEST_F(TestDecorationItem, DefaultNaturalSize)
{
  MockItem item;
  EXPECT_EQ(item.natural_, nux::Size(0, 0));
}

TEST_F(TestDecorationItem, Geometry)
{
  EXPECT_EQ(item.rect_, item.Geometry());
}

TEST_F(TestDecorationItem, NaturalWidth)
{
  EXPECT_EQ(item.natural_.width, item.GetNaturalWidth());
  EXPECT_EQ(item.Geometry().width(), item.GetNaturalWidth());
}

TEST_F(TestDecorationItem, NaturalHeight)
{
  EXPECT_EQ(item.natural_.height, item.GetNaturalHeight());
  EXPECT_EQ(item.Geometry().height(), item.GetNaturalHeight());
}

TEST_F(TestDecorationItem, SetSize)
{
  int w = random_positive_int(), h = random_positive_int();

  EXPECT_CALL(item, SetMinWidth(w));
  EXPECT_CALL(item, SetMaxWidth(w));
  EXPECT_CALL(item, SetMinHeight(h));
  EXPECT_CALL(item, SetMaxHeight(h));
  EXPECT_CALL(sig_receiver, GeoChanged()).Times(AtLeast(1));

  item.SetSize(w, h);

  EXPECT_EQ(w, item.GetNaturalWidth());
  EXPECT_EQ(h, item.GetNaturalHeight());
}

TEST_F(TestDecorationItem, SetNegativeSize)
{
  item.SetSize(g_random_int_range(G_MININT, 0), g_random_int_range(G_MININT, 0));

  EXPECT_EQ(0, item.GetNaturalWidth());
  EXPECT_EQ(0, item.GetNaturalHeight());
}

TEST_F(TestDecorationItem, SetWidth)
{
  int w = random_positive_int();

  EXPECT_CALL(item, SetSize(w, item.Geometry().height()));
  item.SetWidth(w);

  EXPECT_EQ(w, item.GetNaturalWidth());
  EXPECT_EQ(item.Geometry().height(), item.GetNaturalHeight());
}

TEST_F(TestDecorationItem, SetHeight)
{
  int h = random_positive_int();

  EXPECT_CALL(item, SetSize(item.Geometry().width(), h));
  item.SetHeight(h);

  EXPECT_EQ(item.Geometry().width(), item.GetNaturalWidth());
  EXPECT_EQ(h, item.GetNaturalHeight());
}

TEST_F(TestDecorationItem, SetCoords)
{
  int x = random_int(), y = random_int();

  EXPECT_CALL(sig_receiver, GeoChanged());
  item.SetCoords(x, y);

  EXPECT_EQ(item.Geometry().x(), x);
  EXPECT_EQ(item.Geometry().y(), y);
}

TEST_F(TestDecorationItem, SetX)
{
  int x = random_int();

  EXPECT_CALL(item, SetCoords(x, item.Geometry().y()));
  item.SetX(x);

  EXPECT_EQ(item.Geometry().x(), x);
}

TEST_F(TestDecorationItem, SetY)
{
  int y = random_int();

  EXPECT_CALL(item, SetCoords(item.Geometry().x(), y));
  item.SetY(y);

  EXPECT_EQ(item.Geometry().y(), y);
}

//

struct TestDecorationLayout : Test
{
  MockItem::Ptr SizedMockItem(int w, int h)
  {
    auto item = std::make_shared<MockItem::Nice>();
    item->SetSize(w, h);
    return item;
  }

  MockItem::Ptr RandomMockItem()
  {
    return SizedMockItem(g_random_int_range(10, 100), g_random_int_range(10, 100));
  }

  Layout layout;
};

TEST_F(TestDecorationLayout, AppendUnlimited)
{
  layout.SetCoords(random_int(), random_int());
  auto expected_geo = layout.Geometry();

  for (int i = 0; i < 100; ++i)
  {
    auto item = RandomMockItem();
    auto const& item_geo = item->Geometry();
    expected_geo.setWidth(expected_geo.width() + item_geo.width());
    expected_geo.setHeight(std::max(expected_geo.height(), item_geo.height()));

    layout.Append(item);
    ASSERT_EQ(expected_geo, layout.Geometry());
    ASSERT_EQ(layout.Geometry().x2() - item_geo.width(), item_geo.x());
    ASSERT_EQ(layout.Geometry().y() + (layout.Geometry().height() - item_geo.height()) / 2, item_geo.y());
  }

  EXPECT_EQ(100, layout.Items().size());
}

TEST_F(TestDecorationLayout, AppendInvisible)
{
  CompRect expected_geo;
  for (int i = 0; i < 100; ++i)
  {
    auto item = RandomMockItem();
    item->visible = false;
    layout.Append(item);
    ASSERT_EQ(expected_geo, layout.Geometry());
  }

  EXPECT_EQ(100, layout.Items().size());
  EXPECT_EQ(CompRect(), layout.Geometry());
}

TEST_F(TestDecorationLayout, AppendUnlimitedInternalPadding)
{
  layout.inner_padding = g_random_int_range(10, 50);
  layout.SetCoords(random_int(), random_int());
  auto expected_geo = layout.Geometry();

  for (int i = 0; i < 100; ++i)
  {
    auto item = RandomMockItem();
    auto const& item_geo = item->Geometry();
    expected_geo.setWidth(expected_geo.width() + item_geo.width() + layout.inner_padding() * ((i > 0) ? 1 : 0));
    expected_geo.setHeight(std::max(expected_geo.height(), item_geo.height()));

    layout.Append(item);
    ASSERT_EQ(expected_geo, layout.Geometry());
    ASSERT_EQ(layout.Geometry().x2() - item_geo.width(), item_geo.x());
    ASSERT_EQ(layout.Geometry().y() + (layout.Geometry().height() - item_geo.height()) / 2, item_geo.y());
  }

  EXPECT_EQ(100, layout.Items().size());
}

TEST_F(TestDecorationLayout, AppendWithMaxWidth)
{
  layout.SetCoords(random_int(), random_int());

  for (int i = 0; i < 100; ++i)
    layout.Append(RandomMockItem());

  ASSERT_EQ(100, layout.Items().size());

  auto const& layout_geo = layout.Geometry();
  int new_width = layout_geo.width()/2;
  layout.SetMaxWidth(new_width);

  ASSERT_EQ(new_width, layout.GetMaxWidth());
  ASSERT_EQ(new_width, layout_geo.width());

  for (auto const& item : layout.Items())
  {
    auto const& item_geo = item->Geometry();

    if (item_geo.x2() < layout_geo.x2())
    {
      ASSERT_EQ(item->GetNaturalWidth(), item_geo.width());
      ASSERT_EQ(item->GetNaturalHeight(), item_geo.height());
    }
    else
    {
      ASSERT_EQ(std::max(0, layout_geo.x2() - item_geo.x1()), item_geo.width());
    }
  }
}

TEST_F(TestDecorationLayout, ExpandWithMaxWidth)
{
  layout.SetCoords(random_int(), random_int());

  for (int i = 0; i < 100; ++i)
    layout.Append(RandomMockItem());

  ASSERT_EQ(100, layout.Items().size());

  auto const& layout_geo = layout.Geometry();
  int full_width = layout_geo.width();

  layout.SetMaxWidth(full_width/2);
  ASSERT_EQ(full_width/2, layout_geo.width());

  layout.SetMaxWidth(std::numeric_limits<int>::max());
  ASSERT_EQ(full_width, layout_geo.width());

  for (auto const& item : layout.Items())
    ASSERT_EQ(item->GetNaturalWidth(), item->Geometry().width());
}

}
